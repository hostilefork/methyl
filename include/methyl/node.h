//
// node.h
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

#ifndef METHYL_NODE_H
#define METHYL_NODE_H

#include <QUuid>
#include <QDebug>

#include <unordered_set>

#include "methyl/defs.h"
#include "methyl/nodeprivate.h"
#include "methyl/observer.h"
#include "methyl/context.h"
#include "methyl/noderef.h"
#include "methyl/rootnode.h"

// Don't want a dependency on the engine.h file in node.h
// Have to do some acrobatics to get around that
// Long term, nodeprivate/observer/context should be hidden
// as opaque types so clients can be insulated from the details

namespace std {

    // Need this to get std::unordered_set to work on NodeRef
    // http://stackoverflow.com/questions/8157937/

    template<class NodeType>
    struct hash<methyl::NodeRef<NodeType>>
    {
        size_t operator()(
            methyl::NodeRef<NodeType> const & nodeRef
        ) const {
            return std::hash<methyl::NodePrivate const *>()(
                &nodeRef.getNode().getNodePrivate()
            );
        }
    };

    // Need this to get std::map to work on NodeRef
    // http://stackoverflow.com/a/10734231/211160

    template<class NodeType>
    struct less<methyl::NodeRef<NodeType>>
    {
        bool operator()(
            methyl::NodeRef<NodeType> const & left,
            methyl::NodeRef<NodeType> const & right
        ) {
            return left->lowerStructureRankThan(right);
        }
    };
}

namespace methyl {

// Same for Engine.  Rename as MethylEngine or similar?
class Engine;


// Node is the base for our proxy classes.  They provide a compound
// interface to NodePrivate.  If you inherit publicly from it, then your
// wrapped entity will also offer the NodePrivate interface functions...if you
// inherit protected then you will only offer your own interface.  Private
// inheritance is not possible because to do the casting, the templates must
// be able to tell that your class is a subclass of Node

class Node {
    template<class> friend struct std::hash;
    friend class Engine;

private:
    // Node instances are not to assume they will always be processing
    // the same NodePrivate.  But for implementation convenience, the NodeRef
    // pokes the NodePrivate in before passing on
    mutable NodePrivate * nodePrivateDoNotUseDirectly;
    mutable shared_ptr<Context> contextDoNotUseDirectly;
template<class NodeType> friend class NodeRef;
template<class NodeType> friend class RootNode;
    void setInternalProperties(
        NodePrivate * nodePrivate,
        shared_ptr<Context> context
    ) {
        // only called by NodeRef.  The reason this is not passed in
        // the constructor is because it would have to come via
        // the derived class, we want compiler default constructors
        nodePrivateDoNotUseDirectly = nodePrivate;
        contextDoNotUseDirectly = context;
    }
    void setInternalProperties(
        NodePrivate const * nodePrivate,
        shared_ptr<Context> context
    ) const {
        // we know getNodePrivate() will only return a const node which will
        // restore the constness.  As long as nodePtr is not used
        // directly in its non-const form by this class, we should
        // not trigger undefined behavior...
        nodePrivateDoNotUseDirectly = const_cast<NodePrivate *>(nodePrivate);
        contextDoNotUseDirectly = context;
    }

private:
    // Node extraction--for friend internal classes only
    NodePrivate & getNodePrivate() {
        hopefully(nodePrivateDoNotUseDirectly, HERE);
        return *nodePrivateDoNotUseDirectly;
    }
    NodePrivate const & getNodePrivate() const {
        hopefully(nodePrivateDoNotUseDirectly, HERE);
        return *nodePrivateDoNotUseDirectly;
    }
    NodePrivate * maybeGetNodePrivate() {
        return nodePrivateDoNotUseDirectly;
    }
    NodePrivate const * maybeGetNodePrivate() const {
        return nodePrivateDoNotUseDirectly;
    }
    shared_ptr<Context> getContext() const {
        // Even if the node itself is const, we need to be able to change the
        // observer to mark what things have been seen
        return contextDoNotUseDirectly;
    }
    shared_ptr<Observer> getObserver() const {
        // Even if the node itself is const, we need to be able to change the
        // observer to mark what things have been seen
        return contextDoNotUseDirectly->getObserver();
    }

protected:
    // accessors should not be constructible by anyone but the
    // NodeRef class (how to guarantee that?)
    explicit Node () {}

protected:
    // Unfortunately we wind up in accessors and need a NodeRef for the
    // current node when all we have is a this pointer.  Not a perfect
    // solution--could use some more thought
    template<class NodeType>
    NodeRef<NodeType const> thisNodeAs() const {
        return NodeRef<NodeType const>(getNodePrivate(), getContext());
    }
    template<class NodeType>
    NodeRef<NodeType> thisNodeAs() {
        return NodeRef<NodeType>(getNodePrivate(), getContext());
    }

public:
    // read-only accessors
    NodeRef<Node const> getDoc() const {
        return NodeRef<Node const>(
            getNodePrivate().getDoc(),
            getContext()
        );
    }
    NodeRef<Node> getDoc() {
        return NodeRef<Node>(
            getNodePrivate().getDoc(),
            getContext()
        );
    }

    // Extract the Identity of this node.
public:
    Identity getId() const { return getNodePrivate().getId(); }

public:
    // Parent specification
    bool hasParent() const {
        bool result = getNodePrivate().hasParent();
        getObserver()->hasParent(result, getNodePrivate());
        return result;
    }
    NodeRef<Node const> getParent() const {
        NodePrivate const & result = getNodePrivate().getParent();
        getObserver()->getParent(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }
    NodeRef<Node> getParent() {
        NodePrivate & result = getNodePrivate();
        getObserver()->getParent(result, getNodePrivate());
        return NodeRef<Node>(result, getContext());
    }
    optional<NodeRef<Node>> maybeGetParent() const {
        if (not hasParent())
            return nullopt;
        return getParent();
    }
    Label getLabelInParent() const {
        Label result = getLabelInParent();

        getObserver()->getLabelInParent(result, getNodePrivate());
        return result;
    }
    template<class NodeType>
    bool hasParentEqualTo(NodeRef<NodeType> possibleParent) const {
        // should be a finer-grained micro-observation than this
        optional<NodeRef<Node>> parent = maybeGetParent();
        if (not parent) {
            return false;
        }
        return *parent == possibleParent;

    }
    bool hasLabelInParentEqualTo(Label const & possibleLabel) const {
        // should be a finer-grained observation than this
        Label const labelInParent = getLabelInParent();
        return labelInParent == possibleLabel;
    }

public:
    // Tag specification
    bool hasTag() const {
        bool result = getNodePrivate().hasTag();
        getObserver()->hasTag(result, getNodePrivate());
        return result;
    }
    Tag getTag() const {
        Tag result = getNodePrivate().getTag();
        getObserver()->getTag(result, getNodePrivate());
        return result;
    }
    optional<NodeRef<Node const>> maybeGetTagNode() const {
        if (not hasTag())
            return nullopt;

        Tag const & tag = getTag();

        NodePrivate const * optTagNode = NodePrivate::maybeGetFromId(tag);

        if (not optTagNode)
            return nullopt;

        return NodeRef<Node const>(*optTagNode, getContext());
    }
    bool hasTagEqualTo(Tag const & possibleTag) const {
        // should be a finer-grained observation than this
        if (not hasTag())
            return false;
        Tag const tag = getTag();
        return tag == possibleTag;
    }


public:
    // Data accessors - should probably come from a templated lexical cast?
    bool hasText() const { return getNodePrivate().hasText(); }
    QString getText() const {
        QString result = getNodePrivate().getText();
        getObserver()->getText(result, getNodePrivate());
        return result;
    }

public:
    // label enumeration; no implicit ordering, invariant order from Identity
    bool hasAnyLabels() const {
        bool result = getNodePrivate().hasAnyLabels();
        getObserver()->hasAnyLabels(result, getNodePrivate());
        return result;
    }
    bool hasLabel(Label const & label) const {
        bool result = getNodePrivate().hasLabel(label);
        getObserver()->hasLabel(result, getNodePrivate(), label);
        return result;
    }
    Label getFirstLabel() const {
        Label result = getNodePrivate().getFirstLabel();
        getObserver()->getFirstLabel(result, getNodePrivate());
        return result;
    }
    Label getLastLabel() const {
        Label result = getNodePrivate().getLastLabel();
        getObserver()->getLastLabel(result, getNodePrivate());
        return result;
    }

    bool hasNextLabel(Label const & label) const {
        bool result = getNodePrivate().hasNextLabel(label);
        getObserver()->hasNextLabel(result, getNodePrivate(), label);
        return result;
    }
    Label getNextLabel(Label const & label) const {
        Label result = getNodePrivate().getNextLabel(label);
        getObserver()->getNextLabel(result, getNodePrivate(), label);
        return result;
    }
    optional<Label> maybeGetNextLabel(Label const & label) const {
        return getNodePrivate().maybeGetNextLabel(label);
    }
    bool hasPreviousLabel(Label const & label) const {
        bool result = getNodePrivate().hasPreviousLabel(label);
        getObserver()->hasPreviousLabel(result, getNodePrivate(), label);
        return result;
    }
    Label getPreviousLabel(Label const & label) const {
        Label result = getNodePrivate().getPreviousLabel(label);
        getObserver()->getPreviousLabel(result, getNodePrivate(), label);
        return result;
    }
    optional<Label> maybeGetPreviousLabel(Label const & label) const {
        return getNodePrivate().maybeGetPreviousLabel(label);
    }

    // node in label enumeration
public:
    NodeRef<Node const> getFirstChildInLabel(Label const & label) const {
        auto & result = getNodePrivate().getFirstChildInLabel(label);
        getObserver()->getFirstChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node const>(result, getContext());
    }
    NodeRef<Node> getFirstChildInLabel(Label const & label) {
        auto & result = getNodePrivate().getFirstChildInLabel(label);
        getObserver()->getFirstChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node>(result, getContext());
    }
    optional<NodeRef<Node const>> maybeGetFirstChildInLabel(
        Label const & label
    ) const {
        if (not hasLabel(label))
            return nullopt;
        return getFirstChildInLabel(label);
    }
    optional<NodeRef<Node>> maybeGetFirstChildInLabel(Label const & label) {
        if (not hasLabel(label))
            return nullopt;
        return getFirstChildInLabel(label);
    }
    NodeRef<Node const> getLastChildInLabel(Label const & label) const {
        auto & result = getNodePrivate().getLastChildInLabel(label);
        getObserver()->getLastChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node const>(result, getContext());
    }
    NodeRef<Node> getLastChildInLabel(Label const & label) {
        NodePrivate & result = getNodePrivate().getLastChildInLabel(label);
        getObserver()->getLastChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node>(
            getNodePrivate().getLastChildInLabel(label),
            getContext()
        );
    }
    optional<NodeRef<Node const>> maybeGetLastChildInLabel(
        Label const & label
    ) const {
        if (not getNodePrivate().hasLabel(label))
            return nullopt;
        return NodeRef<Node const>(
            getNodePrivate().getLastChildInLabel(label),
            getContext()
        );
    }
    optional<NodeRef<Node>> maybeGetLastChildInLabel(Label const & label) {
        if (not getNodePrivate().hasLabel(label))
            return nullopt;
        return NodeRef<Node>(
            getNodePrivate().getLastChildInLabel(label),
            getContext()
        );
    }

    bool hasNextSiblingInLabel() const {
        bool result = getNodePrivate().hasNextSiblingInLabel();
        getObserver()->hasNextSiblingInLabel(result, getNodePrivate());
        return result;
    }
    NodeRef<Node const> getNextSiblingInLabel() const {
        NodePrivate const & result = getNodePrivate().getNextSiblingInLabel();
        getObserver()->getNextSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }
    NodeRef<Node> getNextSiblingInLabel() {
        NodePrivate & result = getNodePrivate().getNextSiblingInLabel();
        getObserver()->getNextSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node>(result, getContext());
    }
    optional<NodeRef<Node const>> maybeGetNextSiblingInLabel() const {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return getNextSiblingInLabel();
    }
    optional<NodeRef<Node>> maybeGetNextSiblingInLabel() {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return getNextSiblingInLabel();
    }

    bool hasPreviousSiblingInLabel() const {
        bool result = getNodePrivate().hasPreviousSiblingInLabel();
        getObserver()->hasPreviousSiblingInLabel(result, getNodePrivate());
        return result;
    }
    NodeRef<Node const> getPreviousSiblingInLabel() const {
        NodePrivate const & result = getNodePrivate().getPreviousSiblingInLabel();
        getObserver()->getPreviousSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }
    NodeRef<Node> getPreviousSiblingInLabel() {
        NodePrivate & result = getNodePrivate().getPreviousSiblingInLabel();
        getObserver()->getPreviousSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }
    optional<NodeRef<Node const>> maybeGetPreviousSiblingInLabel() const {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return getPreviousSiblingInLabel();
    }
    optional<NodeRef<Node>> maybeGetPreviousSiblingInLabel() {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return getPreviousSiblingInLabel();
    }

    // special accessor for getting children without counting as an observation
    // of their order.  (it does count as an observation of the number of
    // children, obviously).  Implementation lost in a refactoring, add with
    // other missing micro-observations

    std::unordered_set<NodeRef<Node const>> getChildSetForLabel() const {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<NodeRef<Node const>>();
    }
    std::unordered_set<NodeRef<Node>> getChildSetForLabel() {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<NodeRef<Node>>();
    }

    // structural modifications
public:
    void setTag(Tag const & tag) {
        getNodePrivate().setTag(tag);
        getObserver()->setTag(getNodePrivate(), tag);
        return;
    }

public:
    template<class NodeType>
    NodeRef<NodeType> insertChildAsFirstInLabel(
        RootNode<NodeType> newChild,
        Label const & label
    ) {
        NodePrivate const * nextChildInLabel;
        NodePrivate & result = getNodePrivate().insertChildAsFirstInLabel(
            std::move(newChild.extractNodePrivate()),
            label,
            &nextChildInLabel
        );
        getObserver()->insertChildAsFirstInLabel(
            getNodePrivate(),
            result,
            label,
            nextChildInLabel
        );
        return NodeRef<NodeType> (result, getContext());
    }
    template<class NodeType>
    NodeRef<NodeType> insertChildAsLastInLabel(
        RootNode<NodeType> newChild,
        Label const & label
    ) {
        NodePrivate const * previousChildInLabel;
        NodePrivate & result = getNodePrivate().insertChildAsLastInLabel(
            std::move(newChild.extractNodePrivate()),
            label,
            &previousChildInLabel
        );
        getObserver()->insertChildAsLastInLabel(
            getNodePrivate(),
            result,
            label,
            previousChildInLabel
        );
        return NodeRef<NodeType> (result, getContext());
    }
    template<class NodeType>
    NodeRef<NodeType> insertSiblingAfter(
        RootNode<NodeType> newChild
    ) {
        NodePrivate const * nextChildInLabel;
        NodePrivate const * parent;

        Label label;
        NodePrivate & result = getNodePrivate().insertSiblingAfter(
            std::move(newChild.extractNodePrivate()),
            &parent,
            &label,
            &nextChildInLabel
        );
        if (nextChildInLabel == nullptr) {
            getObserver()->insertChildAsLastInLabel(
                *parent,
                result,
                label,
                &getNodePrivate()
            );
        } else {
            getObserver()->insertChildBetween(
                *parent,
                result,
                getNodePrivate(),
                *nextChildInLabel
            );
        }

        return NodeRef<NodeType> (result, getContext());
    }

    template<class NodeType>
    NodeRef<NodeType> insertSiblingBefore(
        RootNode<NodeType> newChild
    ) {
        NodePrivate const * previousChildInLabel;
        NodePrivate const * parent;

        Label label;
        NodePrivate & result = getNodePrivate().insertSiblingBefore(
            std::move(newChild.extractNodePrivate()),
            &parent,
            &label,
            &previousChildInLabel
        );
        if (previousChildInLabel == nullptr) {
            getObserver()->insertChildAsLastInLabel(
                *parent,
                result,
                label,
                &getNodePrivate()
            );
        } else {
            getObserver()->insertChildBetween(
                *parent,
                result,
                getNodePrivate(),
                *previousChildInLabel
            );
        }

        return NodeRef<NodeType> (result, getContext());
    }
    std::vector<RootNode<Node>> detachAllChildrenInLabel(Label const & label) {
        std::vector<RootNode<Node>> children;
        while (hasLabel(label)) {
            children.push_back(getFirstChildInLabel(label).detach());
        }
        return children;
    }

    // data modifications
    // at one time it mirrored the QString API a little, hoping to add benefit
    // from an observer pattern like "checked if it was an integer, it wasn't,
    // then the text changed and it didn't become an integer...so no need to
    // send an invalidation.
    // http://doc.trolltech.com/4.5/qstring.html
    // it's not the worst idea, but premature to put in the API...revisit if
    // interesting cases show up.  We don't want to introduce data blobs
public:
    void setText(QString const & str) {
        getNodePrivate().setText(str);
        getObserver()->setText(getNodePrivate(), str);
    }
    void insertCharBeforeIndex(int index, QChar const & ch) {
        QString str = getNodePrivate().getText();
        str.insert(index, ch);
        getNodePrivate().setText(str);
        getObserver()->setText(getNodePrivate(), str);
    }
    void insertCharAfterIndex(int index, QChar const & ch) {
        QString str = getNodePrivate().getText();
        str.insert(index + 1, ch);
        getNodePrivate().setText(str);
        getObserver()->setText(getNodePrivate(), str);
    }
    void deleteCharAtIndex(int index) {
        QString str = getNodePrivate().getText();
        str.remove(index, 1);
        getNodePrivate().setText(str);
        getObserver()->setText(getNodePrivate(), str);
    }

public:
    bool sameStructureAs(NodeRef<Node> other) const
    {
        return getNodePrivate().compare(other.getNode().getNodePrivate()) == 0;
    }


    // See remarks above about not being sure if I've picked the absolute right
    // invariants for -1 vs +1.  This is going to be canon...encoded in file
    // formats and stuff, it should be gotten right!
    template<class OtherNodeType>
    bool lowerStructureRankThan(NodeRef<OtherNodeType const> other) const
    {
        return getNodePrivate().compare(other.getNode().getNodePrivate()) == -1;
    }
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class NODE_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace methyl


// "Any class or struct that has a public default constructor, a public copy
// constructor, and a public destructor can be registered."
//
// Under those rules we can't use Q_DECLARE_METATYPE_1ARG because there is no
// public default constructor.  Being able to do default construction instead
// of using optional<NodeRef<T>> would defer to runtime "optional-ness" in
// the baseline declaration of node references that we wish to avoid.  Hence
// because they insist, you can only pass optional<NodeRef<T>> as signal or
// slot parameters.

Q_DECLARE_METATYPE(optional<methyl::NodeRef<methyl::Node const>>)
Q_DECLARE_METATYPE(optional<methyl::NodeRef<methyl::Node>>)

// Could this be used?  I don't know.  Maybe.  But just use the coercions.
// Q_DECLARE_METATYPE_TEMPLATE_1ARG(something)

#endif
