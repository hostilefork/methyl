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

    template <class T>
    struct hash<methyl::NodeRef<T>>
    {
        size_t operator()(
            methyl::NodeRef<T> const & nodeRef
        ) const
        {
            return std::hash<methyl::NodePrivate const *>()(
                &nodeRef.getNode().getNodePrivate()
            );
        }
    };

    // Need this to get std::map to work on NodeRef
    // http://stackoverflow.com/a/10734231/211160

    template <class T>
    struct less<methyl::NodeRef<T>>
    {
        bool operator()(
            methyl::NodeRef<T> const & left,
            methyl::NodeRef<T> const & right
        ) const
        {
            return left.getId() < right.getId();
        }
    };

    // Need this to get std::unordered_set to work on RootNode
    // http://stackoverflow.com/questions/8157937/

    template <class T>
    struct hash<methyl::RootNode<T>>
    {
        size_t operator()(
            methyl::RootNode<T> const & nodeRef
        ) const
        {
            return std::hash<methyl::NodePrivate const *>()(
                &nodeRef.getNode().getNodePrivate()
            );
        }
    };

    // Need this to get std::map to work on NodeRef
    // http://stackoverflow.com/a/10734231/211160

    template <class T>
    struct less<methyl::RootNode<T>>
    {
        bool operator()(
            methyl::RootNode<T> const & left,
            methyl::RootNode<T> const & right
        ) const
        {
            return left.getId() < right.getId();
        }
    };

}

namespace methyl {

// If we are interested in treating the methyl structures as comparable
// values vs. by pointer identity, we need a special predicate to use
// in something like an unordered_map... because == compares for identity
// equality not structural equality.  There is also lowerStructureThan
// but an ordered map based on methyl nodes would be slow to access.
// You have to use the structure_hash also because otherwise it would
// hash miss for comparisons

template <class T>
struct structure_hash<methyl::NodeRef<T>>
{
    size_t operator()(
        methyl::NodeRef<T> const & nodeRef
    ) const
    {
        // This algorithm is a placeholder, and needs serious improvement.
        //
        // https://github.com/hostilefork/methyl/issues/32

        NodePrivate const & nodePrivate = nodeRef.getNode().getNodePrivate();

        size_t result = 0;
        int const N = -1; // For now, let's do full equality...
        int i = 0;

        NodePrivate const * current = &nodePrivate;
        while (current and (i++ < N)) {
            // Qt will use qHash and not support std::hash until at least 2015
            // https://bugreports.qt-project.org/browse/QTBUG-33428

            if (current->hasText()) {
                result ^= qHash(current->getText(HERE));
            } else {
                result ^= qHash(current->getTag(HERE).toUuid());
            }
            current = current->maybeNextPreorderNodeUnderRoot(nodePrivate);
        }

        return result;
    }
};

template <class T>
struct structure_equal_to<methyl::NodeRef<T>>
{
    bool operator()(
        methyl::NodeRef<T> const & left,
        methyl::NodeRef<T> const & right
    ) const
    {
        return left->sameStructureAs(right);
    }
};

template <class T>
struct structure_hash<methyl::RootNode<T>>
{
    size_t operator()(
        methyl::RootNode<T> const & ownedNode
    ) const
    {
        structure_hash<methyl::NodeRef<T>> hasher;
        return hasher(ownedNode.get());
    }
};

template <class T>
struct structure_equal_to<methyl::RootNode<T>>
{
    bool operator()(
        methyl::RootNode<T> const & left,
        methyl::RootNode<T> const & right
    ) const
    {
        return left->sameStructureAs(right.get());
    }
};


// Same for Engine.  Rename as MethylEngine or similar?
class Engine;


// Node is the base for our proxy classes.  They provide a compound
// interface to NodePrivate.  If you inherit publicly from it, then your
// wrapped entity will also offer the NodePrivate interface functions...if you
// inherit protected then you will only offer your own interface.  Private
// inheritance is not possible because to do the casting, the templates must
// be able to tell that your class is a subclass of Node

class Node {
    template <class> friend struct ::std::hash;
    template <class> friend struct ::methyl::structure_hash;
    template <class> friend struct ::methyl::structure_equal_to;

    friend class Engine;


private:
    // Node instances are not to assume they will always be processing
    // the same NodePrivate.  But for implementation convenience, the NodeRef
    // pokes the NodePrivate in before passing on
    mutable NodePrivate * nodePrivateDoNotUseDirectly;
    mutable shared_ptr<Context> contextDoNotUseDirectly;
template <class> friend class NodeRef;
template <class> friend class RootNode;
    void setInternalProperties (
        NodePrivate * nodePrivate,
        shared_ptr<Context> context
    ) {
        // only called by NodeRef.  The reason this is not passed in
        // the constructor is because it would have to come via
        // the derived class, we want compiler default constructors
        nodePrivateDoNotUseDirectly = nodePrivate;
        contextDoNotUseDirectly = context;
    }

    void setInternalProperties (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> context
    ) const
    {
        // we know getNodePrivate() will only return a const node which will
        // restore the constness.  As long as nodePtr is not used
        // directly in its non-const form by this class, we should
        // not trigger undefined behavior...
        nodePrivateDoNotUseDirectly = const_cast<NodePrivate *>(nodePrivate);
        contextDoNotUseDirectly = context;
    }

    void checkValid() const {
        if (not contextDoNotUseDirectly or contextDoNotUseDirectly->isValid())
            return;

        throw hopefullyNotReached(
            "Invalid Node Context",
            contextDoNotUseDirectly->_whereConstructed
        );
    }

private:
    // Node extraction--for friend internal classes only
    NodePrivate & getNodePrivate () {
        checkValid();
        hopefully(nodePrivateDoNotUseDirectly, HERE);
        return *nodePrivateDoNotUseDirectly;
    }

    NodePrivate const & getNodePrivate () const {
        checkValid();
        hopefully(nodePrivateDoNotUseDirectly, HERE);
        return *nodePrivateDoNotUseDirectly;
    }

    NodePrivate * maybeGetNodePrivate () {
        checkValid();
        return nodePrivateDoNotUseDirectly;
    }

    NodePrivate const * maybeGetNodePrivate () const {
        checkValid();
        return nodePrivateDoNotUseDirectly;
    }

    shared_ptr<Context> getContext () const {
        // For copying into new nodes
        return contextDoNotUseDirectly;
    }

    static shared_ptr<Observer> getObserver ();

protected:
    // accessors should not be constructible by anyone but the
    // NodeRef class (how to guarantee that?)
    explicit Node () {}

    // We do not use a virtual destructor here, because derived classes are
    // not supposed to have their own destructors at all--much less should
    // we bend over to try and dispatch to them if they have.  As a sneaky way
    // of catching  people who have casually tried to
    ~Node() noexcept {}


protected:
    // Unfortunately we wind up in accessors and need a NodeRef for the
    // current node when all we have is a this pointer.  Not a perfect
    // solution--could use some more thought
    template <class T>
    NodeRef<T const> thisNodeAs () const {
        return NodeRef<T const>(getNodePrivate(), getContext());
    }

    template <class T>
    NodeRef<T> thisNodeAs () {
        return NodeRef<T>(getNodePrivate(), getContext());
    }


public:
    // read-only accessors
    NodeRef<Node const> getDoc () const {
        return NodeRef<Node const>(
            getNodePrivate().getDoc(),
            getContext()
        );
    }

    NodeRef<Node> getDoc () {
        return NodeRef<Node>(
            getNodePrivate().getDoc(),
            getContext()
        );
    }


    // Extract the Identity of this node.
public:
    Identity getId () const {
        return getNodePrivate().getId();
    }


public:
    // Parent specification
    bool hasParent () const {
        bool result = getNodePrivate().hasParent();
        Observer::observerInEffect()->hasParent(result, getNodePrivate());
        return result;
    }

    NodeRef<Node const> getParent (codeplace const & cp) const {
        NodePrivate const & result = getNodePrivate().getParent(cp);
        Observer::observerInEffect()->getParent(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }

    NodeRef<Node> getParent(codeplace const & cp) {
        NodePrivate & result = getNodePrivate().getParent(cp);
        Observer::observerInEffect()->getParent(result, getNodePrivate());
        return NodeRef<Node>(result, getContext());
    }

    template <class T>
    NodeRef<T const> getParent (codeplace const & cp) const {
        auto result = NodeRef<T>::checked(getParent(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    NodeRef<T> getParent (codeplace const & cp) {
        auto result = NodeRef<T>::checked(getParent(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    optional<NodeRef<Node const>> maybeGetParent () const {
        if (not hasParent())
            return nullopt;
        return getParent(HERE);
    }

    optional<NodeRef<Node>> maybeGetParent () {
        if (not hasParent())
            return nullopt;
        return getParent(HERE);
    }

    template <class T>
    optional<NodeRef<T const>> maybeGetParent () const {
        return NodeRef<T>::checked(maybeGetParent());
    }

    template <class T>
    optional<NodeRef<T>> maybeGetParent() {
        return NodeRef<T>::checked(maybeGetParent());
    }

    Label getLabelInParent (codeplace const & cp) const {
        Label result = getLabelInParent(cp);

        Observer::observerInEffect()->getLabelInParent(result, getNodePrivate());
        return result;
    }

    template <class T>
    bool hasParentEqualTo (NodeRef<T> possibleParent) const {
        // should be a finer-grained micro-observation than this
        optional<NodeRef<Node>> parent = maybeGetParent();
        if (not parent) {
            return false;
        }
        return *parent == possibleParent;

    }

    bool hasLabelInParentEqualTo (
        Label const & possibleLabel,
        codeplace const & cp
    ) const
    {
        // should be a finer-grained observation than this
        Label const labelInParent = getLabelInParent(cp);
        return labelInParent == possibleLabel;
    }

    // Is the right term "direct child" or "immediate child"?
    bool hasImmediateChild (NodeRef<Node> possibleChild) const {
        // hasParent is a good enough observation if false
        if (not possibleChild->hasParent())
            return false;

        // REVIEW: Should have special invalidation, TBD

        NodeRef<Node> parentOfChild = possibleChild->getParent(HERE);
        return parentOfChild.getNode().getNodePrivate() == getNodePrivate();
    }

public:
    // Tag specification
    bool hasTag () const {
        bool result = getNodePrivate().hasTag();
        Observer::observerInEffect()->hasTag(result, getNodePrivate());
        return result;
    }

    Tag getTag (codeplace const & cp) const {
        Tag result = getNodePrivate().getTag(cp);
        Observer::observerInEffect()->getTag(result, getNodePrivate());
        return result;
    }

    optional<NodeRef<Node const>> maybeGetTagNode () const {
        if (not hasTag())
            return nullopt;

        Tag const & tag = getTag(HERE);

        NodePrivate const * optTagNode = NodePrivate::maybeGetFromId(tag);

        if (not optTagNode)
            return nullopt;

        return NodeRef<Node const>(*optTagNode, getContext());
    }

    template <class T>
    optional<NodeRef<T const>> maybeGetTagNode() const {
        return NodeRef<T>::checked(maybeGetTagNode());
    }

    bool hasTagEqualTo (Tag const & possibleTag) const {
        // should be a finer-grained observation than this
        if (not hasTag())
            return false;
        Tag const tag = getTag(HERE);
        return tag == possibleTag;
    }


public:
    // Data accessors - should probably come from a templated lexical cast?
    bool hasText () const { return getNodePrivate().hasText(); }
    QString getText(codeplace const & cp) const {
        QString result = getNodePrivate().getText(cp);
        Observer::observerInEffect()->getText(result, getNodePrivate());
        return result;
    }

    bool hasTextEqualTo (QString const & str) const {
        if (not hasText())
            return false;
        return getText(HERE) == str;
    }


public:

    // label enumeration; ordering is not under user control
    // order is invariant and comes from the label's identity

    bool hasAnyLabels () const {
        bool result = getNodePrivate().hasAnyLabels();
        Observer::observerInEffect()->hasAnyLabels(result, getNodePrivate());
        return result;
    }

    bool hasLabel (Label const & label) const {
        bool result = getNodePrivate().hasLabel(label);
        Observer::observerInEffect()->hasLabel(result, getNodePrivate(), label);
        return result;
    }

    Label getFirstLabel (codeplace const & cp) const {
        Label result = getNodePrivate().getFirstLabel(cp);
        Observer::observerInEffect()->getFirstLabel(result, getNodePrivate());
        return result;
    }

    Label getLastLabel (codeplace const & cp) const {
        Label result = getNodePrivate().getLastLabel(cp);
        Observer::observerInEffect()->getLastLabel(result, getNodePrivate());
        return result;
    }

    bool hasLabelAfter (Label const & label) const {
        bool result = getNodePrivate().hasLabelAfter(label);
        Observer::observerInEffect()->hasLabelAfter(result, getNodePrivate(), label);
        return result;
    }

    Label getLabelAfter (Label const & label, codeplace const & cp) const {
        Label result = getNodePrivate().getLabelAfter(label, cp);
        Observer::observerInEffect()->getLabelAfter(result, getNodePrivate(), label);
        return result;
    }

    optional<Label> maybeGetLabelAfter (Label const & label) const {
        return getNodePrivate().maybeGetLabelAfter(label);
    }

    bool hasLabelBefore (Label const & label) const {
        bool result = getNodePrivate().hasLabelBefore(label);
        Observer::observerInEffect()->hasLabelBefore(result, getNodePrivate(), label);
        return result;
    }

    Label getLabelBefore (Label const & label, codeplace const & cp) const {
        Label result = getNodePrivate().getLabelBefore(label, cp);
        Observer::observerInEffect()->getLabelBefore(result, getNodePrivate(), label);
        return result;
    }

    optional<Label> maybeGetLabelBefore(Label const & label) const {
        return getNodePrivate().maybeGetLabelBefore(label);
    }

    // node in label enumeration
public:

    ///
    /// FirstChildInLabel
    ///

    NodeRef<Node const> getFirstChildInLabel (
        Label const & label, codeplace const & cp
    ) const
    {
        auto & result = getNodePrivate().getFirstChildInLabel(label, cp);
        Observer::observerInEffect()->getFirstChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node const>(result, getContext());
    }

    NodeRef<Node> getFirstChildInLabel (
        Label const & label, codeplace const & cp
    ) {
        auto & result = getNodePrivate().getFirstChildInLabel(label, cp);
        Observer::observerInEffect()->getFirstChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node>(result, getContext());
    }

    auto maybeGetFirstChildInLabel (Label const & label) const
        -> optional<NodeRef<Node const>>
    {
        optional<NodeRef<Node const>> result;
        if (hasLabel(label))
            result = getFirstChildInLabel(label, HERE);
        return result;
    }

    auto maybeGetFirstChildInLabel (Label const & label)
        -> optional<NodeRef<Node>>
    {
        optional<NodeRef<Node>> result;
        if (hasLabel(label))
            result = getFirstChildInLabel(label, HERE);
        return result;
    }

    template <class T>
    NodeRef<T const> getFirstChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        auto result = NodeRef<T>::checked(getFirstChildInLabel(label, cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    NodeRef<T> getFirstChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        auto result = NodeRef<T>::checked(getFirstChildInLabel(label, cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybeGetFirstChildInLabel (Label const & label) const
        -> optional<NodeRef<T const>>
    {
        return NodeRef<T>::checked(maybeGetFirstChildInLabel(label));
    }

    template <class T>
    auto maybeGetFirstChildInLabel (Label const & label)
        -> optional<NodeRef<T>>
    {
        return NodeRef<T>::checked(maybeGetFirstChildInLabel(label));
    }


    ///
    /// LastChildInLabel
    ///

    NodeRef<Node const> getLastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        auto & result = getNodePrivate().getLastChildInLabel(label, cp);
        Observer::observerInEffect()->getLastChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node const>(result, getContext());
    }

    NodeRef<Node> getLastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        NodePrivate & result = getNodePrivate().getLastChildInLabel(label, cp);
        Observer::observerInEffect()->getLastChildInLabel(result, getNodePrivate(), label);
        return NodeRef<Node>(result, getContext());
    }

    auto maybeGetLastChildInLabel (Label const & label) const
        -> optional<NodeRef<Node const>>
    {
        if (not hasLabel(label))
            return nullopt;
        return getLastChildInLabel(label, HERE);
    }

    auto maybeGetLastChildInLabel (Label const & label)
        -> optional<NodeRef<Node>>
    {
        if (not hasLabel(label))
            return nullopt;
        return getLastChildInLabel(label, HERE);
    }

    template <class T>
    NodeRef<T const> getLastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        return NodeRef<T>::checked(getLastChildInLabel(label, cp));
    }

    template <class T>
    NodeRef<T> getLastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        return NodeRef<T>::checked(getLastChildInLabel(label, cp));
    }

    template <class T>
    auto maybeGetLastChildInLabel (Label const & label) const
        -> optional<NodeRef<T const>>
    {
        return NodeRef<T>::checked(maybeGetLastChildInLabel(label));
    }

    template <class T>
    auto maybeGetLastChildInLabel (Label const & label)
        -> optional<NodeRef<T>>
    {
        return NodeRef<T>::checked(maybeGetLastChildInLabel(label));
    }


    ///
    /// NextSiblingInLabel
    ///

    bool hasNextSiblingInLabel () const {
        bool result = getNodePrivate().hasNextSiblingInLabel();
        Observer::observerInEffect()->hasNextSiblingInLabel(result, getNodePrivate());
        return result;
    }

    NodeRef<Node const> getNextSiblingInLabel (codeplace const & cp) const {
        NodePrivate const & result = getNodePrivate().getNextSiblingInLabel(cp);
        Observer::observerInEffect()->getNextSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }

    NodeRef<Node> getNextSiblingInLabel (codeplace const & cp) {
        NodePrivate & result = getNodePrivate().getNextSiblingInLabel(cp);
        Observer::observerInEffect()->getNextSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node>(result, getContext());
    }

    optional<NodeRef<Node const>> maybeGetNextSiblingInLabel () const {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return getNextSiblingInLabel(HERE);
    }

    optional<NodeRef<Node>> maybeGetNextSiblingInLabel () {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return getNextSiblingInLabel(HERE);
    }


    template <class T>
    NodeRef<T const> getNextSiblingInLabel (codeplace const & cp) const {
        auto result = NodeRef<T>::checked(getNextSiblingInLabel(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    NodeRef<T> getNextSiblingInLabel (codeplace const & cp) {
        auto result = NodeRef<T>::checked(getNextSiblingInLabel(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybeGetNextSiblingInLabel () const
        -> optional<NodeRef<T const>>
    {
        return NodeRef<T>::checked(maybeGetNextSiblingInLabel());
    }

    template <class T>
    auto maybeGetNextSiblingInLabel ()
        -> optional<NodeRef<T>>
    {
        return NodeRef<T>::checked(maybeGetNextSiblingInLabel());
    }


    ///
    /// PreviousSiblingInLabel
    ///

    bool hasPreviousSiblingInLabel () const {
        bool result = getNodePrivate().hasPreviousSiblingInLabel();
        Observer::observerInEffect()->hasPreviousSiblingInLabel(result, getNodePrivate());
        return result;
    }

    auto getPreviousSiblingInLabel (codeplace const & cp) const
        -> NodeRef<Node const>
    {
        auto & result = getNodePrivate().getPreviousSiblingInLabel(cp);
        Observer::observerInEffect()->getPreviousSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }

    NodeRef<Node> getPreviousSiblingInLabel (codeplace const & cp) {
        NodePrivate & result = getNodePrivate().getPreviousSiblingInLabel(cp);
        Observer::observerInEffect()->getPreviousSiblingInLabel(result, getNodePrivate());
        return NodeRef<Node const>(result, getContext());
    }

    optional<NodeRef<Node const>> maybeGetPreviousSiblingInLabel () const {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return getPreviousSiblingInLabel(HERE);
    }
    optional<NodeRef<Node>> maybeGetPreviousSiblingInLabel () {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return getPreviousSiblingInLabel(HERE);
    }

    template <class T>
    NodeRef<T const> getPreviousSiblingInLabel (codeplace const & cp) const {
        auto result = NodeRef<T>::checked(maybeGetPreviousSiblingInLabel());
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    NodeRef<T> getPreviousSiblingInLabel (codeplace const & cp) {
        auto result = NodeRef<T>::checked(maybeGetPreviousSiblingInLabel());
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybeGetPreviousSiblingInLabel () const
        -> optional<NodeRef<T const>>
    {
        return NodeRef<T>::checked(maybeGetPreviousSiblingInLabel());
    }

    template <class T>
    auto maybeGetPreviousSiblingInLabel ()
        -> optional<NodeRef<T>>
    {
        return NodeRef<T>::checked(maybeGetPreviousSiblingInLabel());
    }


    ///
    /// Child Set Accessors
    ///

    // special accessor for getting children without counting as an observation
    // of their order.  (it does count as an observation of the number of
    // children, obviously).  Implementation lost in a refactoring, add with
    // other missing micro-observations

    std::unordered_set<NodeRef<Node const>> getChildSetForLabel () const {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<NodeRef<Node const>>();
    }

    std::unordered_set<NodeRef<Node>> getChildSetForLabel () {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<NodeRef<Node>>();
    }

    // structural modifications
public:
    void setTag (Tag const & tag) {
        getNodePrivate().setTag(tag);
        Observer::observerInEffect()->setTag(getNodePrivate(), tag);
        return;
    }

public:
    template <class T>
    NodeRef<T> insertChildAsFirstInLabel (
        RootNode<T> newChild,
        Label const & label
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result
            = getNodePrivate().insertChildAsFirstInLabel(
                std::move(newChild.extractNodePrivate()), label
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);
        hopefully(info._previousChild == nullptr, HERE);

        Observer::observerInEffect()->insertChildAsFirstInLabel(
            getNodePrivate(),
            nodeRef,
            info._labelInParent,
            info._nextChild
        );
        return NodeRef<T> (nodeRef, getContext());
    }

    template <class T>
    NodeRef<T> insertChildAsLastInLabel (
        RootNode<T> newChild,
        Label const & label
    ) {
        NodePrivate const * previousChildInLabel;
        tuple<NodePrivate &, NodePrivate::insert_info> result
            = getNodePrivate().insertChildAsLastInLabel(
                std::move(newChild.extractNodePrivate()), label
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);
        hopefully(info._nextChild == nullptr, HERE);

        Observer::observerInEffect()->insertChildAsLastInLabel(
            getNodePrivate(),
            nodeRef,
            info._labelInParent,
            info._previousChild
        );
        return NodeRef<T> (nodeRef, getContext());
    }

    template <class T>
    NodeRef<T> insertSiblingAfter (
        RootNode<T> newSibling
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result =
            getNodePrivate().insertSiblingAfter(
                std::move(newSibling.extractNodePrivate())
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);

        if (not info._nextChild) {
            Observer::observerInEffect()->insertChildAsLastInLabel(
                *info._nodeParent,
                nodeRef,
                info._labelInParent,
                &getNodePrivate()
            );
        } else {
            Observer::observerInEffect()->insertChildBetween(
                *info._nodeParent,
                nodeRef,
                getNodePrivate(),
                *info._nextChild
            );
        }

        return NodeRef<T> (result, getContext());
    }

    template <class T>
    NodeRef<T> insertSiblingBefore (
        RootNode<T> newSibling
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result =
            getNodePrivate().insertSiblingBefore(
                std::move(newSibling.extractNodePrivate())
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);

        if (not info._previousChild) {
            Observer::observerInEffect()->insertChildAsLastInLabel(
                *info._nodeParent,
                nodeRef,
                info._labelInParent,
                &getNodePrivate()
            );
        } else {
            Observer::observerInEffect()->insertChildBetween(
                *info._nodeParent,
                nodeRef,
                *info._previousChild,
                getNodePrivate()
            );
        }

        return NodeRef<T> (nodeRef, getContext());
    }

    auto detachAnyChildrenInLabel (Label const & label)
        -> std::vector<RootNode<Node>>
    {
        std::vector<RootNode<Node>> children;
        while (hasLabel(label)) {
            children.push_back(getFirstChildInLabel(label, HERE).detach());
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
    void setText (QString const & str) {
        getNodePrivate().setText(str);
        Observer::observerInEffect()->setText(getNodePrivate(), str);
    }

    void insertCharBeforeIndex (
        int index,
        QChar const & ch,
        codeplace const & cp
    ) {
        QString str = getNodePrivate().getText(cp);
        str.insert(index, ch);
        getNodePrivate().setText(str);
        Observer::observerInEffect()->setText(getNodePrivate(), str);
    }

    void insertCharAfterIndex (
        int index,
        QChar const & ch,
        codeplace const & cp
    ) {
        QString str = getNodePrivate().getText(cp);
        str.insert(index + 1, ch);
        getNodePrivate().setText(str);
        Observer::observerInEffect()->setText(getNodePrivate(), str);
    }

    void deleteCharAtIndex (int index, codeplace const & cp) {
        QString str = getNodePrivate().getText(cp);
        str.remove(index, 1);
        getNodePrivate().setText(str);
        Observer::observerInEffect()->setText(getNodePrivate(), str);
    }

public:
    bool sameStructureAs (NodeRef<Node const> other) const
    {
        return 0 == getNodePrivate().compare(
            other.getNode().getNodePrivate()
        );
    }


    // See remarks above about not being sure if I've picked the absolute right
    // invariants for -1 vs +1.  This is going to be canon...encoded in file
    // formats and stuff, it should be gotten right!
    template <class OtherT>
    bool lowerStructureRankThan (NodeRef<OtherT const> other) const
    {
        return -1 == getNodePrivate().compare(
            other.getNode().getNodePrivate()
        );
    }

    ///
    /// Downcasting assistance
    ///
protected:
    virtual bool check() const {
        // This checks to the *most derived* structure in the accessor
        // It will not call base class checks on the structure, you can
        // do that yourself though in your override.
        return true;
    }

};


// At the moment, there is a standard label for the name of a node.
extern const Label globalLabelName;

// We also create a special class to represent a notion of "Emptiness"
// that can have no tags.  I thought it was a unique enough name that isn't
// overloaded yet still has meaning.  (Terminal is weird, Terminator is weird,
// going back to the chemical analogy and calling it "Hydrogen" since it
// "terminates the change and can no longer bond" is even more bonkers.)

class Emptiness : public methyl::Node {
public:
    static RootNode<Emptiness> create() {
        return RootNode<Emptiness>::createText("");
    }

    bool check() const override
    {
        return hasTextEqualTo("");
    }
};

// Error signals in benzene are really just Benzene node trees
// This means some contexts may choose to place the errors into the document
// But if an error is returned to the UI, it will render it

extern const Tag globalTagError; // all errors should have this tag.
extern const Tag globalTagCancellation; // does this need a node too?
extern const Label globalLabelCausedBy;

class Error : public methyl::Node
{
public:
    static methyl::RootNode<Error> makeCancellation ();

public:
    bool wasCausedByCancellation () const;
    QString getDescription () const;
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
