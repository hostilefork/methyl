//
// node.h
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

#ifndef METHYL_NODE_H
#define METHYL_NODE_H

#include "methyl/nodeprivate.h"
#include "methyl/context.h"
#include "methyl/observer.h"

namespace methyl {

//
// The Node Reference wrapper.  Client code always works with these
// instead of with a NodePrivate itself, because a Node contains a shared
// pointer to the context providing permissions and info on the node,
// and may be abstracted across various implementations.
//


///
/// Node for const case, -> returns a const Accessor *
///

template <class T>
class Node<T const> {
    static_assert(
        std::is_base_of<Accessor, typename std::remove_const<T>::type>::value,
        "Node<> may only be parameterized with a class derived from Node"
    );

private:
    // Right now it's too labor intensive to forward declare all of Node,
    // so we just cook up something with the same footprint and make sure
    // the parameter hasn't added any data members or multiple inheritance
    class FakeAccessor {
        NodePrivate * _nodePrivate;
        shared_ptr<Context> _context;
        virtual ~FakeAccessor() {}
    };
    static_assert(
        sizeof(T) == sizeof(FakeAccessor),
        "Classes derived from Accessor must not add any new data members"
    );

template <class> friend struct ::std::hash;

protected:
    // if const, assignment would be ill formed.  But we don't want to
    // accidentally invoke any non-const methods, as the node pointer we
    // passed in was const.
    T _nodeDoNotUseDirectly;

friend class Observer;
friend class Accessor;
friend class Engine;
template <class> friend class Tree;
template <class> friend class Node;

private:
    Node () = delete;

private:
    template <class> friend class Node;
    T const & accessor() const
        { return _nodeDoNotUseDirectly; }

private:
    Node (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> const & context
    ) {
        _nodeDoNotUseDirectly.setInternalProperties(nodePrivate, context);
    }

    Node (
        NodePrivate const & nodePrivate,
        shared_ptr<Context> const & context
    )
        : Node (&nodePrivate, context)
    {
    }

    Node (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> && context
    ) {
        _nodeDoNotUseDirectly.setInternalProperties(nodePrivate, context);
    }

    Node (
        NodePrivate const & nodePrivate,
        shared_ptr<Context> && context
    )
        : Node (&nodePrivate, std::move(context))
    {
    }



public:

    // CONSTRUCTION FOR THE NODEREF<T CONST> CASE!

    // Allow implicit casting of Node<U> to Node<T const>
    // *if* T is a base of U (that includes the case that U is T), and
    // include the move optimization to avoid shared_ptr copy.  U
    // may be const or non-const.
    template <class U>
    Node (
        Node<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            other.accessor().context()
        )
    {
    }
    template <class U>
    Node (
        Node<U> && other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            std::move(other.accessor().context())
        )
    {
    }


    // Require EXPLICIT casting of Node<U const> to Node<T const>
    // *if* T is a *not* a base of U, and
    // include the move optimization to avoid shared_ptr copy
    template <class U>
    explicit Node (
        Node<U const> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            other.accessor().context()
        )
    {
    }
    template <class U>
    explicit Node (
        Node<U const> && other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            std::move(other.accessor().context())
        )
    {
    }


    T const * operator->() const {
        return &accessor();
    }

    virtual ~Node () {
    }

public:
    template <class U>
    bool operator== (Node<U const> const & other) const {
        return accessor().nodePrivate() == other.accessor().nodePrivate();
    }

    template <class U>
    bool operator!= (Node<U const> const & other) const {
        return accessor().nodePrivate() != other.accessor().nodePrivate();
    }

    // Need this to get std::map to work on NodeRef
    // http://stackoverflow.com/a/10734231/211160

    template <class U>
    bool operator< (methyl::Node<U> const & other) const {
        return this->identity() < other.identity();
    }

public:
    template <class U>
    Node<T> nonConst(
        Node<U> const & mutableNeighbor,
        typename std::enable_if<
            not std::is_const<U>::value, void *
        >::type = nullptr
    ) const {
        // applies if could have gotten the privileges by walking other's tree
        //
        // http://blog.hostilefork.com/when-should-one-use-const-cast/

        NodePrivate const * thisRoot = &accessor().nodePrivate();
        while (thisRoot->hasParent()) {
            thisRoot = &thisRoot->parent(HERE);
        }
        NodePrivate * mutableRoot = &mutableNeighbor.accessor().nodePrivate();
        while (mutableRoot->hasParent()) {
            mutableRoot = &mutableRoot->parent(HERE);
        }
        hopefully(mutableRoot == thisRoot, HERE);

        return Node<T> (
            const_cast<NodePrivate &>(accessor().nodePrivate()),
            accessor().context()
        );
    }

public:
    Tree<T> makeCloneOfSubtree() const {
        return Tree<T> (
            std::move(accessor().nodePrivate().makeCloneOfSubtree()),
            accessor().context()
        );
    }

public:
    template <class U>
    bool isSubtreeCongruentTo (Node<U const> const & other) const {
        return accessor().nodePrivate().isSubtreeCongruentTo(
            other.accessor().nodePrivate()
        );
    }

public:
    static optional<Node<T>> maybeLookupById (Identity const & id) {
        NodePrivate const * nodePrivate = NodePrivate::maybeGetFromId(id);
        if (not nodePrivate) {
            return nullopt;
        }
        // If you don't want it to be run as "checked", then just pass in
        // Accessoras the type you're asking for.  Review a better way of doing
        // this...
        return Node<T>::checked(Node<T const> (
            nodePrivate, Context::lookup()
        ));
    }
};



// Node for non-const case, -> returns a non-const Accessor*
template <class T = Accessor const>
class Node : public Node<T const> {

friend class Accessor;
friend class Engine;
template <class> friend class Tree;
template <class> friend class Node;

template <class> friend struct ::std::hash;

private:
    T & accessor () const {
        return const_cast<T &>(
            Node<T const>::_nodeDoNotUseDirectly
        );
    }

private:
    Node () = delete;

    Node (
        NodePrivate & node,
        std::shared_ptr<Context> const & context
    ) : Node<T const> (
        node,
        context
    ) {

    }

    Node (
        NodePrivate & node,
        std::shared_ptr<Context> && context
    ) : Node<T const> (
        node,
        std::move(context)
    ) {

    }


public:
    // CONSTRUCTION FOR THE NODEREF<T MUTABLE> CASE!

    // Allow implicit casting of Node<U> to Node<T>
    // *if* T is a base of U (that includes the case that U is T), and
    // include the move optimization to avoid shared_ptr copy.  U
    // must be non-const.
    template <class U>
    Node (
        Node<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value and not std::is_const<U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            other->context()
        )
    {
    }
    template <class U>
    Node (
        Node<U> && other,
        typename std::enable_if<
            std::is_base_of<T, U>::value and not std::is_const<U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            std::move(other->context())
        )
    {
    }


    // Allow implicit casting of Node<U> to Node<T>
    // *if* T is a base of U (that includes the case that U is T), and
    // include the move optimization to avoid shared_ptr copy.  Tree
    // does not allow const-node accessors; no need to check that.
    template <class U>
    Node (
        Tree<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other.accessor().nodePrivate(),
            other.accessor().context()
        )
    {
    }
    template <class U>
    Node (
        Tree<U> && other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other.accessor().nodePrivate(),
            std::move(other.accessor().context())
        )
    {
    }


    // Require EXPLICIT casting of Node<U> to Node<T>
    // *if* T is a *not* a base of U, and
    // include the move optimization to avoid shared_ptr copy.
    // U must be non-const
    template <class U>
    explicit Node (
        Node<U> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value and not std::is_const<U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            other.accessor().context()
        )
    {
    }
    template <class U>
    explicit Node (
        Node<U> && other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value and not std::is_const<U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other->nodePrivate(),
            std::move(other.accessor().context())
        )
    {
    }


    // Require EXPLICIT casting of Tree<U> to Node<T>
    // *if* T is a *not* a base of U, and
    // include the move optimization to avoid shared_ptr copy.
    // U must be non-const
    template <class U>
    explicit Node (
        Tree<U> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other.getNode().nodePrivate(),
            other.getNode().context()
        )
    {
    }
    template <class U>
    explicit Node (
        Tree<U> && other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Node<T const> (
            other.getNode().nodePrivate(),
            std::move(other.getNode().context())
        )
    {
    }


public:
    T * operator-> () const {
        return &accessor();
    }

    virtual ~Node () {
    }


public:
    // detach from parent
    Tree<T> detach() {

        auto result = accessor().nodePrivate().detach();

        unique_ptr<NodePrivate> & detachedNode = std::get<0>(result);
        NodePrivate::detach_info & info = std::get<1>(result);

        Observer::current().detach(
            *detachedNode,
            info._nodeParent,
            info._previousChild,
            info._nextChild,
            nullptr
        );

        return Tree<T> (std::move(detachedNode), accessor().context());
    }

    template <class U>
    Tree<T> replaceWith(Tree<U> other) {

        auto otherPrivateOwned = std::move(other.extractNodePrivate());
        NodePrivate & otherPrivate = *otherPrivateOwned.get();

        auto result = accessor().nodePrivate().replaceWith(
            std::move(otherPrivateOwned)
        );

        unique_ptr<NodePrivate> & detachedNode = std::get<0>(result);
        NodePrivate::detach_info & info = std::get<1>(result);

        Observer::current().detach(
            *detachedNode,
            info._nodeParent,
            info._previousChild,
            info._nextChild,
            &otherPrivate
        );

        return Tree<T> (std::move(detachedNode), accessor().context());
    }

    ///
    /// Structural checked casting assistance
    ///
    /// The only way we can call the most derived function is by
    /// creating an instance of the target accessor class and running
    /// the test on that.  For chaining we make it easy to take optionals
    /// It will not cast across branches in the Accessorclass tree; just
    /// straight up and down cast.
    ///
    /// https://github.com/hostilefork/methyl/issues/24
    ///

public:
    template <class U>
    static optional<Node<T const>> checked(
        Node<U const> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        Node<T const> test (source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<Node<T const>> checked(
        optional<Node<U const>> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        if (not source) {
            return nullopt;
        }

        Node<T const> test (*source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<Node<T>> checked(
        Node<U> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        Node<T> test (source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }

    template <class U>
    static optional<Node<T>> checked(
        optional<Node<U>> const & source,
        typename std::enable_if<
            std::is_base_of<T, U>::value
            or std::is_base_of<U, T>::value,
            void *
        >::type = nullptr
    ) {
        if (not source) {
            return nullopt;
        }

        Node<T> test (*source);
        if (not test->check()) {
            return nullopt;
        }

        return test;
    }
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class METHYL_NODE_no_moc_warning : public QObject { Q_OBJECT };

}

#endif // NODEREF_H
