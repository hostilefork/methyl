//
// context.h
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

#ifndef METHYL_CONTEXT_H
#define METHYL_CONTEXT_H

#include "methyl/observer.h"

// Every NodeRef or Tree may optionally contain a shared pointer to
// a Context object.  That Context pointer will be propagated into any
// Accessorhandles which are navigated to by means of that node.
//
// A derived class from an Context can encode arbitrary information that
// you may extract using the Methyl Engine.  But there are two other
// functions that a context performs.
//
// One is to tell you if the node is still "valid".  This validity check
// can be used to purposefully throw an exception instead of reading
// possibly reallocated memory.
//
// The second is to determine whether a read operation on a certain node
// should count as a registration of an observation on it, for a specific
// Observer.
//
// By default, methyl does not put any Context in a newly created node.
// You can change this by providing the Methyl Engine with a context
// factory object.

namespace methyl {

class Accessor;
class Engine;
template<class> class Tree;

class Context {

friend class Accessor;
private:
    codeplace _whereConstructed;


    //
    // When a Tree creation is requested, there is no context to copy from
    // (as there is when getting a NodeRef from an existing node).  Since
    // Context is something produced by the methyl client, the Engine has
    // to offer a hook to either make the object (or give a shared pointer
    // to one that already exists).  Due to the header file dependencies,
    // it's not currently possible to #include "methyl/engine.h" in the
    // node header files to find the create function...so contact with the
    // engine is offered through this hook routine implemented in context.cpp
    //
friend class Engine;
template<class> friend class Tree;
private:
    static shared_ptr<Context> create();

    static shared_ptr<Context> lookup();

public:
    Context (codeplace const & whereConstructed)
    {
    }

    virtual bool isValid () const {
        return true;
    }


    virtual ~Context () {
    }
};


// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class CONTEXT_no_moc_warning : public QObject { Q_OBJECT };

}

Q_DECLARE_METATYPE(shared_ptr<methyl::Context>)

#endif // METHYL_CONTEXT_H
