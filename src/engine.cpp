//
// engine.cpp
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

#include "methyl/engine.h"

namespace methyl {

Label const globalLabelName (HERE);

//
// ERROR
//
methyl::Tag const globalTagError (HERE);
methyl::Tag const globalTagCancellation (HERE);
methyl::Label const globalLabelCausedBy (HERE);

methyl::Tree<Error> Error::makeCancellation() {
    return methyl::Tree<Error>::create(globalTagCancellation);
}


bool Error::wasCausedByCancellation () const {
    auto current = make_optional(thisNodeAs<Error const>());
    do {
        if ((*current)->hasTagEqualTo(globalTagCancellation)) {
            // should be terminal.  TODO: but what about comments?
            hopefully(not (*current)->hasAnyLabels(), HERE);
            return true;
        }
        if ((*current)->hasLabel(globalLabelCausedBy)) {
            current = (*current)->getFirstChildInLabel<Error>(
                globalLabelCausedBy, HERE
            );
        } else {
            current = nullopt;
        }
    } while (current);
    return false;
}


QString Error::getDescription () const
{
    QString result;
    auto tagNode = maybeGetTagNode();
    if (tagNode and (*tagNode)->hasLabel(methyl::globalLabelName)) {
        auto nameNode = (*tagNode)->getFirstChildInLabel(
            methyl::globalLabelName, HERE
        );
        result = nameNode->getText(HERE);
        // caused by?  how to present...
    } else {
        result = QString("Error Code URI: ") + getTag(HERE).toUrl().toString();
    }
    return result;
}


Engine::Engine () :
    Engine (
        []() -> shared_ptr<Context> {
            static shared_ptr<Context> dummy = make_shared<Context>(HERE);
            return dummy;
        },
        []() -> shared_ptr<Observer> {
            return nullptr;
        }
    )
{
}


Engine::Engine (
    ContextGetter const & contextGetter,
    ObserverGetter const & observerGetter
) :
    _contextGetter (contextGetter),
    _observerGetter (observerGetter)
{
    hopefully(globalEngine == nullptr, HERE);
    globalEngine = this;

    std::unordered_set<Node<Accessor const>> dummyWatchedRoots;
    _dummyObserver = Observer::create(dummyWatchedRoots, HERE);
    _dummyObserver->markBlind();

    qRegisterMetaType<optional<Node<Accessor const>>>(
        "optional<Node<Accessor const>>>"
    );

    qRegisterMetaType<optional<Node<Accessor>>>(
        "optional<Node<Accessor>>>"
    );
}

Observer & Engine::observerInEffect () {
    shared_ptr<Observer> observer = _observerGetter();
    if (observer)
        return *observer;
    return *_dummyObserver;
}

shared_ptr<Context> Engine::contextForCreate() {
    // Should these be different?  Parameterized?
    return _contextGetter();
}


shared_ptr<Context> Engine::contextForLookup() {
    // Should these be different?  Parameterized?
    return _contextGetter();
}


Tree<Accessor> Engine::makeNodeWithId (
    methyl::Identity const & id,
    methyl::Tag const & tag,
    optional<QString const &> name
) {
    auto nodeWithId = Tree<Accessor> (
        // cannot use make_unique here; private constructor
        unique_ptr<NodePrivate> (new NodePrivate (id, tag)),
        Context::contextForCreate()
    );

    if (name) {
        nodeWithId->insertChildAsFirstInLabel(
            Tree<Accessor>::createText(*name),
            globalLabelName
        );
    }
    return nodeWithId;
}


Engine::~Engine () {
    // Have to clean up any engine objects (nodes, observers, etc.) that
    // we allocated ourself before shutting down...
    _dummyObserver.reset();

    int size = _mapIdToNode.size();
    hopefully(size == 0, QString::number(size) + "nodes leaked", HERE);

    hopefully(globalEngine == this, HERE);
    globalEngine = nullptr;
}


Engine* globalEngine = nullptr;

} // end namespace methyl
