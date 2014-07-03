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
Tag const globalTagError (HERE);
Tag const globalTagCancellation (HERE);
Label const globalLabelCausedBy (HERE);
Label const globalLabelDescription (HERE);


Tree<Error> Error::create (
    Tree<> && description
) {
    auto result = Tree<Error>::createWithTag(globalTagError);
    result->insertChildAsFirstInLabel(
        std::move(description), globalLabelDescription
    );
    return result;
}


Tree<Error> Error::create (
    Tree<> && description,
    Tree<Error> && causedBy
) {
    auto result = Tree<Error>::createWithTag(globalTagError);
    result->insertChildAsFirstInLabel(
        std::move(description), globalLabelDescription
    );
    result->insertChildAsFirstInLabel(
        std::move(causedBy), globalLabelCausedBy
    );
    return result;
}


Tree<Error> Error::makeCancellation() {
    return Error::create(Tree<>::createWithTag(globalTagCancellation));
}


bool Error::wasCausedByCancellation () const {
    auto current = make_optional(thisNodeAs<Error>());
    do {
        auto description
            = (*current)->firstChildInLabel(globalLabelDescription, HERE);

        if (description->hasTagEqualTo(globalTagCancellation)) {
            // should be terminal.  TODO: but what about comments?
            hopefully(not description->hasAnyLabels(), HERE);
            return true;
        }

        current = (*current)->maybeFirstChildInLabel<Error>(
            globalLabelCausedBy
        );
    } while (current);
    return false;
}


QString Error::getDescription () const
{
    // Nothing fancy yet.  URL of each error in the causedBy chain.

    auto current = make_optional(thisNodeAs<Error>());
    auto description
        = (*current)->firstChildInLabel(globalLabelDescription, HERE);

    QString result = QString("Error: ")
        + (*current)->tag(HERE).toUrl().toString();

    auto causedBy = (*current)->maybeFirstChildInLabel<Error>(
        globalLabelCausedBy
    );

    if (causedBy) {
        return result + " caused by " + (*causedBy)->getDescription();
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
        Context::create()
    );

    if (name) {
        nodeWithId->insertChildAsFirstInLabel(
            Tree<Accessor>::createAsText(*name),
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
