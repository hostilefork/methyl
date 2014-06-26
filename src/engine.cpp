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

methyl::RootNode<Error> Error::makeCancellation()
{
    return methyl::RootNode<Error>::create(globalTagCancellation);
}

bool Error::wasCausedByCancellation() const
{
    auto current = make_optional(thisNodeAs<Error const>());
    do {
        if ((*current)->hasTagEqualTo(globalTagCancellation)) {
            // should be terminal.  TODO: but what about comments?
            hopefully(not (*current)->hasAnyLabels(), HERE);
            return true;
        }
        if ((*current)->hasLabel(globalLabelCausedBy)) {
            current = (*current)->getFirstChildInLabel<Error>(globalLabelCausedBy, HERE);
        } else {
            current = nullopt;
        }
    } while (current);
    return false;
}

QString Error::getDescription() const
{
    QString result;
    auto tagNode = maybeGetTagNode();
    if (tagNode and (*tagNode)->hasLabel(methyl::globalLabelName)) {
        auto nameNode = (*tagNode)->getFirstChildInLabel(methyl::globalLabelName, HERE);
        result = nameNode->getText(HERE);
        // caused by?  how to present...
    } else {
        result = QString("Error Code ID: ") + getTag(HERE).toBase64();
    }
    return result;
}


Engine::Engine() :
    _document (),
    _mapIdBase64ToNode ()
{
    hopefully(globalEngine == nullptr, HERE);
    globalEngine = this;

    qRegisterMetaType<optional<NodeRef<Node const>>>("optional<NodeRef<Node const>>>");
    qRegisterMetaType<optional<NodeRef<Node>>>("optional<NodeRef<Node>>>");
}

Engine::~Engine()
{
    hopefully(globalEngine == this, HERE);
    globalEngine = nullptr;
    hopefully(_mapIdBase64ToNode.empty(), HERE);
}

Engine* globalEngine = nullptr;

} // end namespace methyl
