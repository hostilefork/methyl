//
// nodeprivate.cpp
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

#include "methyl/nodeprivate.h"
#include "methyl/engine.h"

namespace methyl {

tracked<bool> globalDebugAccessor(false, HERE);
tracked<bool> globalDebugNodeCreate (false, HERE);
tracked<bool> globalDebugNodeLabeling (false, HERE);

//
// STATIC METHODS
//

NodePrivate const * NodePrivate::maybeGetFromId (methyl::Identity const & id) {
    QReadLocker lock (&globalEngine->_mapLock);

    auto iter = globalEngine->_mapIdToNode.find(id);
    if (iter == end(globalEngine->_mapIdToNode))
        return nullptr;
    return iter->second;
}


unique_ptr<NodePrivate> NodePrivate::createWithTag (Tag const & tag) {
    return unique_ptr<NodePrivate> (
        new NodePrivate(Identity (QUuid::createUuid()), tag)
    );
}


unique_ptr<NodePrivate> NodePrivate::createAsText (QString const & data) {
    return unique_ptr<NodePrivate> (
        new NodePrivate(Identity (QUuid::createUuid()), data)
    );
}


unique_ptr<NodePrivate> NodePrivate::makeCloneOfSubtree () const {
    NodePrivate const & original = *this;

    if (not original.hasTag())
        return NodePrivate::createAsText(original.text(HERE));

    auto clone = NodePrivate::createWithTag(original.tag(HERE));
    if (not original.hasAnyLabels())
        return clone;

    Label label = original.firstLabel(HERE);
    bool moreLabels = true;
    unsigned int labelCount = 0;
    while (moreLabels) {
        const NodePrivate * childOfOriginal =
            &original.firstChildInLabel(label, HERE);

        labelCount++;
        bool moreChildren = true;
        unsigned int childCount = 0;
        while (moreChildren) {
            childCount++;
            NodePrivate const * previousChildInLabel;
            static_cast<void>(
                clone->insertChildAsLastInLabel(
                    childOfOriginal->makeCloneOfSubtree(), label
                )
            );
            moreChildren = childOfOriginal->hasNextSiblingInLabel();
            if (moreChildren)
                childOfOriginal = &childOfOriginal->nextSiblingInLabel(HERE);
        }
        moreLabels = original.hasLabelAfter(label, HERE);
        if (moreLabels)
            label = original.labelAfter(label, HERE);
    }
    hopefully(clone->isSubtreeCongruentTo(original), HERE);
    return clone;
}



//
// Constructor and Destructor
//
// NodePrivate represents something that would ideally be some kind of
// pointer or handle into a database or memory-mapped file.  The rest of
// Methyl is defined in terms of expecting the basic services of the
// NodePrivate API to be available.  The simplest implementation on hand
// to use in a Qt project was the Qt DOM.
//

NodePrivate::NodePrivate (methyl::Identity const & id, QString const & text) :
    _parent (nullptr),
    _id (id),
    _tag (),
    _labelToChildren (),
    _text (text)
{

    {
        QWriteLocker lock (&globalEngine->_mapLock);

        bool wasInserted;
        std::tie(std::ignore, wasInserted)
            = globalEngine->_mapIdToNode.insert(std::make_pair(id, this));

        hopefully(wasInserted, HERE);
    }

    chronicle(globalDebugNodeCreate, [&](QDebug o) {
        o << "NodePrivate::Node() with methyl::Identity "
            << id.toUuid().toString()
            << " and text = "
            << text
            << "\n";
    }, HERE);
}


NodePrivate::NodePrivate (methyl::Identity const & id, Tag const & tag) :
    _parent (nullptr),
    _id (id),
    _tag (tag),
    _labelToChildren (),
    _text ()

{
    {
        QWriteLocker lock (&globalEngine->_mapLock);

        bool wasInserted;
        std::tie(std::ignore, wasInserted)
            = globalEngine->_mapIdToNode.insert(std::make_pair(id, this));

        hopefully(wasInserted, HERE);
    }

    chronicle(globalDebugNodeCreate, [&](QDebug o) {
        o << "NodePrivate::Node() with methyl::Identity"
            << id.toUuid().toString()
            << "and tag ="
            << tag.toUrl().toString()
            << "\n";
    }, HERE);
}


NodePrivate::~NodePrivate ()
{
    // null out members to help prevent accesses from children during
    // the destruction process.
    _tag = nullopt;
    _text = nullopt;
    auto labelToChildren = std::move(_labelToChildren);

    for (auto & labelChildren : labelToChildren) {
        for (auto & child : labelChildren.second) {
            child->_parent = nullptr;
            delete child;
        }
    }

    // Currently the internal code for translating between Qt DOM elements
    // and nodes needs to have the entry in _mapIdToAccessorto work; so the
    // above tree traversal would not work if we removed the ID from the
    // table first.
    {
        QWriteLocker lock (&globalEngine->_mapLock);
        hopefully(globalEngine->_mapIdToNode.erase(identity()) == 1, HERE);
    }
}


methyl::Identity NodePrivate::identity() const {
    return _id;
}



//
// Parent specification
//

bool NodePrivate::hasParent() const {
    return _parent != nullptr;
}


NodePrivate const & NodePrivate::parent (codeplace const & cp) const {
    hopefully(hasParent(), cp);
    return *_parent;
}


NodePrivate & NodePrivate::parent (codeplace const & cp) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.parent(cp));
}


NodePrivate::relationship_info NodePrivate::relationshipToParent (
    codeplace const & cp
) const
{
    hopefully(hasParent(), cp);
    for (auto & labelChildren : _parent->_labelToChildren) {
        auto iter = std::find(
            begin(labelChildren.second), end(labelChildren.second), this
        );
        if (iter != end(labelChildren.second))
            return relationship_info (
                labelChildren.first, labelChildren.second, iter
            );
    }
    throw hopefullyNotReached(HERE);
}


NodePrivate::relationship_info NodePrivate::relationshipToParent (
    codeplace const & cp
) {
    hopefully(hasParent(), cp);
    for (auto & labelChildren : _parent->_labelToChildren) {
        auto iter = std::find(
            begin(labelChildren.second), end(labelChildren.second), this
        );
        if (iter != end(labelChildren.second))
            return relationship_info (
                labelChildren.first, labelChildren.second, iter
            );
    }
    throw hopefullyNotReached(HERE);
}




Label NodePrivate::labelInParent (codeplace const & cp) const {
    return relationshipToParent(cp)._labelInParent;
}


NodePrivate const & NodePrivate::root() const {
    NodePrivate const * current = this;
    while (current->_parent) {
        current = _parent->_parent;
    }
    return *current;
}


NodePrivate & NodePrivate::root() {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.root());
}



//
// Tag and Text Examination
//

bool NodePrivate::hasTag() const {
    return _tag != nullopt;
}


Tag NodePrivate::tag (codeplace const & cp) const {
    hopefully(hasTag(), cp);
    return *_tag;
}


QString NodePrivate::text (codeplace const & cp) const {
    hopefully(hasText(), cp);
    return *_text;
}



//
// Label-in-AccessorEnumeration
//
// Order is not user-controllable.  It is invariant from the ordering
// specified by methyl::Label::compare()
//

bool NodePrivate::hasAnyLabels () const {
    return !_labelToChildren.empty();
}


bool NodePrivate::hasLabel (Label const & label) const {
    return _labelToChildren.find(label) != end(_labelToChildren);
}


Label NodePrivate::firstLabel (codeplace const & cp) const {
    hopefully(hasAnyLabels(), cp);
    return (*_labelToChildren.begin()).first;
}


Label NodePrivate::lastLabel (codeplace const & cp) const {
    hopefully(hasAnyLabels(), cp);
    return (*--_labelToChildren.end()).first;
}


bool NodePrivate::hasLabelAfter (
    Label const & label,
    codeplace const & cp
) const
{
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    return ++iter != end(_labelToChildren);
}


Label NodePrivate::labelAfter (
    Label const & label,
    codeplace const & cp
) const
{
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    hopefully(++iter != end(_labelToChildren), cp);
    return (*iter).first;
}


bool NodePrivate::hasLabelBefore (
    Label const & label,
    codeplace const & cp
) const {
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    return iter != begin(_labelToChildren);
}


Label NodePrivate::labelBefore (
    Label const & label,
    codeplace const & cp
) const
{
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    hopefully(iter != begin(_labelToChildren), cp);
    return (*--iter).first;
}



//
// AccessorIn Label Enumeration
//

NodePrivate const & NodePrivate::firstChildInLabel (
    Label const & label,
    codeplace const & cp
) const
{
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    return *(*iter).second.front();
}


NodePrivate & NodePrivate::firstChildInLabel (
    Label const & label,
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.firstChildInLabel(label, cp));
}


NodePrivate const & NodePrivate::lastChildInLabel (
    Label const & label,
    codeplace const & cp
) const
{
    auto iter = _labelToChildren.find(label);
    hopefully(iter != end(_labelToChildren), cp);
    return *(*iter).second.back();
}


NodePrivate & NodePrivate::lastChildInLabel (
    Label const & label,
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.lastChildInLabel(label, cp));
}


bool NodePrivate::hasNextSiblingInLabel () const {
    relationship_info info = relationshipToParent(HERE);

    return (info._iter + 1) != end(info._siblings.get());
}


NodePrivate const & NodePrivate::nextSiblingInLabel (
    codeplace const & cp
) const
{
    relationship_info info = relationshipToParent(cp);

    return *(*++info._iter);
}


NodePrivate & NodePrivate::nextSiblingInLabel (
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.nextSiblingInLabel(cp));
}


bool NodePrivate::hasPreviousSiblingInLabel () const {
    relationship_info info = relationshipToParent(HERE);

    return info._iter != begin(info._siblings.get());
}


NodePrivate const & NodePrivate::previousSiblingInLabel (
    codeplace const & cp
) const {
    relationship_info info = relationshipToParent(cp);

    return *(*--info._iter);
}


NodePrivate & NodePrivate::previousSiblingInLabel (
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.previousSiblingInLabel(cp));
}



//
// Modifications
//

void NodePrivate::setTag(Tag const & tag) {
    hopefully(hasTag(), HERE);
    _tag = tag;
}


NodePrivate::insert_result NodePrivate::insertChildAsFirstInLabel (
    unique_ptr<NodePrivate> newChild,
    Label const & label
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    NodePrivate * newChildPtr = newChild.release();
    newChildPtr->_parent = this;

    auto iter = _labelToChildren.find(label);

    if (iter == end(_labelToChildren)) {
        _labelToChildren.insert(
            std::make_pair(label, std::vector<NodePrivate *>{newChildPtr})
        );
        return insert_result (
            *newChildPtr,
            insert_info (nullptr, label, nullptr, nullptr)
        );
    }

    NodePrivate * nextChild = (*iter).second.front();
    (*iter).second.insert(begin((*iter).second), newChildPtr);

    return insert_result (
        *newChildPtr,
        insert_info (nullptr, label, nullptr, nextChild)
    );
}


NodePrivate::insert_result NodePrivate::insertChildAsLastInLabel (
    unique_ptr<NodePrivate> newChild,
    Label const & label
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    NodePrivate * newChildPtr = newChild.release();
    newChildPtr->_parent = this;

    auto iter = _labelToChildren.find(label);

    if (iter == end(_labelToChildren)) {
        _labelToChildren.insert(
            std::make_pair(label, std::vector<NodePrivate *>{newChildPtr})
        );
        return insert_result (
            *newChildPtr,
            insert_info (nullptr, label, nullptr, nullptr)
        );
    }

    NodePrivate * previousChild = (*iter).second.back();
    (*iter).second.push_back(newChildPtr);

    return insert_result (
        *newChildPtr,
        insert_info (nullptr, label, previousChild, nullptr)
    );
}


NodePrivate::insert_result NodePrivate::insertSiblingAfter (
    unique_ptr<NodePrivate> newSibling
) {
    hopefully(not newSibling->hasParent(), HERE);

    NodePrivate * newSiblingPtr = newSibling.release();
    newSiblingPtr->_parent = this->_parent;

    relationship_info info = relationshipToParent(HERE);

    auto nextChild = [&]() -> NodePrivate const * {
        if (++info._iter == end(info._siblings.get())) {
            return nullptr;
        } else {
            return *info._iter;
        }
    }();

    // we've bumped the iterator... so inserting *after* initial iter
    info._siblings.get().insert(info._iter, newSiblingPtr);

    return insert_result (
        *newSiblingPtr,
        insert_info (_parent, info._labelInParent, nullptr, nextChild)
    );
}


NodePrivate::insert_result NodePrivate::insertSiblingBefore (
    unique_ptr<NodePrivate> newSibling
) {
    NodePrivate * newSiblingPtr = newSibling.release();
    newSiblingPtr->_parent = this->_parent;

    relationship_info info = relationshipToParent(HERE);

    auto previousChild = [&]() -> NodePrivate const * {
        if (info._iter == begin(info._siblings.get())) {
            return nullptr;
        } else {
            return *(--info._iter);
        }
    }();

    // we've bumped the iterator... so inserting *before* initial iter
    info._siblings.get().insert(info._iter, newSiblingPtr);

    return insert_result (
        *newSiblingPtr,
        insert_info (_parent, info._labelInParent, previousChild, nullptr)
    );
}


auto NodePrivate::detach ()
    -> tuple<unique_ptr<NodePrivate>, NodePrivate::detach_info>
{
    hopefully(hasParent(), HERE);

    relationship_info info = relationshipToParent(HERE);

    auto previousChild = [&]() -> NodePrivate const * {
        if (info._iter == begin(info._siblings.get())) {
            return nullptr;
        } else {
            return *(info._iter - 1);
        }
    }();

    auto nextChild = [&]() -> NodePrivate const * {
        if (info._iter == end(info._siblings.get())) {
            return nullptr;
        } else {
            return *(info._iter + 1);
        }
    }();

    info._siblings.get().erase(info._iter);
    if (info._siblings.get().empty()) {
        _parent->_labelToChildren.erase(info._labelInParent);
    }

    this->_parent = nullptr;

    return make_tuple(
        unique_ptr<NodePrivate> (this),
        detach_info (*_parent, info._labelInParent, previousChild, nextChild)
    );
}


auto NodePrivate::replaceWith (
    unique_ptr<NodePrivate> replacement
)
    -> tuple<unique_ptr<NodePrivate>, NodePrivate::detach_info>
{
    hopefully(hasParent(), HERE);

    NodePrivate * replacementPtr = replacement.release();
    replacementPtr->_parent = this->_parent;

    relationship_info info = relationshipToParent(HERE);

    auto previousChild = [&]() -> NodePrivate const * {
        if (info._iter == begin(info._siblings.get())) {
            return nullptr;
        } else {
            return *(info._iter - 1);
        }
    }();

    auto nextChild = [&]() -> NodePrivate const * {
        if (info._iter == end(info._siblings.get())) {
            return nullptr;
        } else {
            return *(info._iter + 1);
        }
    }();

    *info._iter = replacementPtr;
    this->_parent = nullptr;

    return make_tuple(
        unique_ptr<NodePrivate> (this),
        detach_info (*_parent, info._labelInParent, previousChild, nextChild)
    );
}


void NodePrivate::setText (
    QString const & text
) {
    hopefully(hasText(), HERE);
    _text = text;
}


//
// Tree Walking and comparison
//

int NodePrivate::compare (NodePrivate const & other) const {
    NodePrivate const * thisCur = this;
    NodePrivate const * otherCur = &other;

    do {
        if (thisCur->hasText() and not otherCur->hasText())
            return -1;
        if (not thisCur->hasText() and otherCur->hasText())
            return 1;
        if (thisCur->hasText() and otherCur->hasText()) {
            int cmp = thisCur->text(HERE).compare(otherCur->text(HERE));
            if (cmp != 0)
                return cmp;
        } else {
            int cmp = thisCur->tag(HERE).compare(otherCur->tag(HERE));
            if (cmp != 0)
                return cmp;
            if (thisCur->tag(HERE) != otherCur->tag(HERE))
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

} // namespace methyl
