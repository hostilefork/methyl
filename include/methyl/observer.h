//
// observer.h
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

#ifndef METHYL_OBSERVER_H
#define METHYL_OBSERVER_H

#include <unordered_map>
#include <unordered_set>
#include <type_traits>

#include "hoist/hoist.h"
#include "methyl/defs.h"
#include "methyl/identity.h"
#include "methyl/label.h"
#include "methyl/tag.h"

namespace methyl {

// The Observer class is included via shared_ptr into Node.h, and since
// it is included through a pointer it would be possible to #include
// "methyl/Node.h" here.  But for convenience of the moment, many of
// the observer methods are called directly from inline code inside the
// classes in that header.  Hence we cannot include "node.h".  These
// forward definitions are in order to  directly into the node.h
class NodePrivate;
class Accessor;
template <class> class Node;
template <class> class Tree;

// A goal is to try and get NodePrivate and all privileged operations
// implemented through the methyl::Engine.  We want to friend this class
// so that we don't have to explicitly name any specific client.  If a
// piece of functionality is needed either permanently or temporarily
// that involves privileged access to nodes, we have that class call
// a function in the Engine
class Engine;

//
// methyl::Observer records the observer you make.  If a
// change to the document happens such that any of the questions
// you asked *might* yield a different answer, it will raise
// a signal to let you know.  (For efficiency purposes, it may
// compact observation data to be coarser, such that false
// positives are possible.  Hence you might ask all the same
// questions and get all the same answers after an invalidation.)
//
// An observation label can be "blinded" at certain points so
// no more observations are made.  This is based on the
// circumstances under which you acquired the label, as well as
// what has happened since.  It is a separate question from the
// validity of a node handle--that is managed by the Context.
//
class Observer final : public QObject {

    Q_OBJECT

    friend class Engine;
    template <class> friend class Node;
    template <class> friend class Tree;

public:
    // simple list to start, will do more later
    enum class SeenFlags {
        None = 0, // Note: Needed as we can't convert to bool for testing
        HasTag = 1 << 0,
        Tag =  1 << 1,
        HasParent = 1 << 2,
        Parent = 1 << 3,
        LabelInParent = 1 << 4,
        HasLabel = 1 << 5,
        FirstChild = 1 << 6,
        LastChild = 1 << 7,
        HasNextSiblingInLabel = 1 << 8,
        NextSiblingInLabel = 1 << 9,
        HasPreviousSiblingInLabel = 1 << 10,
        PreviousSiblingInLabel = 1 << 11,
        Data = 1 << 12
    };

private:
    std::unordered_set<NodePrivate const *> _watchedRoots;
    QReadWriteLock mutable _mapLock;
    optional<std::unordered_map<NodePrivate const *, SeenFlags>> _map;


// protected constructor, make_shared can't call it...
// REVIEW: http://stackoverflow.com/a/8147326/211160
private:
    Observer (
        std::unordered_set<Node<Accessor const>> const & watchedRoots,
        codeplace const & cp
    );

    Observer (
        Node<Accessor const> const & watchedRoot,
        codeplace const & cp
    );


public:
    template <class... Args>
    static shared_ptr<Observer> create (Args &&... args) {
        // can't use make_shared when constructor is private
        return shared_ptr<Observer>(
            new Observer (std::forward<Args>(args)...)
        );
    }

    static Observer & observerInEffect ();

private:
    void markBlind() {
        {
            QWriteLocker lock (&_mapLock);
            _map = nullopt;
        }

        emit blinded();
    }

signals:
    void blinded();

public:
    bool isBlinded();


private:
    SeenFlags getSeenFlags (NodePrivate const & node) const;

    void addSeenFlags (
        NodePrivate const & node,
        SeenFlags const & flags,
        codeplace const & cp
    );

    // NOTE: This gets called several times in a row
    // better to capture the seen flags once... or cache... or something
    bool maybeObserved (
        methyl::NodePrivate const & node,
        SeenFlags const & flags
    );

public:
    // no effect, also we use this so it would create weird recursion...
    // consider also: if nodes can be moved between documents, might that
    // create an "invalidation"...or is getting a document sort of like
    // getting an ID, in that if anything were actually observed you'd
    // get that falling out of somewhere else?
#ifdef NODE_ID_CAN_BE_CHANGED
    void getId(
        Identity const & result,
        methyl::NodePrivate const & thisNode
    );
#endif

    void hasParent (
        bool const & result,
        methyl::NodePrivate const & thisNode
    );

    void getParent (
        methyl::NodePrivate const & result,
        methyl::NodePrivate const & thisNode
    );

    void getLabelInParent (
        methyl::Label const & result,
        methyl::NodePrivate const & thisNode
    );

    void hasParentEqualTo (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & parent
    );

    void hasLabelInParentEqualTo (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    // TAG SPECIFICATION

    void hasTag (
        bool const & result,
        methyl::NodePrivate const & thisNode
    );

    void hasTagEqualTo (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Tag const & tag
    );

    void getTag (
        methyl::Tag const & result,
        methyl::NodePrivate const & thisNode
    );

    void tryGetTagNode (
        const methyl::NodePrivate * result,
        methyl::NodePrivate const & thisNode
    );

    // LABEL ENUMERATION
    // no implicit ordering, invariant order from ID

    void hasAnyLabels (
        bool const & result,
        methyl::NodePrivate const & thisNode
    );

    void hasLabel (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void getFirstLabel (
        methyl::Label const & result,
        methyl::NodePrivate const & thisNode
    );

    void getLastLabel (
        methyl::Label const & result,
        methyl::NodePrivate const & thisNode
    );

    void hasLabelAfter (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void getLabelAfter (
        methyl::Label const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void hasLabelBefore (
        bool const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void getLabelBefore (
        methyl::Label const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    // NODE IN LABEL ENUMERATION

    void getFirstChildInLabel (
        methyl::NodePrivate const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void getLastChildInLabel (
        methyl::NodePrivate const & result,
        methyl::NodePrivate const & thisNode,
        methyl::Label const & label
    );

    void hasNextSiblingInLabel (
        bool const & result,
        methyl::NodePrivate const & thisNode
    );

    void getNextSiblingInLabel (
        methyl::NodePrivate const & result,
        methyl::NodePrivate const & thisNode
    );

    void hasPreviousSiblingInLabel (
        bool const & result,
        methyl::NodePrivate const & thisNode
    );

    void getPreviousSiblingInLabel (
        methyl::NodePrivate const & result,
        methyl::NodePrivate const & thisNode
    );

    void getText (
        const QString & result,
        methyl::NodePrivate const & thisNode
    );


public:
    static void setTag (
        methyl::NodePrivate const & thisNode,
        methyl::Tag const & tag
    );

    static void insertChildAsFirstInLabel (
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & newChild,
        methyl::Label const & label,
        methyl::NodePrivate const * nextChildInLabel
    );

    static void insertChildAsLastInLabel (
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & newChild,
        methyl::Label const & label,
        methyl::NodePrivate const * previousChildInLabel
    );

    static void insertChildBetween (
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & newChild,
        methyl::NodePrivate const & previousChild,
        methyl::NodePrivate const & nextChild
    );

#ifdef INSERT_CHILD_BEFORE_AND_AFTER_MICRO_OBSERVATIONS
    // use insertChildBetween
    void insertChildAfter(
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & newChild,
        methyl::NodePrivate const & refChild
    );
    void insertChildBefore(
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & newChild,
        methyl::NodePrivate const & refChild
    );
#endif

    static void detach (
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & parent,
        methyl::NodePrivate const * previousChild,
        methyl::NodePrivate const * nextChild,
        methyl::NodePrivate const * replacement);

#ifdef REPLACEWITH_MICRO_OBSERVATION
    // right now we do not differentiate between a replace and a detach
    void replaceWith(
        methyl::NodePrivate const & thisNode,
        methyl::NodePrivate const & nodeReplacement
    );
#endif


    // data modifications
public:
#ifdef STRING_MICRO_OBSERVATIONS
    // for now, inserts and deletes affect any observation made of a text node
    // in theory, a smaller modification could cause less invalidation if there
    // were corresponding smaller observations.
    void insertCharBeforeIndex (
        methyl::NodePrivate const & thisNode,
        IndexOf(QChar) index,
        const QChar& ch
    );

    void insertCharAfterIndex (
        methyl::NodePrivate const & thisNode,
        IndexOf(QChar) index,
        const QChar& ch
    );

    void deleteCharAtIndex (
        methyl::NodePrivate const & thisNode,
        IndexOf(QChar) index
    )
#endif
    static void setText (
        methyl::NodePrivate const & thisNode,
        QString const & data
    );

    virtual ~Observer();
};

}


// C++11 type-safe bitflag operations compatible with "enum class"
// (Qt's QFlags will not work with this feature.)
//
//    http://stackoverflow.com/a/4227264/211160
//    http://stackoverflow.com/a/8357462/211160
//
// Must test for == or != to SeenFlags::None as bool/int conversion is not
// possible for an enum class.

inline methyl::Observer::SeenFlags operator| (
    methyl::Observer::SeenFlags const & a,
    methyl::Observer::SeenFlags const & b
) {
    using SeenFlags = methyl::Observer::SeenFlags;
    typedef std::underlying_type<SeenFlags>::type ut;

    return static_cast<SeenFlags>(
        static_cast<ut>(a) | static_cast<ut>(b)
    );
}

inline methyl::Observer::SeenFlags operator& (
    methyl::Observer::SeenFlags const & a,
    methyl::Observer::SeenFlags const & b
) {
    using SeenFlags = methyl::Observer::SeenFlags;
    typedef std::underlying_type<SeenFlags>::type ut;

    return static_cast<SeenFlags>(
        static_cast<ut>(a) & static_cast<ut>(b)
    );
}

#endif // METHYL_OBSERVATIONS_H
