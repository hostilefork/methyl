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

Engine::Engine() :
    _document (),
    _mapIdToNode ()
{
	hopefully(globalEngine == NULL, HERE);
	globalEngine = this;

    qRegisterMetaType<optional<NodeRef<Node const>>>("optional<NodeRef<Node const>>>");
    qRegisterMetaType<optional<NodeRef<Node>>>("optional<NodeRef<Node>>>");
}

Engine::~Engine()
{
	hopefully(globalEngine == this, HERE);
	globalEngine = NULL;
	hopefully(_mapIdToNode.empty(), HERE);
}

Engine* globalEngine = NULL;

} // end namespace methyl
