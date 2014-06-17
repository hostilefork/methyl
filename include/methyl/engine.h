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
#include "node.h"
#include "observer.h"

#include <map>

namespace methyl {

// REVIEW: Other general classes to keep people from reinventing common
// needs?


// The methyl::Engine is responsible for managing the opening and
// closing of databases.  It holds the global state relevant to
// a methyl session.  There should be only one in effect at a
// time.
class Engine
{
friend class methyl::NodePrivate;

private:
    // Currently there is only one document in existence
    // Over the long term there will have to probably be support for more
    // Including scratch documents if they are memory mapped files
    QDomDocument _document;
    QMap<QUuid, methyl::NodePrivate *> _mapIdToNode;

public:
    void forAllObservers(std::function<void(methyl::Observer &)> fn) {
        QList<tracked<Observer *>> observerList = 
            methyl::Observer::_listManager.getList();
        for (tracked<Observer *> & observer : observerList) {
            fn(*observer.get());
        }
    }

    shared_ptr<Observer> makeObserver(codeplace const & cp) {
        return shared_ptr<Observer>(new Observer (cp));
    }

    NodeRef<Node const> monitoredNode(
        NodeRef<Node const> node,
        shared_ptr<Observer> observer
    ) {
        return NodeRef<Node const>(
            node.getNode().getNodePrivate(),
            make_shared<Context>(observer)
        );
    }

    // Is the complexity of an "unmonitored Node" worth it?
    // It's a performance gain, so probably.  Decks (Daemons) need monitored
    // nodes but any other briefer operation probably does not
    NodeRef<Node const> unmonitoredNode(
        NodeRef<Node const> node
    ) {
        // do this more efficiently?
        auto blindObserver = Observer::create(HERE);
        blindObserver->markBlind();
        return NodeRef<Node const>(
            node.getNode().getNodePrivate(),
            make_shared<Context>(blindObserver)
        );
    }

    template <class NodeType>
    optional<NodeRef<NodeType const>> reconstituteNode(
        NodePrivate const * nodePrivate,
        shared_ptr<Context> context
    ) {
        if (not nodePrivate)
            return nullopt;

        return NodeRef<NodeType const> (nodePrivate, context);
    }

    template <class NodeType>
    optional<RootNode<NodeType>> reconstituteRootNode(
        NodePrivate * nodePrivateOwned,
        shared_ptr<Context> context
    ) {
        if (not nodePrivateOwned)
            return nullopt;

        return RootNode<NodeType> (
            unique_ptr<NodePrivate> (nodePrivateOwned),
            context
        );
    }

    std::pair<NodePrivate const *, shared_ptr<Context>> dissectNode(
        optional<NodeRef<Node const>> node
    ) {
        if (not node)
            return std::pair<NodePrivate const *, shared_ptr<Context>> (
                nullptr, nullptr
            );

        return std::pair<NodePrivate const *, shared_ptr<Context>> (
            &(*node).getNode().getNodePrivate(),
            (*node).getNode().getContext()
        );
    }

    std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> dissectRootNode(
        optional<RootNode<Node>> node
    ) {
        if (not node)
            return std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> (
                nullptr, nullptr
            );

        return std::pair<unique_ptr<NodePrivate>, shared_ptr<Context>> (
            (*node).extractNodePrivate(),
            (*node).getNode().getContext()
        );
    }

    RootNode<Node> makeNodeWithId(
        methyl::Identity const & id,
        methyl::Tag const & tag,
        QString const & name
    ) {
        // make_unique doesn't work here due to this private constructor,
        // figure out better way of doing this
        auto nodeWithId = methyl::RootNode<methyl::Node> (
            unique_ptr<methyl::NodePrivate> (
                new methyl::NodePrivate (id, tag)),
                make_shared<Context> (makeObserver(HERE))
            );
        if (not name.isEmpty()) {
            nodeWithId->insertChildAsFirstInLabel(
                methyl::RootNode<Node>::createText(name),
                globalLabelName
            );
        }
        return nodeWithId;
    }

public:
    explicit Engine ();

    virtual ~Engine ();
};

extern Engine* globalEngine;

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class ENGINE_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace methyl

#endif
