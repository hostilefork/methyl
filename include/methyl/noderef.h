//
// noderef.h
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

#ifndef METHYL_NODEREF_H
#define METHYL_NODEREF_H

#include "methyl/nodeprivate.h"
#include "methyl/context.h"

namespace methyl {

//
// The Node Reference wrapper.  Client code always works with these
// instead of with a NodePrivate itself, because a NodePrivate Reference
// contains a shared pointer to the context providing permissions and info on
// the node.
//

template <class> struct structure_equal_to;

template <class> struct structure_hash;


///
/// NodeRef for const case, -> returns a const Node *
///

template <class T>
class NodeRef<T const> {
    static_assert(
        std::is_base_of<Node, typename std::remove_const<T>::type>::value,
        "NodeRef<> may only be parameterized with a class derived from Node"
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

template <class> friend struct ::std::hash;
template <class> friend struct ::methyl::structure_equal_to;
template <class> friend struct ::methyl::structure_hash;

protected:
    // if const, assignment would be ill formed.  But we don't want to
    // accidentally invoke any non-const methods, as the node pointer we
    // passed in was const.
    T _nodeDoNotUseDirectly;

friend class Node;
friend class Engine;
template <class> friend class RootNode;
template <class> friend class NodeRef;

private:
    NodeRef () = delete;

private:
    template <class> friend class NodeRef;
    T const & getNode() const
        { return _nodeDoNotUseDirectly; }

private:
    explicit NodeRef (
        NodePrivate const & nodePrivate,
        shared_ptr<Context> context
    ) {
        _nodeDoNotUseDirectly.setInternalProperties(&nodePrivate, context);
    }
    explicit NodeRef (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> context
    ) {
        _nodeDoNotUseDirectly.setInternalProperties(nodePrivate, context);
    }

public:
    template <class U>
    NodeRef (
        NodeRef<U const> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other->getNodePrivate(),
            other.getNode().getContext()
        )
    {
    }

    template <class U>
    NodeRef (
        NodeRef<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other->getNodePrivate(),
            other.getNode().getContext()
        )
    {
    }

    template <class U>
    explicit NodeRef (
        NodeRef<U const> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other->getNodePrivate(),
            other.getNode().getContext()
        )
    {
    }

    T const * operator->() const {
        return &getNode();
    }

    virtual ~NodeRef () {
    }

public:
    template <class U>
    bool operator== (NodeRef<U const> const & other) const {
        return getNode().getNodePrivate() == other.getNode().getNodePrivate();
    }

    template <class U>
    bool operator!= (NodeRef<U const> const & other) const {
        return getNode().getNodePrivate() != other.getNode().getNodePrivate();
    }

public:
    template <class U>
    NodeRef<T> nonConst(
        NodeRef<U> const & mutableNeighbor,
        typename std::enable_if<
            not std::is_const<U>::value, void *
        >::type = nullptr
    ) const {
        // applies if could have gotten the privileges by walking other's tree
        //
        // http://blog.hostilefork.com/when-should-one-use-const-cast/

        NodePrivate const * thisRoot = &getNode().getNodePrivate();
        while (thisRoot->hasParent()) {
            thisRoot = &thisRoot->getParent(HERE);
        }
        NodePrivate * mutableRoot = &mutableNeighbor.getNode().getNodePrivate();
        while (mutableRoot->hasParent()) {
            mutableRoot = &mutableRoot->getParent(HERE);
        }
        hopefully(mutableRoot == thisRoot, HERE);

        return NodeRef<T> (
            const_cast<NodePrivate &>(getNode().getNodePrivate()),
            getNode().getContext()
        );
    }

public:
    RootNode<T> makeCloneOfSubtree() const {
        return RootNode<T> (
            std::move(getNode().getNodePrivate().makeCloneOfSubtree()),
            getNode().getContext()
        );
    }

public:
    template <class U>
    bool isSubtreeCongruentTo(NodeRef<U const> const & other) const {
        return getNode().getNodePrivate().isSubtreeCongruentTo(
            other.getNode().getNodePrivate()
        );
    }

public:
    static optional<NodeRef<T>> maybeGetFromId(Identity const & id) {
        NodePrivate const * nodePrivate = NodePrivate::maybeGetFromId(id);
        if (not nodePrivate) {
            return nullopt;
        }
        // If you don't want it to be run as "checked", then just pass in
        // Node as the type you're asking for.  Review a better way of doing
        // this...
        return NodeRef<T>::checked(NodeRef<T const> (
            nodePrivate,
            make_shared<Context> (Observer::create(HERE))
        ));
    }
};



// NodeRef for non-const case, -> returns a non-const Node *
template <class T = Node const>
class NodeRef : public NodeRef<T const> {

friend class Node;
friend class Engine;
template <class> friend class RootNode;
template <class> friend class NodeRef;

template <class> friend struct ::std::hash;
template <class> friend struct ::methyl::structure_equal_to;
template <class> friend struct ::methyl::structure_hash;

private:
    T & getNode() const {
        return const_cast<T &>(
            NodeRef<T const>::_nodeDoNotUseDirectly
        );
    }

private:
    NodeRef () = delete;
    NodeRef (
        NodePrivate & node,
        std::shared_ptr<Context> context
    ) : NodeRef<T const> (
        node,
        context
    ) {

    }

public:
    template <class U>
    NodeRef (
        NodeRef<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other->getNodePrivate(),
            other->getContext()
        )
    {
    }

    template <class U>
    NodeRef (
        RootNode<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other.getNode().getNodePrivate(),
            other.getNode().getContext()
        )
    {
    }

    template <class U>
    explicit NodeRef (
        NodeRef<U> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other->getNodePrivate(),
            other.getNode().getContext()
        )
    {
    }

    template <class U>
    explicit NodeRef (
        RootNode<U> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        NodeRef<T const> (
            other.getNodeRef().getNodePrivate(),
            other.getNodeRef().getContext()
        )
    {
    }

public:
    T * operator-> () const { return &getNode(); }
    virtual ~NodeRef () { }

public:
    // detach from parent
    RootNode<T> detach() {
        Label label;
        NodePrivate const * nodeParent;
        NodePrivate const * previousChild;
        NodePrivate const * nextChild;

        unique_ptr<NodePrivate> result = getNode().getNodePrivate().detach(
            &label,
            &nodeParent,
            &previousChild,
            &nextChild
        );
        getNode().getObserver()->detach(
            *result, *nodeParent, nullptr, previousChild, nextChild
        );

        return RootNode<T> (std::move(result), getNode().getContext());
    }

    template <class U>
    RootNode<T> replaceWith(RootNode<U> other) {
        Label label;
        NodePrivate const * nodeParent;
        NodePrivate const * previousChild;
        NodePrivate const * nextChild;

        auto otherPrivateOwned = std::move(other.extractNodePrivate());
        NodePrivate * otherPrivate = otherPrivateOwned.get();
        auto thisNodePrivate = getNode().getNodePrivate().replaceWith(
            std::move(otherPrivateOwned),
            &label,
            &nodeParent,
            &previousChild,
            &nextChild
        );

        getNode().getObserver()->detach(
            *thisNodePrivate,
            *nodeParent,
            previousChild,
            nextChild,
            otherPrivate
        );

        return RootNode<T> (
            std::move(thisNodePrivate),
            getNode().getContext()
        );
    }

    ///
    /// Structural checked casting assistance
    ///
    /// The only way we can call the most derived function is by
    /// creating an instance of the target accessor class and running
    /// the test on that.  For chaining we make it easy to take optionals
    /// It will not cast across branches in the Node class tree; just
    /// straight up and down cast.
    ///
    /// https://github.com/hostilefork/methyl/issues/24
    ///

public:
    template <class U>
    static optional<NodeRef<T const>> checked(
        NodeRef<U const> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        NodeRef<T const> test (source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<NodeRef<T const>> checked(
        optional<NodeRef<U const>> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        if (not source) {
            return nullopt;
        }

        NodeRef<T const> test (*source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<NodeRef<T>> checked(
        NodeRef<U> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        NodeRef<T> test (source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<NodeRef<T>> checked(
        optional<NodeRef<U>> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        if (not source) {
            return nullopt;
        }

        NodeRef<T> test (*source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class NODEREF_no_moc_warning : public QObject { Q_OBJECT };

}

#endif // NODEREF_H
