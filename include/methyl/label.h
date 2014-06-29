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

#ifndef METHYL_LABEL_H
#define METHYL_LABEL_H

#include "methyl/tag.h"

namespace methyl {

class NodePrivate;

//
// Label
// 
// For the moment, the demands on labels appear to be essentially the same
// as on tags.  The main distinction seems to be that they are less likely
// to be looked up as a Node.
//
// In case it turns out that there needs to be a distinction, they are
// kept as different types.
//

class Label : private Tag {

    friend class NodePrivate;
    using Tag::Tag;

public:
    using Tag::toUrl;

    using Tag::maybeGetAsIdentity;


    // Comparison operators need to take Label, and not Tag.  So we
    // cannot inherit them directly.

    bool operator== (Label const & rhs) const {
        return Tag::operator== (rhs);
    }

    bool operator!= (Label const & rhs) const {
        return Tag::operator!= (rhs);
    }

    bool operator< (Label const & rhs) const {
        return Tag::operator< (rhs);
    }
};

} // end namespace methyl

#endif
