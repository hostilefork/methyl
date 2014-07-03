//
// accessor.h
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

#ifndef METHYL_ACCESSOR_H
#define METHYL_ACCESSOR_H

#include <QUuid>
#include <QDebug>

#include <unordered_set>

#include "methyl/defs.h"
#include "methyl/nodeprivate.h"
#include "methyl/observer.h"
#include "methyl/context.h"
#include "methyl/node.h"
#include "methyl/tree.h"

// Don't want a dependency on the engine.h file in node.h
// Have to do some acrobatics to get around that
// Long term, nodeprivate/observer/context should be hidden
// as opaque types so clients can be insulated from the details

namespace std {

    // Need this to get std::unordered_set and unordered_map to work on Node
    // http://stackoverflow.com/questions/8157937/

    template <class T>
    struct hash<methyl::Node<T>>
    {
        size_t operator()(
            methyl::Node<T> const & nodeRef
        ) const
        {
            return std::hash<methyl::NodePrivate const *>()(
                &nodeRef.accessor().nodePrivate()
            );
        }
    };

    // Need this to get std::unordered_set and unordered_map to work on Tree
    // http://stackoverflow.com/questions/8157937/

    template <class T>
    struct hash<methyl::Tree<T>>
    {
        size_t operator()(
            methyl::Tree<T> const & nodeRef
        ) const
        {
            // This algorithm is a placeholder, and needs serious improvement.
            //
            // https://github.com/hostilefork/methyl/issues/32

            methyl::NodePrivate const & nodePrivate
                = nodeRef.accessor().nodePrivate();

            size_t result = 0;
            int const N = -1; // For now, let's do full equality...
            int i = 0;

            std::hash<methyl::Tag> tagHasher;

            methyl::NodePrivate const * current = &nodePrivate;
            while (current and (i++ < N)) {
                // Qt will not support std::hash until at least 2015
                // https://bugreports.qt-project.org/browse/QTBUG-33428

                if (current->hasText()) {
                    result ^= qHash(current->text(HERE));
                } else {
                    result ^= tagHasher(current->tag(HERE));
                }
                current = current->maybeNextPreorderNodeUnderRoot(nodePrivate);
            }

            return result;
        }
    };
}



namespace methyl {

class Engine;
class Observer;


// Accessor is the base for our proxy classes.  They provide a compound
// interface to NodePrivate.  If you inherit publicly from it, then your
// wrapped entity will also offer the NodePrivate interface functions...if you
// inherit protected then you will only offer your own interface.  Private
// inheritance is not possible because to do the casting, the templates must
// be able to tell that your class is a subclass of Node

class Accessor {
    template <class> friend struct ::std::hash;

    friend class Engine;
    friend class Observer;


private:
    // Accessor instances are not to assume they will always be processing
    // the same NodePrivate.  But for implementation convenience, the NodeRef
    // pokes the NodePrivate in before passing on
    mutable NodePrivate * _nodePrivateDoNotUseDirectly;
    mutable shared_ptr<Context> _contextDoNotUseDirectly;
template <class> friend class Node;
template <class> friend class Tree;

    // only called by NodeRef.  The reason this is not passed in
    // the constructor is because it would have to come via
    // the derived class, we want compiler default constructors
    void setInternalProperties (
        NodePrivate * nodePrivate,
        shared_ptr<Context> const & context
    ) {
        _nodePrivateDoNotUseDirectly = nodePrivate;
        _contextDoNotUseDirectly = context;
    }
    // ...
    void setInternalProperties (
        NodePrivate * nodePrivate,
        shared_ptr<Context> && context
    ) {
        _nodePrivateDoNotUseDirectly = nodePrivate;
        _contextDoNotUseDirectly = std::move(context);
    }


    // we know nodePrivate() will only return a const node which will
    // restore the constness.  As long as nodePtr is not used
    // directly in its non-const form by this class, we should
    // not trigger undefined behavior...
    void setInternalProperties (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> const & context
    ) const
    {
        _nodePrivateDoNotUseDirectly = const_cast<NodePrivate *>(nodePrivate);
        _contextDoNotUseDirectly = context;
    }
    // ...
    void setInternalProperties (
        NodePrivate const * nodePrivate,
        shared_ptr<Context> && context
    ) const
    {
        _nodePrivateDoNotUseDirectly = const_cast<NodePrivate *>(nodePrivate);
        _contextDoNotUseDirectly = std::move(context);
    }


private:
    void checkValid () const {
        if (not _contextDoNotUseDirectly or _contextDoNotUseDirectly->isValid())
            return;

        throw hopefullyNotReached(
            "Invalid Accessor Context",
            _contextDoNotUseDirectly->_whereConstructed
        );
    }

    // Accessor extraction--for friend internal classes only
    NodePrivate & nodePrivate () {
        checkValid();

        static auto nullNodePrivateCodeplace = HERE;
        hopefully(_nodePrivateDoNotUseDirectly, nullNodePrivateCodeplace);

        return *_nodePrivateDoNotUseDirectly;
    }

    NodePrivate const & nodePrivate () const {
        checkValid();

        static auto nullNodePrivateCodeplace = HERE;
        hopefully(_nodePrivateDoNotUseDirectly, nullNodePrivateCodeplace);

        return *_nodePrivateDoNotUseDirectly;
    }

    NodePrivate * maybeNodePrivate () {
        checkValid();
        return _nodePrivateDoNotUseDirectly;
    }

    NodePrivate const * maybeNodePrivate () const {
        checkValid();
        return _nodePrivateDoNotUseDirectly;
    }

    shared_ptr<Context> const & context () const {
        // For copying into new nodes
        return _contextDoNotUseDirectly;
    }


protected:
    // accessors should not be constructible by anyone but the
    // NodeRef class (how to guarantee that?)
    explicit Accessor () {}

    // We do not use a virtual destructor here, because derived classes are
    // not supposed to have their own destructors at all--much less should
    // we bend over to try and dispatch to them if they have.  As a sneaky way
    // of catching  people who have casually tried to
    ~Accessor () {}


protected:
    // Unfortunately we wind up in accessors and need a NodeRef for the
    // current node when all we have is a this pointer.  Not a perfect
    // solution--could use some more thought
    template <class T>
    Node<T const> thisNodeAs () const {
        return Node<T const>(nodePrivate(), context());
    }

    template <class T>
    Node<T> thisNodeAs () {
        return Node<T>(nodePrivate(), context());
    }


public:
    // read-only accessors
    Node<Accessor const> root () const {
        // Currently there is no specialized observer for seeing the root
        // of something; even though there's a fast operation for finding
        // the root in NodePrivate.  So we have to register observation
        // of every parent link on behalf of the client.

        Node<Accessor const> current = thisNodeAs<Accessor>();
        while (current->hasParent()) {
            current = current->parent(HERE);
        }
        return current;
    }

    Node<Accessor> root () {
        Node<Accessor> current = thisNodeAs<Accessor>();
        while (current->hasParent()) {
            current = current->parent(HERE);
        }
        return current;
    }


    // Extract the Identity of this node.
public:
    Identity identity () const {
        return nodePrivate().identity();
    }


public:
    // Parent specification
    bool hasParent () const {
        bool result = nodePrivate().hasParent();
        Observer::current().hasParent(result, nodePrivate());
        return result;
    }

    Node<Accessor const> parent (codeplace const & cp) const {
        NodePrivate const & result = nodePrivate().parent(cp);
        Observer::current().parent(result, nodePrivate());
        return Node<Accessor const>(result, context());
    }

    Node<Accessor> parent(codeplace const & cp) {
        NodePrivate & result = nodePrivate().parent(cp);
        Observer::current().parent(result, nodePrivate());
        return Node<Accessor>(result, context());
    }

    template <class T>
    Node<T const> parent (codeplace const & cp) const {
        auto result = Node<T>::checked(parent(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    Node<T> parent (codeplace const & cp) {
        auto result = Node<T>::checked(parent(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    optional<Node<Accessor const>> maybeParent () const {
        if (not hasParent())
            return nullopt;
        return parent(HERE);
    }

    optional<Node<Accessor>> maybeParent () {
        if (not hasParent())
            return nullopt;
        return parent(HERE);
    }

    template <class T>
    optional<Node<T const>> maybeParent () const {
        return Node<T>::checked(maybeParent());
    }

    template <class T>
    optional<Node<T>> maybeParent() {
        return Node<T>::checked(maybeParent());
    }

    Label labelInParent (codeplace const & cp) const {
        Label result = labelInParent(cp);

        Observer::current().labelInParent(result, nodePrivate());
        return result;
    }

    template <class T>
    bool hasParentEqualTo (Node<T> possibleParent) const {
        // should be a finer-grained micro-observation than this
        optional<Node<Accessor>> parent = maybeParent();
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
        Label label = labelInParent(cp);
        return label == possibleLabel;
    }

    // Is the right term "direct child" or "immediate child"?
    bool hasImmediateChild (Node<Accessor const> const & possibleChild) const {
        // hasParent is a good enough observation if false
        if (not possibleChild->hasParent())
            return false;

        // REVIEW: Should have special invalidation, TBD

        Node<Accessor const> parentOfChild = possibleChild->parent(HERE);
        return parentOfChild.accessor().nodePrivate() == nodePrivate();
    }

public:
    // Tag specification
    bool hasTag () const {
        bool result = nodePrivate().hasTag();
        Observer::current().hasTag(result, nodePrivate());
        return result;
    }

    Tag tag (codeplace const & cp) const {
        Tag result = nodePrivate().tag(cp);
        Observer::current().tag(result, nodePrivate());
        return result;
    }

    optional<Node<Accessor const>> maybeLookupTagNode() const {
        if (not hasTag())
            return nullopt;

        optional<Identity> id = tag(HERE).maybeAsIdentity();
        if (not id)
            return nullopt;

        optional<NodePrivate const &> tagNode
            = NodePrivate::maybeGetFromId(*id);

        if (not tagNode)
            return nullopt;

        return Node<Accessor const>(*tagNode, context());
    }

    template <class T>
    optional<Node<T const>> maybeLookupTagNode () const {
        return Node<T>::checked(maybeLookupTagNode());
    }

    bool hasTagEqualTo (Tag const & possibleTag) const {
        // should be a finer-grained observation than this
        if (not hasTag())
            return false;

        return tag(HERE) == possibleTag;
    }


public:
    // Data accessors
    
    bool hasText () const {
        return nodePrivate().hasText();
    }

    QString text (codeplace const & cp) const {
        QString result = nodePrivate().text(cp);
        Observer::current().text(result, nodePrivate());
        return result;
    }

    bool hasTextEqualTo (QString const & str) const {
        if (not hasText())
            return false;
        return text(HERE) == str;
    }


public:

    // label enumeration; ordering is not under user control
    // order is invariant and comes from the label's identity

    bool hasAnyLabels () const {
        bool result = nodePrivate().hasAnyLabels();
        Observer::current().hasAnyLabels(result, nodePrivate());
        return result;
    }

    bool hasLabel (Label const & label) const {
        bool result = nodePrivate().hasLabel(label);
        Observer::current().hasLabel(result, nodePrivate(), label);
        return result;
    }

    Label firstLabel (codeplace const & cp) const {
        Label result = nodePrivate().firstLabel(cp);
        Observer::current().firstLabel(result, nodePrivate());
        return result;
    }

    Label lastLabel (codeplace const & cp) const {
        Label result = nodePrivate().lastLabel(cp);
        Observer::current().lastLabel(result, nodePrivate());
        return result;
    }

    bool hasLabelAfter (Label const & label, codeplace const & cp) const {
        bool result = nodePrivate().hasLabelAfter(label, cp);
        Observer::current().hasLabelAfter(result, nodePrivate(), label);
        return result;
    }

    Label labelAfter (Label const & label, codeplace const & cp) const {
        Label result = nodePrivate().labelAfter(label, cp);
        Observer::current().labelAfter(result, nodePrivate(), label);
        return result;
    }

    optional<Label> maybeLabelAfter (
        Label const & label,
        codeplace const & cp
    ) const
    {
        return nodePrivate().maybeLabelAfter(label, cp);
    }

    bool hasLabelBefore (Label const & label, codeplace const & cp) const {
        bool result = nodePrivate().hasLabelBefore(label, cp);
        Observer::current().hasLabelBefore(result, nodePrivate(), label);
        return result;
    }

    Label labelBefore (Label const & label, codeplace const & cp) const {
        Label result = nodePrivate().labelBefore(label, cp);
        Observer::current().labelBefore(result, nodePrivate(), label);
        return result;
    }

    optional<Label> maybeLabelBefore (
        Label const & label,
        codeplace const & cp
    ) const
    {
        return nodePrivate().maybeLabelBefore(label, cp);
    }

    // node in label enumeration
public:

    ///
    /// FirstChildInLabel
    ///

    Node<Accessor const> firstChildInLabel (
        Label const & label, codeplace const & cp
    ) const
    {
        auto & result = nodePrivate().firstChildInLabel(label, cp);
        Observer::current().firstChildInLabel(result, nodePrivate(), label);
        return Node<Accessor const>(result, context());
    }

    Node<Accessor> firstChildInLabel (
        Label const & label, codeplace const & cp
    ) {
        auto & result = nodePrivate().firstChildInLabel(label, cp);
        Observer::current().firstChildInLabel(result, nodePrivate(), label);
        return Node<Accessor>(result, context());
    }

    auto maybeFirstChildInLabel (Label const & label) const
        -> optional<Node<Accessor const>>
    {
        optional<Node<Accessor const>> result;
        if (hasLabel(label))
            result = firstChildInLabel(label, HERE);
        return result;
    }

    auto maybeFirstChildInLabel (Label const & label)
        -> optional<Node<Accessor>>
    {
        optional<Node<Accessor>> result;
        if (hasLabel(label))
            result = firstChildInLabel(label, HERE);
        return result;
    }

    template <class T>
    Node<T const> firstChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        auto result = Node<T>::checked(firstChildInLabel(label, cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    Node<T> firstChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        auto result = Node<T>::checked(firstChildInLabel(label, cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybeFirstChildInLabel (Label const & label) const
        -> optional<Node<T const>>
    {
        return Node<T>::checked(maybeFirstChildInLabel(label));
    }

    template <class T>
    auto maybeFirstChildInLabel (Label const & label)
        -> optional<Node<T>>
    {
        return Node<T>::checked(maybeFirstChildInLabel(label));
    }


    ///
    /// LastChildInLabel
    ///

    Node<Accessor const> lastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        auto & result = nodePrivate().lastChildInLabel(label, cp);
        Observer::current().lastChildInLabel(result, nodePrivate(), label);
        return Node<Accessor const>(result, context());
    }

    Node<Accessor> lastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        NodePrivate & result = nodePrivate().lastChildInLabel(label, cp);
        Observer::current().lastChildInLabel(result, nodePrivate(), label);
        return Node<Accessor>(result, context());
    }

    auto maybeLastChildInLabel (Label const & label) const
        -> optional<Node<Accessor const>>
    {
        if (not hasLabel(label))
            return nullopt;
        return lastChildInLabel(label, HERE);
    }

    auto maybeLastChildInLabel (Label const & label)
        -> optional<Node<Accessor>>
    {
        if (not hasLabel(label))
            return nullopt;
        return lastChildInLabel(label, HERE);
    }

    template <class T>
    Node<T const> lastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) const
    {
        return Node<T>::checked(lastChildInLabel(label, cp));
    }

    template <class T>
    Node<T> lastChildInLabel (
        Label const & label,
        codeplace const & cp
    ) {
        return Node<T>::checked(lastChildInLabel(label, cp));
    }

    template <class T>
    auto maybeLastChildInLabel (Label const & label) const
        -> optional<Node<T const>>
    {
        return Node<T>::checked(maybeLastChildInLabel(label));
    }

    template <class T>
    auto maybeLastChildInLabel (Label const & label)
        -> optional<Node<T>>
    {
        return Node<T>::checked(maybeLastChildInLabel(label));
    }


    ///
    /// NextSiblingInLabel
    ///

    bool hasNextSiblingInLabel () const {
        bool result = nodePrivate().hasNextSiblingInLabel();
        Observer::current().hasNextSiblingInLabel(result, nodePrivate());
        return result;
    }

    Node<Accessor const> nextSiblingInLabel (codeplace const & cp) const {
        NodePrivate const & result = nodePrivate().nextSiblingInLabel(cp);
        Observer::current().nextSiblingInLabel(result, nodePrivate());
        return Node<Accessor const>(result, context());
    }

    Node<Accessor> nextSiblingInLabel (codeplace const & cp) {
        NodePrivate & result = nodePrivate().nextSiblingInLabel(cp);
        Observer::current().nextSiblingInLabel(result, nodePrivate());
        return Node<Accessor>(result, context());
    }

    optional<Node<Accessor const>> maybeNextSiblingInLabel () const {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return nextSiblingInLabel(HERE);
    }

    optional<Node<Accessor>> maybeNextSiblingInLabel () {
        if (not hasNextSiblingInLabel())
            return nullopt;
        return nextSiblingInLabel(HERE);
    }


    template <class T>
    Node<T const> nextSiblingInLabel (codeplace const & cp) const {
        auto result = Node<T>::checked(nextSiblingInLabel(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    Node<T> nextSiblingInLabel (codeplace const & cp) {
        auto result = Node<T>::checked(nextSiblingInLabel(cp));
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybeNextSiblingInLabel () const
        -> optional<Node<T const>>
    {
        return Node<T>::checked(maybeNextSiblingInLabel());
    }

    template <class T>
    auto maybeNextSiblingInLabel ()
        -> optional<Node<T>>
    {
        return Node<T>::checked(maybeNextSiblingInLabel());
    }


    ///
    /// PreviousSiblingInLabel
    ///

    bool hasPreviousSiblingInLabel () const {
        bool result = nodePrivate().hasPreviousSiblingInLabel();
        Observer::current().hasPreviousSiblingInLabel(result, nodePrivate());
        return result;
    }

    auto previousSiblingInLabel (codeplace const & cp) const
        -> Node<Accessor const>
    {
        auto & result = nodePrivate().previousSiblingInLabel(cp);
        Observer::current().previousSiblingInLabel(result, nodePrivate());
        return Node<Accessor const>(result, context());
    }

    Node<Accessor> previousSiblingInLabel (codeplace const & cp) {
        NodePrivate & result = nodePrivate().previousSiblingInLabel(cp);
        Observer::current().previousSiblingInLabel(result, nodePrivate());
        return Node<Accessor>(result, context());
    }

    optional<Node<Accessor const>> maybePreviousSiblingInLabel () const {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return previousSiblingInLabel(HERE);
    }

    optional<Node<Accessor>> maybePreviousSiblingInLabel () {
        if (not hasPreviousSiblingInLabel())
            return nullopt;
        return previousSiblingInLabel(HERE);
    }

    template <class T>
    Node<T const> previousSiblingInLabel (codeplace const & cp) const {
        auto result = Node<T>::checked(maybePreviousSiblingInLabel());
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    Node<T> previousSiblingInLabel (codeplace const & cp) {
        auto result = Node<T>::checked(maybePreviousSiblingInLabel());
        hopefully(result != nullopt, cp);
        return *result;
    }

    template <class T>
    auto maybePreviousSiblingInLabel () const
        -> optional<Node<T const>>
    {
        return Node<T>::checked(maybePreviousSiblingInLabel());
    }

    template <class T>
    auto maybePreviousSiblingInLabel ()
        -> optional<Node<T>>
    {
        return Node<T>::checked(maybePreviousSiblingInLabel());
    }


    ///
    /// Child Set Accessors
    ///

    // special accessor for getting children without counting as an observation
    // of their order.  (it does count as an observation of the number of
    // children, obviously).  Implementation lost in a refactoring, add with
    // other missing micro-observations

    std::unordered_set<Node<Accessor const>> childSetForLabel () const {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<Node<Accessor const>>();
    }

    std::unordered_set<Node<Accessor>> childSetForLabel () {
        hopefullyNotReached("Not implemented yet", HERE);
        return std::unordered_set<Node<Accessor>>();
    }

    // structural modifications
public:
    void setTag (Tag const & tag) {
        nodePrivate().setTag(tag);
        Observer::current().setTag(nodePrivate(), tag);
        return;
    }

public:
    template <class T>
    Node<T> insertChildAsFirstInLabel (
        Tree<T> && newChild,
        Label const & label
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result
            = nodePrivate().insertChildAsFirstInLabel(
                std::move(newChild.extractNodePrivate()), label
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);
        hopefully(info._previousChild == nullptr, HERE);

        Observer::current().insertChildAsFirstInLabel(
            nodePrivate(),
            nodeRef,
            info._labelInParent,
            info._nextChild
        );
        return Node<T> (nodeRef, context());
    }

    template <class T>
    Node<T> insertChildAsLastInLabel (
        Tree<T> && newChild,
        Label const & label
    ) {
        NodePrivate const * previousChildInLabel;
        tuple<NodePrivate &, NodePrivate::insert_info> result
            = nodePrivate().insertChildAsLastInLabel(
                std::move(newChild.extractNodePrivate()), label
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);
        hopefully(info._nextChild == nullptr, HERE);

        Observer::current().insertChildAsLastInLabel(
            nodePrivate(),
            nodeRef,
            info._labelInParent,
            info._previousChild
        );
        return Node<T> (nodeRef, context());
    }

    template <class T>
    Node<T> insertSiblingAfter (
        Tree<T> && newSibling
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result =
            nodePrivate().insertSiblingAfter(
                std::move(newSibling.extractNodePrivate())
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);

        if (not info._nextChild) {
            Observer::current().insertChildAsLastInLabel(
                *info._nodeParent,
                nodeRef,
                info._labelInParent,
                &nodePrivate()
            );
        } else {
            Observer::current().insertChildBetween(
                *info._nodeParent,
                nodeRef,
                nodePrivate(),
                *info._nextChild
            );
        }

        return Node<T> (result, context());
    }

    template <class T>
    Node<T> insertSiblingBefore (
        Tree<T> && newSibling
    ) {
        tuple<NodePrivate &, NodePrivate::insert_info> result =
            nodePrivate().insertSiblingBefore(
                std::move(newSibling.extractNodePrivate())
            );

        NodePrivate & nodeRef = std::get<0>(result);

        NodePrivate::insert_info & info = std::get<1>(result);

        if (not info._previousChild) {
            Observer::current().insertChildAsLastInLabel(
                *info._nodeParent,
                nodeRef,
                info._labelInParent,
                &nodePrivate()
            );
        } else {
            Observer::current().insertChildBetween(
                *info._nodeParent,
                nodeRef,
                *info._previousChild,
                nodePrivate()
            );
        }

        return Node<T> (nodeRef, context());
    }

    auto detachAnyChildrenInLabel (Label const & label)
        -> std::vector<Tree<Accessor>>
    {
        std::vector<Tree<Accessor>> children;
        while (hasLabel(label)) {
            children.push_back(firstChildInLabel(label, HERE).detach());
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
        nodePrivate().setText(str);
        Observer::current().setText(nodePrivate(), str);
    }

    void insertCharBeforeIndex (
        int index,
        QChar const & ch,
        codeplace const & cp
    ) {
        QString str = nodePrivate().text(cp);
        str.insert(index, ch);
        nodePrivate().setText(str);
        Observer::current().setText(nodePrivate(), str);
    }

    void insertCharAfterIndex (
        int index,
        QChar const & ch,
        codeplace const & cp
    ) {
        QString str = nodePrivate().text(cp);
        str.insert(index + 1, ch);
        nodePrivate().setText(str);
        Observer::current().setText(nodePrivate(), str);
    }

    void deleteCharAtIndex (int index, codeplace const & cp) {
        QString str = nodePrivate().text(cp);
        str.remove(index, 1);
        nodePrivate().setText(str);
        Observer::current().setText(nodePrivate(), str);
    }

public:
    bool isSubtreeCongruentTo (Node<Accessor const> const & other) const
    {
        return 0 == nodePrivate().compare(
            other.accessor().nodePrivate()
        );
    }


    // See remarks above about not being sure if I've picked the absolute right
    // invariants for -1 vs +1.  This is going to be canon...encoded in file
    // formats and stuff, it should be gotten right!
    template <class OtherT>
    bool lowerStructureRankThan (Node<OtherT const> const & other) const
    {
        return -1 == nodePrivate().compare(
            other.accessor().nodePrivate()
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

class Emptiness : public methyl::Accessor {
public:
    static Tree<Emptiness> create() {
        return Tree<Emptiness>::createText("");
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

class Error : public methyl::Accessor
{
public:
    static methyl::Tree<Error> makeCancellation ();

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
// of using optional<Node<T>> would defer to runtime "optional-ness" in
// the baseline declaration of node references that we wish to avoid.  Hence
// because they insist, you can only pass optional<Node<T>> as signal or
// slot parameters.

Q_DECLARE_METATYPE(optional<methyl::Node<methyl::Accessor const>>)
Q_DECLARE_METATYPE(optional<methyl::Node<methyl::Accessor>>)

// Could this be used?  I don't know.  Maybe.  But just use the coercions.
// Q_DECLARE_METATYPE_TEMPLATE_1ARG(something)

#endif
