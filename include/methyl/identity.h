//
// identity.h
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

#ifndef METHYL_IDENTITY_H
#define METHYL_IDENTITY_H

#include "methyl/defs.h"


namespace methyl {

//
// Identity
//
// I should research the flexibile notion of authorities and varied Identity
// that Freebase is using.  Yet even they give every object a "machine ID"
// it may just optionally have other identities.
//
// Because we're leveraging the string-based Qt DOM for now, it's more
// efficient to store it as the base 64 string being written there.
//

class Identity {

friend struct ::std::hash<Identity>;
private:
    QUuid _uuid;

public:
    explicit Identity (QUuid const & uuid) :
        _uuid (uuid)
    {
    }

    QUuid toUuid() const {
        return _uuid;
    }

    bool operator== (Identity const & rhs) const {
        return _uuid == rhs._uuid;
    }

    bool operator!= (Identity const & rhs) const {
        return _uuid != rhs._uuid;
    }

    bool operator< (Identity const & rhs) const {
        return _uuid < rhs._uuid;
    }
};

} // end namespace methyl


namespace std {

    // Need this to get std::unordered_set to work on Identity
    // http://stackoverflow.com/questions/8157937/

    template <>
    struct hash<methyl::Identity>
    {
        size_t operator()(
            methyl::Identity const & id
        ) const
        {
            return qHash(id._uuid);
        }
    };

/*
    // Need this to get std::map to work on Identity
    // http://stackoverflow.com/a/10734231/211160

    template <class T>
    struct less<methyl::Node<T>>
    {
        bool operator()(
            methyl::Identity const & left,
            methyl::Identity const & right
        ) const
        {
            return left.getId() < right.getId();
        }
    };
*/

} // end namespace std

#endif // METHYL_IDENTITY_H
