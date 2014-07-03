//
// observer.cpp
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

#include "methyl/observer.h"
#include "methyl/engine.h"

QDebug & operator<< (
    QDebug o,
    methyl::Observer::SeenFlags const flags
) {
    using SeenFlags = methyl::Observer::SeenFlags;

    o << "SeenFlags(";

    bool first = true;

    for (
        SeenFlags saw = SeenFlags::HasTag;
        saw <= SeenFlags::Data;
        saw = static_cast<SeenFlags>(
            static_cast<int>(saw) << 1
        )
    ) {
        if ((flags & saw) != SeenFlags::None) {
            switch (saw) {
            case SeenFlags::HasTag:
                o << "HasTag";
                break;
            case SeenFlags::Tag:
                o << "Parent";
                break;
            case SeenFlags::HasParent:
                o << "HasParent";
                break;
            case SeenFlags::Parent:
                o << "Parent";
                break;
            case SeenFlags::LabelInParent:
                o << "LabelInParent";
                break;
            case SeenFlags::HasLabel:
                o << "Label";
                break;
            case SeenFlags::FirstChild:
                o << "FirstChild";
                break;
            case SeenFlags::LastChild:
                o << "LastChild";
                break;
            case SeenFlags::HasNextSiblingInLabel:
                o << "HasNextSiblingInLabel";
                break;
            case SeenFlags::NextSiblingInLabel:
                o << "NextSiblingInLabel";
                break;
            case SeenFlags::HasPreviousSiblingInLabel:
                o << "HasPreviousSiblingInLabel";
                break;
            case SeenFlags::PreviousSiblingInLabel:
                o << "PreviousSiblingInLabel";
                break;
            case SeenFlags::Data:
                o << "Data";
                break;
            default:
                throw hopefullyNotReached(HERE);
            }

            if (not first) {
                o << "|";
            } else {
                first = false;
            }
        }
    }

    return o << ")";
}

namespace methyl {

tracked<bool> globalDebugObserver (true, HERE);


Observer & Observer::current () {
    return globalEngine->observerInEffect();
}


Observer::Observer (
    std::unordered_set<Node<Accessor const>> const & watchedRoots,
    codeplace const & cp
) :
    _map (std::unordered_map<NodePrivate const *, SeenFlags>())
{
    Q_UNUSED(cp);

    for (auto & root : watchedRoots) {
        // Can't call hasParent() here if no Observer is in effect (catch-22)
        // Have to reach underneath and use the NodePrivate function
        auto & rootPrivate = root.accessor().nodePrivate();
        hopefully(not rootPrivate.hasParent(), HERE);
        _watchedRoots.insert(&rootPrivate);
    }

    QWriteLocker lock (&globalEngine->_observersLock);
    globalEngine->_observers.insert(this);
}


Observer::Observer (
    Node<Accessor const> const & watchedRoot,
    codeplace const & cp
) :
    Observer (std::unordered_set<Node<Accessor const>>{watchedRoot}, cp)
{
}


//
// LOOKUP ROUTINES
// These check the observation map.
//

bool Observer::isBlinded() {
    QReadLocker lock (&_mapLock);
    return _map == nullopt;
}


Observer::SeenFlags Observer::getSeenFlags (NodePrivate const & node) const {
    QReadLocker lock (&_mapLock);

    hopefully(_map != nullopt, HERE);
    auto it = (*_map).find(&node);
    if (it == (*_map).end()) {
        return SeenFlags::None;
    }
    return it->second;
}


void Observer::addSeenFlags (
    NodePrivate const & node,
    SeenFlags const & flags,
    codeplace const & cp
) {
    Q_UNUSED(cp);

    QWriteLocker lock (&_mapLock);

    if (_map == nullopt)
        return;

    auto it = (*_map).find(&node);
    if (it == (*_map).end()) {
        (*_map).insert(
            std::pair<NodePrivate const *, SeenFlags>(&node, flags)
        );
    } else {
        it->second = it->second | flags;
    }
}


bool Observer::maybeObserved (
    methyl::NodePrivate const & node,
    SeenFlags const & flags
) {
    return (getSeenFlags(node) & flags) != SeenFlags::None;
}



//
// READ OPERATIONS
// Record the read as interesting only to the currently effective observer
//

// node in label enumeration
void Observer::firstChildInLabel (
    NodePrivate const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    // changes to first child in any label will invalidate
    addSeenFlags(thisNode, SeenFlags::FirstChild, HERE);
    // effectively, we have been told the result has no previous siblings and
    // its label in parent is label!  So no need for SeenFlags::LabelInParent or
    // SeenFlags::HasPreviousSiblingInLabel
}


void Observer::lastChildInLabel (
    NodePrivate const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    // changes to last child in any label will invalidate
    addSeenFlags(thisNode, SeenFlags::LastChild, HERE);
    // effectively, we have been told the result has no next siblings and
    // its label in parent is label!  So no need for SeenFlags::LabelInParent or
    // SeenFlags::HasNextSiblingInLabel
}


void Observer::hasParent (
    bool const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasParent, HERE);
}


void Observer::parent (
    NodePrivate const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::Parent, HERE);
}


void Observer::labelInParent (
    Label const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::LabelInParent, HERE);
}


void Observer::hasParentEqualTo (
    bool const & result,
    NodePrivate const & thisNode,
    NodePrivate const & parent
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(parent);
    notImplemented("hasParentEqualTo", HERE);
}


void Observer::hasLabelInParentEqualTo (
    bool const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(label);
    notImplemented("hasLabelInParentEqualTo", HERE);
}



// TAG SPECIFICATION

void Observer::hasTag (
    bool const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasTag, HERE);
}


void Observer::hasTagEqualTo (
    bool const & result,
    NodePrivate const & thisNode,
    methyl::Tag const & tag
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(tag);
    notImplemented("hasTagEqualTo", HERE);
}


void Observer::tag(
    methyl::Tag const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result)
    addSeenFlags(thisNode, SeenFlags::Tag, HERE);
}


void Observer::tryGetTagNode (
    NodePrivate const * result,
    NodePrivate const & thisNode
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    notImplemented("tryGetTagNode", HERE);
}



// LABEL ENUMERATION
// no implicit ordering, invariant order from ID

void Observer::hasAnyLabels (
    bool const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::hasLabel (
    bool const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::firstLabel (
    Label const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::lastLabel (
    Label const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::hasLabelAfter (
    bool const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::labelAfter (
    Label const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::hasLabelBefore (
    bool const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}


void Observer::labelBefore (
    Label const & result,
    NodePrivate const & thisNode,
    Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SeenFlags::HasLabel, HERE);
    // any additions or removals of labels will invalidate
}



// NODE IN LABEL ENUMERATION

void Observer::hasNextSiblingInLabel (
    bool const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasNextSiblingInLabel, HERE);
}


void Observer::nextSiblingInLabel (
    NodePrivate const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::NextSiblingInLabel, HERE);
}


void Observer::hasPreviousSiblingInLabel (
    bool const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::HasPreviousSiblingInLabel, HERE);
}


void Observer::previousSiblingInLabel (
    NodePrivate const & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::PreviousSiblingInLabel, HERE);
}


void Observer::text (
    const QString & result,
    NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SeenFlags::Data, HERE);
}



//
// WRITE OPERATIONS
// Invalidate any observer which has an interest in this write
//

void Observer::setTag (
    NodePrivate const & thisNode,
    Tag const & tag
) {
    Q_UNUSED(tag);

    globalEngine->forAllObservers([&](Observer & observer) {
        if (observer.isBlinded())
            return;
        if (observer.maybeObserved(thisNode, SeenFlags::Tag)) {
            observer.markBlind();
            return;
        }
    });
}


void Observer::insertChildAsFirstInLabel (
    NodePrivate const & thisNode,
    NodePrivate const & newChild,
    Label const & label,
    NodePrivate const * nextChildInLabel
) {
    Q_UNUSED(label);

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SeenFlags::HasParent
            | SeenFlags::Parent
            | SeenFlags::LabelInParent
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(thisNode, SeenFlags::FirstChild)) {
            observer.markBlind();
            return;
        }

        if (nextChildInLabel) {
            if (observer.maybeObserved(
                *nextChildInLabel,
                SeenFlags::HasPreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (observer.maybeObserved(thisNode, SeenFlags::HasNextSiblingInLabel)) {
                observer.markBlind();
                return;
            }
        } else {
            if (observer.maybeObserved(thisNode, SeenFlags::HasLabel)) {
                observer.markBlind();
                return;
            }
        }
    });
}

void Observer::insertChildAsLastInLabel (
    NodePrivate const & thisNode,
    NodePrivate const & newChild,
    Label const & label,
    const NodePrivate* previousChildInLabel
) {
    Q_UNUSED(label);

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SeenFlags::HasParent
            | SeenFlags::Parent
            | SeenFlags::LabelInParent
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(thisNode, SeenFlags::LastChild)) {
            observer.markBlind();
            return;
        }

        if (previousChildInLabel) {
            if (observer.maybeObserved(*previousChildInLabel,
                SeenFlags::HasNextSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (observer.maybeObserved(newChild, 
                SeenFlags::HasPreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }
        } else {
            if (observer.maybeObserved(thisNode, SeenFlags::HasLabel)) {
                observer.markBlind();
                return;
            }
        }
    });
}


void Observer::insertChildBetween (
    NodePrivate const & thisNode,
    NodePrivate const & newChild,
    NodePrivate const & previousChild,
    NodePrivate const & nextChild
) {
    Q_UNUSED(thisNode);

    // use the insertChildAsFirstInLabel or insertChildAsLastInLabel
    // invalidations if applicable!

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SeenFlags::HasParent
            | SeenFlags::Parent
            | SeenFlags::LabelInParent
        )) {
            observer.markBlind();
            return;
        }

        // previous and next have same status for has next sibling...
        // but the sibling is changing
        if (observer.maybeObserved(newChild,
            SeenFlags::NextSiblingInLabel
            | SeenFlags::HasNextSiblingInLabel
            | SeenFlags::PreviousSiblingInLabel
            | SeenFlags::HasNextSiblingInLabel
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(previousChild, SeenFlags::NextSiblingInLabel)) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(nextChild, SeenFlags::PreviousSiblingInLabel)) {
            observer.markBlind();
            return;
        }
    });
}


void Observer::detach (
    NodePrivate const & thisNode,
    NodePrivate const & parent,
    NodePrivate const * previousChild,
    NodePrivate const * nextChild,
    NodePrivate const * replacement
) {
    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(thisNode,
            SeenFlags::HasParent
            | SeenFlags::Parent
            | SeenFlags::LabelInParent
            | SeenFlags::NextSiblingInLabel
            | SeenFlags::PreviousSiblingInLabel
        )) {
            observer.markBlind();
            return;
        }

        if (replacement) {
            if (observer.maybeObserved(*replacement,
                SeenFlags::HasParent
                | SeenFlags::Parent
                | SeenFlags::LabelInParent
                | SeenFlags::NextSiblingInLabel
                | SeenFlags::PreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }
        }
        if (previousChild) {
            if (observer.maybeObserved(*previousChild, 
                SeenFlags::NextSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (
                (not replacement)
                and (not nextChild)
                and observer.maybeObserved(*previousChild,
                    SeenFlags::HasNextSiblingInLabel
                )
            ) {
                observer.markBlind();
                return;
            }
        } else {
            // first child is changing...
            if (observer.maybeObserved(parent, SeenFlags::FirstChild)) {
                observer.markBlind();
                return;
            }
        }
        if (nextChild) {
            if (observer.maybeObserved(*nextChild, SeenFlags::PreviousSiblingInLabel)) {
                observer.markBlind();
                return;
            }

            if (
                (not replacement)
                and (not previousChild)
                and observer.maybeObserved(*nextChild, SeenFlags::HasNextSiblingInLabel)
            ) {
                observer.markBlind();
                return;
            }
        } else {
            // last child is changing...
            if (observer.maybeObserved(parent, SeenFlags::LastChild)) {
                observer.markBlind();
                return;
            }
        }
    });
}


void Observer::setText (
    NodePrivate const & thisNode,
    QString const & str
) {
    Q_UNUSED(str);

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(thisNode, SeenFlags::Data)) {
            observer.markBlind();
            return;
        }
    });
}


Observer::~Observer() {
    QWriteLocker lock (&globalEngine->_observersLock);
    globalEngine->_observers.erase(this);
}

} // end namespace methyl
