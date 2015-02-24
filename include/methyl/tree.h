//
// tree.h
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
#include "methyl/node.h"
#include "methyl/observer.h"
#include "methyl/context.h"

namespace methyl {

///
/// OWNERSHIP RESPONSIBILITY ROOT NODE WRAPPER
///

template <class T = Accessor>
class Tree {
    static_assert(
        std::is_base_of<Accessor, T>::value,
        "Tree<> may only be parameterized with a class derived from Node"
    );

    static_assert(
        not std::is_const<T>::value,
        "Tree<> may not be parameterized by a const Accessoraccessor"
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

protected:
    T _nodeDoNotUseDirectly;

friend class Accessor;
friend class Engine;
template <class> friend class Tree;
template <class> friend class Node;

template <class> friend struct ::std::hash;

public:
    // We are able to copy Tree trees, so after the copy is complete the
    // semantic needs to be that those trees are equal.  If you really want to
    // check to see that the root of the tree is the same node reference, then
    // you need to use x.get() == y.get().

    template <class U>
    bool operator== (Tree<U> const & other) const {
        return (*this)->isSubtreeCongruentTo(other.root());
    }

    template <class U>
    bool operator!= (Tree<U> const & other) const {
        return not (*this)->isSubtreeCongruentTo(other.root());
    }

    // Need this to get std::map to work on Tree
    // http://stackoverflow.com/a/10734231/211160
    template <class U>
    bool operator< (methyl::Tree<U> const & other) const {
        return (*this)->lowerStructureThan(other.node());
    }

private:
    T & accessor() {
        return _nodeDoNotUseDirectly;
    }

    T const & accessor() const {
        return _nodeDoNotUseDirectly;
    }


private:
    unique_ptr<NodePrivate> extractNodePrivate() {
        NodePrivate & node = accessor().nodePrivate();
        // REVIEW: We keep the context alive so constructors can extract it out
        // but is this the best way of doing it?
        shared_ptr<Context> context = accessor().context();
        accessor().setInternalProperties(nullptr, context);
        return unique_ptr<NodePrivate> (&node);
    }


public:
    explicit operator bool () const {
        return accessor().maybeNodePrivate() != nullptr;
    }

    Node<T const> root () const {
        Q_ASSERT(static_cast<bool>(*this));
        hopefully(not accessor().nodePrivate().hasParent(), HERE);
        return Node<T const>(
            accessor().nodePrivate(),
            accessor().context()
        );
    }

    Node<T> root () {
        Q_ASSERT(static_cast<bool>(*this));
        hopefully(not accessor().nodePrivate().hasParent(), HERE);
        return Node<T>(
            accessor().nodePrivate(),
            accessor().context()
        );
    }


public:
    // disable default construction.  If you need a Tree that can be
    // initialized to no value, then use optional<Tree<...>> and
    // start it out at nullopt.

    Tree () = delete;

    // Trees are copied and compared as values, though they may copy
    // large trees.  Be careful and pass by const & or use std::move.

    Tree & operator= (
        Tree const & other
    ) {
        if (this == &other) return *this;

        // Reset whatever node we might have been holding onto before.
        extractNodePrivate().reset();

        // Set internals to the result of duplicating the other's content
        accessor().setInternalProperties(
            other.accessor().nodePrivate().makeCloneOfSubtree().release(),
            Context::create()
        );

        return *this;
    }

    template <class U>
    Tree & operator= (
        Tree<U> const & other
    ) {
        static_assert(
            std::is_base_of<T, U>::value,
            "Can't do copy-assign of Tree unless base types compatible"
        );

        if (this == &other) return *this;

        // Reset whatever node we might have been holding onto before.
        extractNodePrivate().reset();

        // Set internals to the result of duplicating the other's content
        accessor().setInternalProperties(
            other.accessor().nodePrivate().makeCloneOfSubtree(),
            Context::create()
        );

        return *this;
    }

    template <class U>
    Tree & operator= (
        Tree<U> && other
    ) {
        static_assert(
            std::is_base_of<T, U>::value,
            "Can't do move-assign of Tree unless base types compatible"
        );

        if (this == &other) return *this;

        return Tree (
            std::move(other.extractNodePrivate()),
            std::move(other.accessor().context())
        );
    }

    void operator= (std::nullptr_t) {
        unique_ptr<NodePrivate> thisAccessor(extractNodePrivate());

        // REVIEW: Go through and mimic what unique_ptr does, and if use of
        // nullptr_t is a good idea (same old concept)

        return;
    }

    Tree & operator= (Tree && other) {
        unique_ptr<NodePrivate> otherNode = other.extractNodePrivate();
        unique_ptr<NodePrivate> thisNode = extractNodePrivate();
        accessor().setInternalProperties(
            otherNode.release(), other.accessor().context()
        );
        return *this;
    }


    // Templated case won't make up for an implicitly deleted copy constructor
    // ... have to explicitly make one.

    Tree (
        Tree const & other
    ) :
        Tree (
            other.accessor().nodePrivate().makeCloneOfSubtree(),
            Context::create()
        )
    {
    }

    // Only allow implicit casts if the accessor is going toward a base
    template <class U>
    Tree (
        Tree<U> && other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Tree (
            std::move(other.extractNodePrivate()),
            std::move(other.accessor().context())
        )
    {
    }
    template <class U>
    Tree (
        Tree<U> const & other,
        typename std::enable_if<
            std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) :
        Tree (
            other.accessor().nodePrivate().makeCloneOfSubtree(),
            Context::create()
        )
    {
    }


    template <class U>
    explicit Tree (
        Tree<U> && other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) noexcept :
        Tree (
            std::move(other.extractNodePrivate()),
            std::move(other.accessor().context())
        )
    {
    }
    template <class U>
    explicit Tree (
        Tree<U> const & other,
        typename std::enable_if<
            not std::is_base_of<T, U>::value,
            void *
        >::type = nullptr
    ) noexcept :
        Tree (
            other.accessor().nodePrivate().makeCloneOfSubtree(),
            Context::create()
        )
    {
    }

protected:
    Tree (
        unique_ptr<NodePrivate> nodePrivate,
        shared_ptr<Context> context
    ) {
        accessor().setInternalProperties(nodePrivate.release(), context);
    }

public:
    T const * operator-> () const {
        return &accessor();
    }

    T * operator-> () {
        return &accessor();
    }

    // http://stackoverflow.com/a/15418208/211160
    virtual ~Tree () {
        if (static_cast<bool>(*this)) {
            unique_ptr<NodePrivate> nodeToFree = extractNodePrivate();
            nodeToFree.reset();
        }
    }

    // Notice that creation cannot be fit inside the Accessor
    // because there's no way to automatically couple the right return type
    // in derived classes.  You'd have to pass in a parameter.  I did this
    // with Node::createAs<T> before...which can work but is a bit
    // more confusing I think.
public:
    static Tree<T> createWithTag (Tag const & tag) {
        return Tree<T>(
            std::move(NodePrivate::createWithTag(tag)),
            Context::create()
        );
    }

    static Tree<T> createAsText (QString const & str) {
        return Tree<T>(
            std::move(NodePrivate::createAsText(str)),
            Context::create()
        );
    }
};

}

#endif // METHYL_ROOTNODE_H
