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

namespace methyl {

tracked<bool> globalDebugObserver (false, HERE);

shared_ptr<Observer> Observer::create(codeplace const & cp) {
    return globalEngine->makeObserver(cp);
}


Observer::Observer (methyl::Engine & engine, codeplace const & cp) :
    _map (std::unordered_map<NodePrivate const *, SeenFlags>()),
    _engine (engine)
{
    Q_UNUSED(cp);

    QWriteLocker lock (&_engine._observersLock);
    _engine._observers.insert(this);
}


//
// READ OPERATIONS
// Record the read as interesting only to the currently effective observer
//

// node in label enumeration
void Observer::getFirstChildInLabel(
    methyl::NodePrivate const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    // changes to first child in any label will invalidate
    addSeenFlags(thisNode, SawFirstChild, HERE);
    // effectively, we have been told the result has no previous siblings and
    // its label in parent is label!  So no need for SawLabelInParent or
    // SawHasPreviousSiblingInLabel
}

void Observer::getLastChildInLabel(
    methyl::NodePrivate const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    // changes to last child in any label will invalidate
    addSeenFlags(thisNode, SawLastChild, HERE);
    // effectively, we have been told the result has no next siblings and
    // its label in parent is label!  So no need for SawLabelInParent or
    // SawHasNextSiblingInLabel
}

void Observer::hasParent(
    bool const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasParent, HERE);
}

void Observer::getParent(
    methyl::NodePrivate const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawParent, HERE);
}

void Observer::getLabelInParent(
    methyl::Label const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawLabelInParent, HERE);
}

void Observer::hasParentEqualTo(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::NodePrivate const & parent
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(parent);
    notImplemented("hasParentEqualTo", HERE);
}

void Observer::hasLabelInParentEqualTo(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(label);
    notImplemented("hasLabelInParentEqualTo", HERE);
}

// TAG SPECIFICATION

void Observer::hasTag(
    bool const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasTag, HERE);
}

void Observer::hasTagEqualTo(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Tag const & tag
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    Q_UNUSED(tag);
    notImplemented("hasTagEqualTo", HERE);
}

void Observer::getTag(
    methyl::Tag const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result)
    addSeenFlags(thisNode, SawTag, HERE);
}

void Observer::tryGetTagNode(
    const methyl::NodePrivate * result,
    methyl::NodePrivate const & thisNode
) {
    // TODO: implement this
    Q_UNUSED(result);
    Q_UNUSED(thisNode);
    notImplemented("tryGetTagNode", HERE);
}

// LABEL ENUMERATION
// no implicit ordering, invariant order from ID

void Observer::hasAnyLabels(
    bool const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::hasLabel(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::getFirstLabel(
    methyl::Label const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::getLastLabel(
    methyl::Label const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::hasLabelAfter(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::getLabelAfter(
    methyl::Label const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::hasLabelBefore(
    bool const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

void Observer::getLabelBefore(
    methyl::Label const & result,
    methyl::NodePrivate const & thisNode,
    methyl::Label const & label
) {
    Q_UNUSED(result);
    Q_UNUSED(label);
    addSeenFlags(thisNode, SawHasLabel, HERE);
    // any additions or removals of labels will invalidate
}

// NODE IN LABEL ENUMERATION

void Observer::hasNextSiblingInLabel(
    bool const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasNextSiblingInLabel, HERE);
}

void Observer::getNextSiblingInLabel(
    methyl::NodePrivate const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawNextSiblingInLabel, HERE);
}

void Observer::hasPreviousSiblingInLabel(
    bool const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawHasPreviousSiblingInLabel, HERE);
}

void Observer::getPreviousSiblingInLabel(
    methyl::NodePrivate const & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawPreviousSiblingInLabel, HERE);
}

void Observer::getText(
    const QString & result,
    methyl::NodePrivate const & thisNode
) {
    Q_UNUSED(result);
    addSeenFlags(thisNode, SawData, HERE);
}



//
// WRITE OPERATIONS
// Invalidate any observer which has an interest in this write
//

void Observer::setTag(
    methyl::NodePrivate const & thisNode,
    const methyl::Tag& /*tag*/
) {
    globalEngine->forAllObservers([&](Observer & observer) {
        if (observer.isBlinded())
            return;
        if (observer.maybeObserved(thisNode, SawTag)) {
            observer.markBlind();
            return;
        }
    });
}

void Observer::insertChildAsFirstInLabel(
    methyl::NodePrivate const & thisNode,
    methyl::NodePrivate const & newChild,
    methyl::Label const & /*label*/,
    const methyl::NodePrivate * nextChildInLabel
) {
    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SawHasParent
            | SawParent
            | SawLabelInParent
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(thisNode, SawFirstChild)) {
            observer.markBlind();
            return;
        }

        if (nextChildInLabel) {
            if (observer.maybeObserved(
                *nextChildInLabel,
                SawHasPreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (observer.maybeObserved(thisNode, SawHasNextSiblingInLabel)) {
                observer.markBlind();
                return;
            }
        } else {
            if (observer.maybeObserved(thisNode, SawHasLabel)) {
                observer.markBlind();
                return;
            }
        }
    });
}

void Observer::insertChildAsLastInLabel(
    methyl::NodePrivate const & thisNode,
    methyl::NodePrivate const & newChild,
    methyl::Label const & label,
    const methyl::NodePrivate* previousChildInLabel
) {
    Q_UNUSED(label);

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SawHasParent
            | SawParent
            | SawLabelInParent
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(thisNode, SawLastChild)) {
            observer.markBlind();
            return;
        }

        if (previousChildInLabel) {
            if (observer.maybeObserved(*previousChildInLabel,
                SawHasNextSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (observer.maybeObserved(newChild, 
                SawHasPreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }
        } else {
            if (observer.maybeObserved(thisNode, SawHasLabel)) {
                observer.markBlind();
                return;
            }
        }
    });
}

void Observer::insertChildBetween(
    methyl::NodePrivate const & thisNode,
    methyl::NodePrivate const & newChild,
    methyl::NodePrivate const & previousChild,
    methyl::NodePrivate const & nextChild
) {
    Q_UNUSED(thisNode);

    // use the insertChildAsFirstInLabel or insertChildAsLastInLabel
    // invalidations if applicable!

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(newChild,
            SawHasParent
            | SawParent
            | SawLabelInParent
        )) {
            observer.markBlind();
            return;
        }

        // previous and next have same status for has next sibling...
        // but the sibling is changing
        if (observer.maybeObserved(newChild,
            SawNextSiblingInLabel
            | SawHasNextSiblingInLabel
            | SawPreviousSiblingInLabel
            | SawHasNextSiblingInLabel
        )) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(previousChild, SawNextSiblingInLabel)) {
            observer.markBlind();
            return;
        }

        if (observer.maybeObserved(nextChild, SawPreviousSiblingInLabel)) {
            observer.markBlind();
            return;
        }
    });
}

void Observer::detach(
    methyl::NodePrivate const & thisNode,
    methyl::NodePrivate const & parent,
    methyl::NodePrivate const * previousChild,
    methyl::NodePrivate const * nextChild,
    methyl::NodePrivate const * replacement
) {
    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(thisNode,
            SawHasParent
            | SawParent
            | SawLabelInParent
            | SawNextSiblingInLabel
            | SawPreviousSiblingInLabel
        )) {
            observer.markBlind();
            return;
        }

        if (replacement) {
            if (observer.maybeObserved(*replacement,
                SawHasParent
                | SawParent
                | SawLabelInParent
                | SawNextSiblingInLabel
                | SawPreviousSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }
        }
        if (previousChild) {
            if (observer.maybeObserved(*previousChild, 
                SawNextSiblingInLabel
            )) {
                observer.markBlind();
                return;
            }

            if (
                (not replacement)
                and (not nextChild)
                and observer.maybeObserved(*previousChild,
                    SawHasNextSiblingInLabel
                )
            ) {
                observer.markBlind();
                return;
            }
        } else {
            // first child is changing...
            if (observer.maybeObserved(parent, SawFirstChild)) {
                observer.markBlind();
                return;
            }
        }
        if (nextChild) {
            if (observer.maybeObserved(*nextChild, SawPreviousSiblingInLabel)) {
                observer.markBlind();
                return;
            }

            if (
                (not replacement)
                and (not previousChild)
                and observer.maybeObserved(*nextChild, SawHasNextSiblingInLabel)
            ) {
                observer.markBlind();
                return;
            }
        } else {
            // last child is changing...
            if (observer.maybeObserved(parent, SawLastChild)) {
                observer.markBlind();
                return;
            }
        }
    });
}

// data modifications
void Observer::setText(
    methyl::NodePrivate const & thisNode,
    QString const & str
) {
    Q_UNUSED(str);

    globalEngine->forAllObservers([&](Observer & observer) {

        if (observer.isBlinded())
            return;

        if (observer.maybeObserved(thisNode, SawData)) {
            observer.markBlind();
            return;
        }
    });
}

Observer::~Observer()
{
    QWriteLocker lock (&_engine._observersLock);
    _engine._observers.erase(this);
}

} // end namespace methyl
