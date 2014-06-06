//
// Context.h
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

#include "methyl/Observer.h"

namespace methyl {

class Context {
private:
    shared_ptr<Observer> _observer;
    bool _expired;

public:
    Context(shared_ptr<Observer> observer) :
        _observer (observer),
        _expired (false)
    {

    }

    shared_ptr<Observer> getObserver() {
        return _observer;
    }

    bool isExpired() const {
        return _expired;
    }

    // emit a signal of some kind, here?
    void makeExpired() {
        _expired = true;
    }

    virtual ~Context() {};
};

typedef shared_ptr<Context> contextSharedPointer;

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class CONTEXT_no_moc_warning : public QObject { Q_OBJECT };

}

Q_DECLARE_METATYPE(methyl::contextSharedPointer)

#endif // METHYL_CONTEXT_H
