//
// rootnode.h
// This file is part of Methyl
// Copyright (C) 2002-2014 HostileFork.com
//
// Methyl is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Methyl is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Methyl.  If not, see <http://www.gnu.org/licenses/>.
//
// See http://methyl.hostilefork.com/ for more information on this project
//

#ifndef METHYL_ROOTNODE_H
#define METHYL_ROOTNODE_H

#include "methyl/nodeprivate.h"
#include "methyl/noderef.h"
#include "methyl/observer.h"
#include "methyl/context.h"

namespace methyl {

///
/// OWNERSHIP RESPONSIBILITY ROOT NODE WRAPPER
///

template <class T = Node>
class RootNode {
    static_assert(
        std::is_base_of<Node, T>::value,
        "RootNode<> may only be parameterized with a class derived from Node"
    );

    static_assert(
        not std::is_const<T>::value,
        "RootNode<> may not be parameterized by a const Node accessor"
    );

private:
    // Right now it's too labor intensive to forward declare all of Node,
    // so we just cook up something with the same footprint and make sure
    // the parameter hasn't added any data members or multiple inheritance
    class FakeNode {
        NodePrivate * _nodePrivate;
        shared_ptr<Context> _context;
        virtual ~FakeNode() {}
    };
    static_assert(
        sizeof(T) == sizeof(FakeNode),
        "Classes derived from Node must not add any new data members"
    );

protected:
    T _nodeDoNotUseDirectly;

friend class Node;
friend class Engine;
template <class> friend class RootNode;
template <class> friend class NodeRef;

template <class> friend struct ::std::hash;
template <class> friend struct ::methyl::congruent_to;
template <class> friend struct ::methyl::congruence_hash;

template <class T1, class T2> friend
bool operator==(RootNode<T1> const & x, RootNode<T2> const & y);

private:
    T & getNode()
        { return _nodeDoNotUseDirectly; }
    T const & getNode() const
        { return _nodeDoNotUseDirectly; }

private:
    unique_ptr<NodePrivate> extractNodePrivate() {
        NodePrivate & node = getNode().getNodePrivate();
        // REVIEW: We keep the context alive so constructors can extract it out
        // but is this the best way of doing it?
        shared_ptr<Context> context = getNode().getContext();
        getNode().setInternalProperties(nullptr, context);
        return unique_ptr<NodePrivate> (&node);
    }

public:
    explicit operator bool () const
        { return getNode().maybeGetNodePrivate() != nullptr; }

    NodeRef<T const> get() const {
        Q_ASSERT(static_cast<bool>(*this));
        return NodeRef<T const>(
            getNode().getNodePrivate(),
            getNode().getContext()
        );
    }

    NodeRef<T> get() {
        Q_ASSERT(static_cast<bool>(*this));
        return NodeRef<T>(
            getNode().getNodePrivate(),
            getNode().getContext()
        );
    }


public:
    // disable copy assignment and default construction
    RootNode () = delete;
    RootNode & operator= (RootNode const &) = delete; 

    void operator= (nullptr_t dummy) {
        unique_ptr<NodePrivate> thisNode (extractNodePrivate());

        // REVIEW: Go through and mimic what unique_ptr does
        return;
    }

    RootNode & operator= (RootNode && other)
    {
        unique_ptr<NodePrivate> otherNode = other.extractNodePrivate();
        unique_ptr<NodePrivate> thisNode = extractNodePrivate();
        getNode().setInternalProperties(
            otherNode.release(), other.getNode().getContext()
        );
        return *this;
    }

    // Only allow implicit casts if the accessor is going toward a base
    template <class U>
    RootNode (
        RootNode<U> other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) noexcept :
        RootNode (
            std::move(other.extractNodePrivate()),
            other.getNode().getContext()
        )
    {
    }

    template <class U>
    explicit RootNode (
        RootNode<U> other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) noexcept :
        RootNode (std::move(other.extractNodePrivate()))
    {
    }
    RootNode (RootNode && other) noexcept :
        RootNode(
            std::move(other.extractNodePrivate()),
            other.getNode().getContext()
        )
    {
    }

protected:
    RootNode (
        unique_ptr<NodePrivate> nodePrivate,
        shared_ptr<Context> context
    ) {
        getNode().setInternalProperties(nodePrivate.release(), context);
    }

public:
    T const * operator-> () const
        { return &getNode(); }

    T * operator-> ()
        { return &getNode(); }

    // http://stackoverflow.com/a/15418208/211160
    virtual ~RootNode () noexcept {
        if (static_cast<bool>(*this)) {
            unique_ptr<NodePrivate> nodeToFree (extractNodePrivate());
            nodeToFree.release();
        }
    }

    // Creation, e.g. RootNode<T>::create()
    // Notice that creation cannot be fit inside the Node "accessor"
    // because there's no way to automatically couple the right return type
    // in derived classes, you'd have to pass in a parameter.  I did this
    // with Node::createAs<T> before...which can work but is a bit
    // more confusing I think.  Time will tell.
public:
    static RootNode<T> create(Tag const & tag) {
        // REVIEW: New material that is not inserted into the graph should not
        // require a context, as if it is observed it will be thrown away.
        // It should only take on an observation group *if* it becomes inserted
        // somewhere persistently where it might be seen again
        return RootNode<T>(
            std::move(NodePrivate::create(tag)),
            make_shared<Context>(Observer::create(HERE))
        );
    }

    static RootNode<T> createText(QString const & str) {
        // REVIEW: New material that is not inserted into the graph should not
        // require a context, as if it is observed it will be thrown away.
        // It should only take on an observation group *if* it becomes inserted
        // somewhere persistently where it might be seen again
        return RootNode<T>(
            std::move(NodePrivate::createText(str)),
            make_shared<Context>(Observer::create(HERE))
        );
    }
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class ROOTNODE_no_moc_warning : public QObject { Q_OBJECT };

// Consider other operators
template <class T1, class T2>
bool operator==(RootNode<T1> const & x, RootNode<T2> const & y) {
    return x.get() == y.get();
}

}

#endif // METHYL_ROOTNODE_H
