//
// nodeprivate.h
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

#ifndef METHYL_NODEPRIVATE_H
#define METHYL_NODEPRIVATE_H

#include "methyl/defs.h"

#include <unordered_set>

#include <QDomNode>
#include <QDomText>
#include <QDomElement>

namespace methyl {

// The NodePrivate name follows the Qt convention of having the private
// data members (through the PIMPL idiom) in a class named XXXPrivate.
// For expedience in the stub implementation, there is a lot of code in
// the header and none of the tricks used there.  But it should follow
// that pattern after stabilization.

class NodePrivate final {
private:
    // Nodes may have data or they may have labeled children (not both)
    // If node has children:
    // 	tag is the Uuid of the node's Tag in Base64
    // 	attribute "id" is Uuid of the node's Identity in Base64
    // 	labels are QDomElement children of this element
    //		their tags are the label Identity
    //		children of these labels are parent's children in that label
    //  If node has data:
    //	tag is the string "data"
    //	attribute "id" is Uuid of the node's Identity in Base64
    //	attribute "data" is the node's UNICODE data string
    QDomElement _element;

protected:
    // never call this explicitly, manage lifetime w/unique_ptr
    // final class: need not be virtual!  Rare case.
    ~NodePrivate(); 
public:
    template<typename> friend struct std::default_delete;

// construction / destruction are private
// clients should use static New() method
friend class Node;
friend class Engine;

private:
    NodePrivate (Identity const & id, QString const & str);
    NodePrivate (Identity const & id, Tag const & tag);

public:
    NodePrivate () = delete;
    NodePrivate (NodePrivate const &) = delete;
    NodePrivate & operator= (NodePrivate const &) = delete;

// Implementation helpers
private:
    static NodePrivate & NodeFromDomElement(QDomElement const & element);
    static Label LabelFromDomElement(QDomElement const & element);

public:
    static NodePrivate const * maybeGetFromId(Identity const & id);

public:
    bool operator== (NodePrivate const & other) const {
        return _element == other._element;
    }
    bool operator!= (NodePrivate const & other) const {
        return _element != other._element;
    }

public:
    static unique_ptr<NodePrivate> create(Tag const & tag);
    unique_ptr<NodePrivate> makeCloneOfSubtree() const;

// read-only accessors
public:
    NodePrivate const & getDoc() const;
    NodePrivate & getDoc();

    // Extract the Identity of this node.
public:
    Identity getId() const;

public:
    // Parent specification
    bool hasParent() const;
    NodePrivate const & getParent(codeplace const & cp) const;
    NodePrivate & getParent(codeplace const & cp);
    Label getLabelInParent(codeplace const & cp) const;

public:
    // Tag specification
    bool hasTag() const;
    Tag getTag(codeplace const & cp) const;

    // Data accessors
    bool hasText() const { return !hasTag(); }
    QString getText(codeplace const & cp) const; // should have more 'micro-observer'...

private:
    tuple<bool, optional<QDomElement>> maybeGetLabelElementCore (
        Label const & label,
        bool createIfNecessary = false
    ) const;
    optional<QDomElement> maybeGetLabelElement(Label const & label) const {
        return std::get<1>(maybeGetLabelElementCore(label, false));
    }
    tuple<bool, QDomElement> getLabelElementCreateIfNecessary(
        Label const & label
    ) const {
        auto wasCreatedAndLabelElement = maybeGetLabelElementCore(label, true);
        return std::make_tuple(
            std::get<0>(wasCreatedAndLabelElement),
            *std::get<1>(wasCreatedAndLabelElement)
        );
    }
    QDomElement getLabelElement(Label const & label) const {
        return *(std::get<1>(maybeGetLabelElementCore(label, false)));
    }

public:
    // label enumeration; no implicit ordering, invariant order from Identity
    bool hasAnyLabels() const;
    bool hasLabel(Label const & label) const;
    Label getFirstLabel(codeplace const & cp) const;
    Label getLastLabel(codeplace const & cp) const;

    bool hasLabelAfter(Label const & label) const;
    Label getLabelAfter(Label const & label, codeplace const & cp) const;
    optional<Label> maybeGetLabelAfter(Label const & label) const {
        if (not hasLabelAfter(label))
            return nullopt;
        return getLabelAfter(label, HERE);
    }
    bool hasLabelBefore(Label const & label) const;
    Label getLabelBefore(Label const & label, codeplace const & cp) const;
    optional<Label> maybeGetLabelBefore(Label const & label) const {
        if (not hasLabelBefore(label))
            return nullopt;
        return getLabelBefore(label, HERE);
    }

    // node in label enumeration
    NodePrivate const & getFirstChildInLabel(Label const & label, codeplace const & cp) const;
    NodePrivate & getFirstChildInLabel(Label const & label, codeplace const & cp);
    NodePrivate const * maybeGetFirstChildInLabel(Label const & label) const {
        if (not hasLabel(label))
            return nullptr;
        return &getFirstChildInLabel(label, HERE);
    }
    NodePrivate * maybeGetFirstChildInLabel(Label const & label) {
        if (not hasLabel(label))
            return nullptr;
        return &getFirstChildInLabel(label, HERE);
    }

    NodePrivate const & getLastChildInLabel(Label const & label, codeplace const & cp) const;
    NodePrivate & getLastChildInLabel(Label const & label, codeplace const & cp);
    NodePrivate const * maybeGetLastChildInLabel(Label const & label) const {
        if (not hasLabel(label))
            return nullptr;
        return &getLastChildInLabel(label, HERE);
    }
    NodePrivate * maybeGetLastChildInLabel(Label const & label) {
        if (not hasLabel(label))
            return nullptr;
        return &getLastChildInLabel(label, HERE);
    }

    bool hasNextSiblingInLabel() const;
    NodePrivate const & getNextSiblingInLabel(codeplace const & cp) const;
    NodePrivate & getNextSiblingInLabel(codeplace const & cp);
    NodePrivate const * maybeGetNextSiblingInLabel() const {
        if (not hasNextSiblingInLabel())
            return nullptr;
        return &getNextSiblingInLabel(HERE);
    }
    NodePrivate * maybeGetNextSiblingInLabel() {
        if (not hasNextSiblingInLabel())
            return nullptr;
        return &getNextSiblingInLabel(HERE);
    }

    bool hasPreviousSiblingInLabel() const;
    NodePrivate const & getPreviousSiblingInLabel(codeplace const & cp) const;
    NodePrivate & getPreviousSiblingInLabel(codeplace const & cp);
    NodePrivate const * maybeGetPreviousSiblingInLabel() const {
        if (not hasPreviousSiblingInLabel())
            return nullptr;
        return &getPreviousSiblingInLabel(HERE);
    }
    NodePrivate * maybeGetPreviousSiblingInLabel() {
        if (not hasPreviousSiblingInLabel())
            return nullptr;
        return &getPreviousSiblingInLabel(HERE);
    }

    // structural modifications
public:
    void setTag(Tag const & tag);

public:
    NodePrivate & insertChildAsFirstInLabel(
        unique_ptr<NodePrivate> newChild,
        Label const & label,
        NodePrivate const * * nextChildInLabelOut
    );
    NodePrivate & insertChildAsLastInLabel(
        unique_ptr<NodePrivate> newChild,
        Label const & label,
        NodePrivate const * * previousChildInLabelOut
    );
    NodePrivate & insertSiblingAfter(
        unique_ptr<NodePrivate> nodeAfter,
        NodePrivate const * * nodeParentOut,
        Label * labelInParentOut,
        NodePrivate const * * nextChildInLabelOut
    );
    NodePrivate & insertSiblingBefore(
        unique_ptr<NodePrivate> nodeBefore,
        NodePrivate const * * nodeParentOut,
        Label * labelInParentOut,
        NodePrivate const * * previousChildInLabelOut
    );
    // detach from parent
    unique_ptr<NodePrivate> detach(
        Label * labelOut,
        NodePrivate const * * nodeParentOut,
        NodePrivate const * * previousChildOut,
        NodePrivate const * * nextChildOut
    );
    unique_ptr<NodePrivate> replaceWith(
        unique_ptr<NodePrivate> nodeReplacement,
        Label * labelInParentOut,
        NodePrivate const * * nodeParentOut,
        NodePrivate const * * previousChildOut,
        NodePrivate const * * nextChildOut
    );

    // data modifications
public:
    void setText(QString const & str);

// simple constant accessors
public:
    static unique_ptr<NodePrivate> createText(QString const & data);

// traversal and comparison
public:
    NodePrivate const * maybeNextPreorderNodeUnderRoot(
        NodePrivate const & nodeRoot
    ) const {
        NodePrivate const * nodeCur = this;
        if (nodeCur->hasAnyLabels())
            return &nodeCur->getFirstChildInLabel(
                nodeCur->getFirstLabel(HERE), HERE
            );

        while (nodeCur != &nodeRoot) {
            if (nodeCur->hasNextSiblingInLabel())
                return &nodeCur->getNextSiblingInLabel(HERE);

            Label labelInParent = nodeCur->getLabelInParent(HERE);
            NodePrivate const & nodeParent = nodeCur->getParent(HERE);
            if (nodeParent.hasLabelAfter(labelInParent))
                return &nodeParent.getFirstChildInLabel(
                    nodeParent.getLabelAfter(labelInParent, HERE), HERE
                );

            nodeCur = &nodeParent;
        }
        return nullptr;
    }

    NodePrivate * maybeNextPreorderNodeUnderRoot(
        NodePrivate const & nodeRoot
    ) {
        NodePrivate const & constRef = *this;
        return const_cast<NodePrivate *>(
            constRef.maybeNextPreorderNodeUnderRoot(nodeRoot)
        );
    }

    // I don't really like the integer return values, and I'm not sure if I'm
    // picking the right cases to be -1 or 1.  Private for now.
private:
    int compare(NodePrivate const & other) const
    {
        NodePrivate const * thisCur = this;
        NodePrivate const * otherCur = &other;

        do {
            if (thisCur->hasText() and not otherCur->hasText())
                return -1;
            if (not thisCur->hasText() and otherCur->hasText())
                return 1;
            if (thisCur->hasText() and otherCur->hasText()) {
                int cmp = thisCur->getText(HERE).compare(otherCur->getText(HERE));
                if (cmp != 0)
                    return cmp;
            } else {
                int cmp = thisCur->getTag(HERE).compare(otherCur->getTag(HERE));
                if (cmp != 0)
                    return cmp;
                if (thisCur->getTag(HERE) != otherCur->getTag(HERE))
                    return false;
            }
            thisCur = thisCur->maybeNextPreorderNodeUnderRoot(*this);
            otherCur = otherCur->maybeNextPreorderNodeUnderRoot(other);
        } while (thisCur and otherCur);

        if (thisCur and not otherCur)
            return -1;
        if (otherCur and not thisCur)
            return 1;

        return 0;
    }

public:
    bool isSubtreeCongruentTo(NodePrivate const & other) const
    {
        return compare(other) == 0;
    }

    // See remarks above about not being sure if I've picked the absolute right
    // invariants for -1 vs +1.  This is going to be canon...encoded in file
    // formats and stuff, it should be gotten right!
    bool lowerStructureRankThan(NodePrivate const & other) const
    {
        return compare(other) == -1;
    }
};


// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class NODEPRIVATE_no_moc_warning : public QObject { Q_OBJECT };
}

#endif // NODEPRIVATE_H
