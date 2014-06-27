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

//////////////////////////////////////////////////////////////////////////////
//
// API exposed to Node, RootNode, and NodeRef
//
// This is the NodePrivate API.  It is not public to clients of methyl, but
// it is what the methyl node types build upon.  A NodePrivate is not
// concerned with higher level issues like node contexts or the observer
// pattern; it is only concerned with the mechanics of the data structure.
// Handles are to a non-copyable object
//

public:
    static NodePrivate const * maybeGetFromId (Identity const & id);


public:
    bool operator== (NodePrivate const & other) const {
        return _element == other._element;
    }
    bool operator!= (NodePrivate const & other) const {
        return _element != other._element;
    }


    //
    // Node Creation
    //
public:
    static unique_ptr<NodePrivate> create (Tag const & tag);

    static unique_ptr<NodePrivate> createText (QString const & data);

    unique_ptr<NodePrivate> makeCloneOfSubtree () const;


    //
    // REVIEW: Document and Identity
    //
public:
    NodePrivate const & getDoc() const;

    NodePrivate & getDoc();

    Identity getId() const;


    //
    // Parent Examination
    //
public:
    bool hasParent() const;

    NodePrivate const & getParent (codeplace const & cp) const;

    NodePrivate & getParent (codeplace const & cp);

    Label getLabelInParent (codeplace const & cp) const;


    //
    // Tag Examination
    //
public:
    bool hasTag () const;

    Tag getTag (codeplace const & cp) const;


    //
    // Data Examination
    //
    // Issue: Should there be more 'micro-observers'?
    //
public:
    bool hasText () const {
        return not hasTag();
    }

    QString getText (codeplace const & cp) const;


    //
    // label enumeration; no implicit ordering, invariant order from Identity
    //
public:
    bool hasAnyLabels () const;

    bool hasLabel (Label const & label) const;

    Label getFirstLabel (codeplace const & cp) const;

    Label getLastLabel (codeplace const & cp) const;

    bool hasLabelAfter (Label const & label) const;

    Label getLabelAfter (Label const & label, codeplace const & cp) const;

    optional<Label> maybeGetLabelAfter (Label const & label) const {
        if (not hasLabelAfter(label))
            return nullopt;
        return getLabelAfter(label, HERE);
    }

    bool hasLabelBefore (Label const & label) const;

    Label getLabelBefore (Label const & label, codeplace const & cp) const;

    optional<Label> maybeGetLabelBefore (Label const & label) const {
        if (not hasLabelBefore(label))
            return nullopt;
        return getLabelBefore(label, HERE);
    }


    //
    // node in label enumeration
    //
public:
    NodePrivate const & getFirstChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const;

    NodePrivate & getFirstChildInLabel (
        Label const & label,
        codeplace const & cp
    );

    NodePrivate const * maybeGetFirstChildInLabel (Label const & label) const {
        if (not hasLabel(label))
            return nullptr;
        return &getFirstChildInLabel(label, HERE);
    }

    NodePrivate * maybeGetFirstChildInLabel (Label const & label) {
        if (not hasLabel(label))
            return nullptr;
        return &getFirstChildInLabel(label, HERE);
    }

    NodePrivate const & getLastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const;

    NodePrivate & getLastChildInLabel (
        Label const & label, codeplace const & cp
    );

    NodePrivate const * maybeGetLastChildInLabel (Label const & label) const {
        if (not hasLabel(label))
            return nullptr;
        return &getLastChildInLabel(label, HERE);
    }

    NodePrivate * maybeGetLastChildInLabel (Label const & label) {
        if (not hasLabel(label))
            return nullptr;
        return &getLastChildInLabel(label, HERE);
    }

    bool hasNextSiblingInLabel() const;

    NodePrivate const & getNextSiblingInLabel (codeplace const & cp) const;

    NodePrivate & getNextSiblingInLabel (codeplace const & cp);

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


    //
    // structural modifications
    //
public:
    void setTag(Tag const & tag);

    struct insert_info final {
        NodePrivate const * _nodeParent;
        Label _labelInParent;
        NodePrivate const * _previousChild;
        NodePrivate const * _nextChild;

        insert_info (
            NodePrivate const * nodeParent,
            Label labelInParent,
            NodePrivate const * previousChild,
            NodePrivate const * nextChild
        ) :
            _nodeParent (nodeParent),
            _labelInParent (labelInParent),
            _previousChild (previousChild),
            _nextChild (nextChild)
        {
        }
    };

    typedef tuple<
        std::reference_wrapper<NodePrivate>,
        insert_info
    > insert_result;

    insert_result insertChildAsFirstInLabel (
        unique_ptr<NodePrivate> newChild,
        Label const & label
    );

    insert_result insertChildAsLastInLabel (
        unique_ptr<NodePrivate> newChild,
        Label const & label
    );

    insert_result insertSiblingAfter (
        unique_ptr<NodePrivate> newSibling
    );

    insert_result insertSiblingBefore (
        unique_ptr<NodePrivate> newSibling
    );

    struct detach_info final {
        NodePrivate const & _nodeParent;
        Label _labelInParent;
        NodePrivate const * _previousChild;
        NodePrivate const * _nextChild;

        detach_info (
            NodePrivate const & nodeParent,
            Label labelInParent,
            NodePrivate const * previousChild,
            NodePrivate const * nextChild
        ) :
            _nodeParent (nodeParent),
            _labelInParent (labelInParent),
            _previousChild (previousChild),
            _nextChild (nextChild)
        {
        }
    };

    tuple<unique_ptr<NodePrivate>, detach_info> detach ();

    tuple<unique_ptr<NodePrivate>, detach_info> replaceWith (
        unique_ptr<NodePrivate> nodeReplacement
    );

    void setText(QString const & str);


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

    NodePrivate * maybeNextPreorderNodeUnderRoot (
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
    int compare (NodePrivate const & other) const;


public:
    bool isSubtreeCongruentTo (NodePrivate const & other) const {
        return compare(other) == 0;
    }

    // See remarks above about not being sure if I've picked the absolute right
    // invariants for -1 vs +1.  This is going to be canon...encoded in file
    // formats and stuff, it should be gotten right!
    bool lowerStructureRankThan (NodePrivate const & other) const {
        return compare(other) == -1;
    }


//////////////////////////////////////////////////////////////////////////////
//
// Internal helpers
//
// These routines are private and used in the implementation.
//

    //
    // The stub implementation of the methyl structure, based on the Qt DOM,
    // uses two levels of hierarchy to implement a single level of methyl
    // hierarchy.  This is because there is no labeling of children in the
    // XML DOM--only attributes.  Methyl has no attributes and only labeled
    // children... so labels wind up being mapped to their own QDomElements
    // underneath the QDomElement representing the parent of a Node.
    //
private:
    static NodePrivate & NodeFromDomElement (QDomElement const & element);

    static Label LabelFromDomElement (QDomElement const & element);

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


    //
    // Construction and Assignment
    //
    // The only exposed way to create a NodePrivate is through a static
    // factory method that returns a unique_ptr.
    //
friend class Node;
friend class Engine;
private:
    NodePrivate () = delete;

    NodePrivate (NodePrivate const &) = delete;

    NodePrivate (NodePrivate &&) = delete;

    NodePrivate & operator= (NodePrivate const &) = delete;

    NodePrivate (Identity const & id, QString const & str);

    NodePrivate (Identity const & id, Tag const & tag);


    //
    // Destruction
    //
    // Lifetime is always managed with w/unique_ptr; no stack creation of
    // NodePrivate is allowed.  We also make this a final class, so the
    // destructor need not be virtual.
    //
template <typename> friend struct std::default_delete;
protected:
    ~NodePrivate();


private:
    // Nodes may have data or they may have labeled children (not both)
    // If node has children:
    //  tag is the Uuid of the node's Tag in Base64
    //  attribute "id" is Uuid of the node's Identity in Base64
    //  labels are QDomElement children of this element
    //      their tags are the label Identity
    //      children of these labels are parent's children in that label
    //  If node has data:
    //  tag is the string "data"
    //  attribute "id" is Uuid of the node's Identity in Base64
    //  attribute "data" is the node's UNICODE data string
    QDomElement _element;
};


// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class NODEPRIVATE_no_moc_warning : public QObject { Q_OBJECT };
}

#endif // NODEPRIVATE_H
