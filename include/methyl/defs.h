//
// defs.h
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

#ifndef METHYL_DEFS_H
#define METHYL_DEFS_H

#include "hoist/hoist.h"
using namespace hoist;

#include <tuple>
using std::tuple;
using std::make_tuple;
using std::tie;

//
// TYPE CONTAINER FOR "OPTIONAL" VALUES
//
// See summary of the state of things at:
// https://github.com/hostilefork/CopyMoveConstrainedOptional
//
// For the moment I am expecting that the include search paths will be able
// to find the optional.hpp file under a directory called "Optional", which
// is the name and casing of the GitHub repository for the reference
// implementation.  I myself put it under the methyl/include/methyl
// directory, which is just for expedience (it's in the .gitignore).

#include "Optional/optional.hpp"
using std::experimental::optional;
using std::experimental::nullopt;
using std::experimental::make_optional;


//
// I have some functions that retrieve the QString off of a data node, and
// want to turn that into a uint or an int.  For a safe handling of that, I
// have these routines, which probably don't belong here.  But they're here
// for now.
//
inline optional<uint> maybeGetAsUInt(QString const & str) {
    bool ok;
    uint const value = str.toUInt(&ok);
    if (not ok)
        return nullopt;
    return optional<uint>(value);
}

inline optional<int> maybeGetAsInt(QString const & str) {
    bool ok;
    int const value = str.toInt(&ok);
    if (not ok)
        return nullopt;
    return optional<int>(value);
}


// I hate it when you have to test against "magic values" for things like -1,
// so if you have a function that gives a result like that then these will let
// you convert them into optional values of that type from the result

template <typename T>
optional<T> maybeIf(bool (*resultTestFunc)(T& result), T result) {
    if (resultTestFunc(result)) {
        return result;
    }
    return nullopt;
}

template <typename T>
optional<T> maybeIfNotValue(T const & value, T result) {
    if (result != value) {
        return result;
    }
    return nullopt;
}


//
// SMART POINTERS
//
// As with optional, I use these so much that I am willing to do the 
// "bad thing" and not include the namespace in the header files.
//

#include <memory>
using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;

// make_unique is in C++14, which I'm not using yet.  Accidentally left out of
// C++11 somehow.  (?)
//
//     http://stackoverflow.com/questions/7038357/
//
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Should probably be in hoist.  I don't know why it isn't.  Review.
inline void notImplemented(QString const & routine, codeplace const & cp) {
    hopefullyNotReached(QString("Not Implemented: ") + routine, cp);
}

namespace methyl {

// Identity
// I should research the flexibile notion of authorities and varied Identitys
// that Freebase is using.  Yet even they give every object a "machine ID"
// it may just optionally have other identities.
class Identity : protected QUuid {
public:
    Identity (QUuid const & uuid) : QUuid (uuid) {}
    explicit Identity (QString const & str) :
        QUuid (UuidFromBase64String(str.toLatin1()))
    {
    }
    QString toString() const {
        return Base64StringFromUuid(*this);
    }
    QUuid toUuid() const {
        return (*this);
    }
    int compare(Identity const & other) const {
        return this->toString().compare(other.toString());
    }
    bool operator== (Identity const & rhs) const {
        return compare(rhs) == 0;
    }
    bool operator!= (Identity const & rhs) const {
        return compare(rhs) != 0;
    }

};


// Should I go with QString to make this easier for the moment?
class Label : protected QUuid {
    friend class NodePrivate;
public:
    Label () : QUuid() {}
    Label (QUuid const & uuid) : QUuid (uuid) {}
    bool operator== (Label const & rhs) const {
        return (
            static_cast<QUuid const &>(*this)
            == static_cast<QUuid const &>(rhs)
        );
    }
    bool operator!= (Label const & rhs) const {
        return (
            static_cast<QUuid const &>(*this)
            != static_cast<QUuid const &>(rhs)
        );
    }
};

typedef Identity Tag; // so it goes....


///
/// Forward declarations
///

class Node;

// https://github.com/hostilefork/methyl/issues/2
template <class> class NodeRef;

// https://github.com/hostilefork/methyl/issues/2
template <class> class RootNode;

class Engine;

} // end namespace methyl

#endif // METHYL_DEFS_H
