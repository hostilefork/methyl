//
// engine.h
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

#ifndef METHYL_ENGINE_H
#define METHYL_ENGINE_H

#include "defs.h"
#include "accessor.h"
#include "observer.h"

#include <map>

namespace methyl {

typedef std::function<shared_ptr<Context>()> ContextGetter;

typedef std::function<shared_ptr<Observer>()> ObserverGetter;


// The methyl::Engine is responsible for managing the opening and
// closing of databases.  It holds the global state relevant to
// a methyl session.  There should be only one in effect at a
// time.
class Engine
{
private:
    // Currently there is only one document in existence
    // Over the long term there will have to probably be support for more
    // Including scratch documents if they are memory mapped files
private:
    friend class ::methyl::NodePrivate;
    QReadWriteLock _mapLock;
    std::unordered_map<methyl::Identity, methyl::NodePrivate *> _mapIdToNode;
    ContextGetter _contextGetter;
    ObserverGetter _observerGetter;
    shared_ptr<Observer> _dummyObserver;

private:
    friend class ::methyl::Observer;
    std::unordered_set<Observer *> _observers;
    QReadWriteLock _observersLock;

public:
    // You cannot destroy observers during the enumeration...
    void forAllObservers (std::function<void(methyl::Observer &)> fn) {
        QReadLocker lock (&_observersLock);
        for (Observer * observer : _observers) {
            fn(*observer);
        }
    }

    Observer & observerInEffect ();

    shared_ptr<Context> contextForCreate ();

    shared_ptr<Context> contextForLookup ();

    template <class T>
    Node<T> contextualNodeRef (
        Node<T> const & node,
        shared_ptr<Context> const & context
    ) {
        return Node<T> (node.accessor().nodePrivate(), context);
    }

    template <class T>
    Node<T> contextualNodeRef (
        Node<T> const & node,
        shared_ptr<Context> && context
    ) {
        return Node<T> (node.accessor().nodePrivate(), context);
    }

    template <class NodeType>
    optional<Node<NodeType const>> reconstituteNode(
        NodePrivate const * nodePrivate,
        shared_ptr<Context> context
    ) {
        if (not nodePrivate)
            return nullopt;

        return Node<NodeType const> (nodePrivate, context);
    }

    template <class NodeType>
    optional<Tree<NodeType>> reconstituteTree (
        NodePrivate * nodePrivateOwned,
        shared_ptr<Context> context
    ) {
        if (not nodePrivateOwned)
            return nullopt;

        return Tree<NodeType> (
            unique_ptr<NodePrivate> (nodePrivateOwned),
            context
        );
    }

    std::pair<NodePrivate const *, shared_ptr<Context>> dissectNode(
        optional<Node<Accessor const>> node
    ) {
        if (not node)
            return std::pair<NodePrivate const *, shared_ptr<Context>> (
                nullptr, nullptr
            );

        return std::pair<NodePrivate const *, shared_ptr<Context>> (
            &(*node).accessor().nodePrivate(),
            (*node).accessor().context()
        );
    }

    std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> dissectTree (
        optional<Tree<Accessor>> node
    ) {
        if (not node)
            return std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> (
                nullptr, nullptr
            );

        return std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> (
            (*node).extractNodePrivate(),
            (*node).accessor().context()
        );
    }

    Tree<Accessor> makeNodeWithId (
        methyl::Identity const & id,
        methyl::Tag const & tag,
        optional<QString const &> name
    );

public:
    explicit Engine ();

    explicit Engine (
        ContextGetter const & contextGetter,
        ObserverGetter const & observerGetter
    );

    virtual ~Engine ();
};

extern Engine* globalEngine;

} // end namespace methyl

#endif
