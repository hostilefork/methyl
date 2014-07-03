//
// tag.h
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

#ifndef METHYL_TAG_H
#define METHYL_TAG_H

#include <QUrl>
#include <QUuid>

#include "methyl/defs.h"
#include "methyl/identity.h"


namespace methyl {

//
// Tag
//
// Previously a tag was just a Accessoridentity, because it was an important
// aspect of the design that nodes be able to point at each other (
// as a cell in a spreadsheet can use a formula by pointing to another
// cell address).
//
// However, this excluded the ability to just make up a unique invariant
// name for an element that did not have a formal existence as a Methyl
// node.  Since there is a W3C standards effort to create a notion of unique
// identity through the "URI", it seemed pragmatic to leverage that:
//
// https://en.wikipedia.org/wiki/Uniform_resource_identifier
//
// All Methyl tags may resolve to URIs... even tags which point to Methyl
// AccessorIDs, which are effectively UUIDs.  Those currently resolve to
// URN URIs as specified in IETF RFC #4122:
//
// http://www.ietf.org/rfc/rfc4122.txt
//
// Theoretically it could be useful to allow a node to act as a proxy
// for a URI identity.  But at present, there is no facility for resolving
// non-URN/UUID tags into Methyl nodes...as a Methyl node cannot have an
// arbitrary URI as an ID.
//

class Tag {

friend struct ::std::hash<Tag>;
private:
    optional<QString> _urlString;
    optional<QUuid> _uuid;

public:
    explicit Tag (
        QString const & urlString,
        QUrl::ParsingMode mode /* = QUrl::StrictMode */
    ) :
        _urlString (urlString),
        _uuid ()
    {
        // We need to check if this url is a special case of a UUID URN,
        // in which case we resolve it as an ID.  We don't output lowercase,
        // but we tolerate it on read:
        //
        // "Although schemes are case-insensitive, the canonical form is
        // lowercase and documents that specify schemes must do so with
        // lowercase letters."

        static QString const urnPrefix = "urn:uuid:";
        if (not urlString.startsWith(urnPrefix, Qt::CaseInsensitive))
            return;

        _uuid = QUuid (urlString.mid(
            urnPrefix.length(), urlString.length() - urnPrefix.length()
        ));
        hopefully(not (*_uuid).isNull(), HERE);

        _urlString = nullopt;
    }

    explicit Tag (QUuid const & uuid) :
        _urlString(),
        _uuid (uuid)
    {
    }

    Tag (Identity const & id) :
        Tag (id.toUuid())
    {
    }

    // Temporary hack; allow creation of tags with just a file and line #
    explicit Tag (codeplace const & cp) :
        Tag (cp.getUuid())
    {
    }


    // QUrl implicitly converts from string, and we don't want to do that;
    // especially if testing additional parameters for the constructor that
    // takes a string.

    static Tag fromUrl (QUrl const & url) {
        hopefully(url.isValid(), HERE);
        return Tag (url.toString(), QUrl::TolerantMode);
    }

    Tag (Tag const & other) = default;

    Tag (Tag && other) = default;

    Tag & operator= (Tag const & other) = default;

    Tag & operator= (Tag && other) = default;

public:
    QUrl toUrl () const {
        if (_uuid)
            return QUrl("urn:uuid:" + (*_uuid).toString());

        // Shouldn't need QUrl::StrictMode since we check isValid on
        // the construction of the Tag if it was a Url.
        return QUrl(*_urlString);
    }

    optional<Identity> maybeAsIdentity () const {
        if (not _uuid)
            return nullopt;

        return Identity (*_uuid);
    }


    bool operator== (Tag const & rhs) const {
        if (this->_uuid) {
            if (rhs._uuid)
                return *this->_uuid == *rhs._uuid;

            return false;
        }

        if (rhs._uuid)
            return false;

        return *this->_urlString == *rhs._urlString;
    }

    bool operator!= (Tag const & rhs) const {
        if (this->_uuid) {
            if (rhs._uuid)
                return *this->_uuid != *rhs._uuid;

            return true;
        }

        if (rhs._uuid)
            return true;

        return *this->_urlString != *rhs._urlString;
    }

    bool operator< (Tag const & rhs) const {
        if (this->_uuid) {
            if (rhs._uuid)
                return *this->_uuid < *rhs._uuid;

            return false;
        }

        if (rhs._uuid)
            return false;

        return *this->_urlString < *rhs._urlString;
    }

    int compare (Tag const & other) const {
        if (this->_uuid) {
            if (other._uuid) {
                if (*this->_uuid > *other._uuid)
                    return -1;

                if (*this->_uuid == *other._uuid)
                    return 0;

                return 1;
            }

            // rule is all UUIDs come after all URLs.
            return -1;
        }

        if (other._uuid)
            return 1;

        return (*this->_urlString).compare(*other._urlString);
    }
};

} // end namespace methyl


namespace std {

    // Need this to get std::unordered_set to work on Identity
    // http://stackoverflow.com/questions/8157937/

    template <>
    struct hash<methyl::Tag>
    {
        size_t operator()(
            methyl::Tag const & tag
        ) const
        {
            if (tag._uuid)
                return qHash(*tag._uuid);

            return qHash(*tag._urlString);
        }
    };
} // end namespace std


#endif
