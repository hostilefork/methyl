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

tracked<bool> globalDebugNode (false, HERE);
tracked<bool> globalDebugNodeCreate (false, HERE);
tracked<bool> globalDebugNodeLabeling (false, HERE);

//
// STATIC METHODS
//

NodePrivate const * NodePrivate::maybeGetFromId(methyl::Identity const & id) {
    QReadLocker lock (&globalEngine->_mapLock);

    return globalEngine->_mapIdBase64ToNode.value(id.toBase64(), nullptr);
}


NodePrivate & NodePrivate::NodeFromDomElement(const QDomElement & element) {
    QString idBase64 = element.attribute("id");
    auto result = NodePrivate::maybeGetFromId(Identity (idBase64));
    hopefully(result, HERE);

    // we are the implementation, we can do this...
    return const_cast<NodePrivate &>(*result); 
}


Label NodePrivate::LabelFromDomElement(const QDomElement& element) {
    return Label (element.tagName());
}


unique_ptr<NodePrivate> NodePrivate::create(Tag const & tag) {
    QUuid uuid = QUuid::createUuid();
    return unique_ptr<NodePrivate> (new NodePrivate(uuid, tag));
}


unique_ptr<NodePrivate> NodePrivate::createText(QString const & data) {
    QUuid uuid = QUuid::createUuid();
    return unique_ptr<NodePrivate> (new NodePrivate(uuid, data));
}


unique_ptr<NodePrivate> NodePrivate::makeCloneOfSubtree() const {
    NodePrivate const & original = *this;

    if (not original.hasTag())
        return NodePrivate::createText(original.getText(HERE));

    auto clone = NodePrivate::create(original.getTag(HERE));
    if (not original.hasAnyLabels())
        return clone;

    Label label = original.getFirstLabel(HERE);
    bool moreLabels = true;
    unsigned int labelCount = 0;
    while (moreLabels) {
        const NodePrivate * childOfOriginal =
            &original.getFirstChildInLabel(label, HERE);

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
                childOfOriginal = &childOfOriginal->getNextSiblingInLabel(HERE);
        }
        moreLabels = original.hasLabelAfter(label);
        if (moreLabels)
            label = original.getLabelAfter(label, HERE);
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

NodePrivate::NodePrivate (methyl::Identity const & id, QString const & data)
{
    _element = globalEngine->_document.createElement("text");

    QString idBase64 = Base64StringFromUuid(id.toUuid());
    _element.setAttribute("id", idBase64);

    {
        QWriteLocker lock (&globalEngine->_mapLock);

        // way to atomic insert and check for non-prior existence?
        hopefully(not globalEngine->_mapIdBase64ToNode.contains(idBase64), HERE);
        globalEngine->_mapIdBase64ToNode.insert(idBase64, this);
    }

    _element.setAttribute("text", data);

    chronicle(globalDebugNodeCreate, 
        "NodePrivate::Node() with methyl::Identity " + idBase64
        + " and data = " + data,
        HERE
    );
}


NodePrivate::NodePrivate (methyl::Identity const & id, Tag const & tag)
{
    QString tagBase64 = tag.toBase64();
    _element = globalEngine->_document.createElement(tagBase64);

    QString idBase64 = id.toBase64();
    _element.setAttribute("id", idBase64);

    {
        QWriteLocker lock (&globalEngine->_mapLock);

        // way to atomic insert and check for non-prior existence?
        hopefully(not globalEngine->_mapIdBase64ToNode.contains(idBase64), HERE);
        globalEngine->_mapIdBase64ToNode.insert(idBase64, this);
    }

    chronicle(globalDebugNodeCreate, 
        "NodePrivate::Node() with methyl::Identity " + idBase64
        + " and tag = " + tagBase64,
        HERE
    );
}


NodePrivate::~NodePrivate ()
{
    QString idBase64 = getId().toBase64();

    if (hasAnyLabels()) {
        Label label = getFirstLabel(HERE);
        bool moreLabels = true;
        while (moreLabels) {
            Label lastLabel = label;
            moreLabels = hasLabelAfter(label);
            if (moreLabels)
                label = getLabelAfter(label, HERE);
            while (hasLabel(lastLabel)) {
                auto result = getFirstChildInLabel(lastLabel, HERE).detach();
                unique_ptr<NodePrivate> & ownedNode = std::get<0>(result);
                ownedNode.release();
            }
        }
    }

    QWriteLocker lock (&globalEngine->_mapLock);
    hopefully(globalEngine->_mapIdBase64ToNode.remove(idBase64) == 1, HERE);
}



//
// Document and Identity
//

NodePrivate const & NodePrivate::getDoc() const {
    return NodeFromDomElement(globalEngine->_document.documentElement());
}


NodePrivate & NodePrivate::getDoc() {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getDoc());
}


methyl::Identity NodePrivate::getId() const {
    return Identity (_element.attribute("id"));
}



//
// Parent specification
//

bool NodePrivate::hasParent() const {
    return not _element.parentNode().isNull();
}


NodePrivate const & NodePrivate::getParent (codeplace const & cp) const {
    hopefully(hasParent(), cp);
    const QDomElement parentLabelElement = _element.parentNode().toElement();
    const QDomElement parentNodeElement =
        parentLabelElement.parentNode().toElement();

    hopefully(not parentNodeElement.isNull(), HERE);
    return NodeFromDomElement(parentNodeElement);
}


NodePrivate & NodePrivate::getParent (codeplace const & cp) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getParent(cp));
}


Label NodePrivate::getLabelInParent (codeplace const & cp) const {
    hopefully(hasParent(), cp);
    const QDomElement immediateParent =  _element.parentNode().toElement();
    return LabelFromDomElement(immediateParent);
}


bool NodePrivate::hasTag() const {
    return _element.tagName() != "text";
}


Tag NodePrivate::getTag (codeplace const & cp) const {
    hopefully(_element.tagName() != "text", cp);
    return Tag (_element.tagName());
}


QString NodePrivate::getText (codeplace const & cp) const {
    hopefully(_element.tagName() == "text", cp);
    return _element.attribute("text");
}



//
// Label-in-Node Enumeration
//
// Order is not user-controllable.  It is invariant from the ordering
// specified by methyl::Label::compare()
//

tuple<bool, optional<QDomElement>> NodePrivate::maybeGetLabelElementCore (
    Label const & label,
    bool createIfNecessary
) const {
    QString labelBase64 = label.toBase64();
    auto & mutableElement = *const_cast<QDomElement *>(&_element);

    QDomNodeList children = _element.childNodes();
    for (int childIndex = 0; childIndex < children.length(); childIndex++) {

        QDomNode node = children.item(childIndex);
        QString nodeTypeString = node.nodeName();

        QDomElement labelElement = children.item(childIndex).toElement();
        hopefully(not labelElement.isNull(), HERE);
        int cmp = labelElement.tagName().compare(labelBase64);
        if (cmp == 0) {
            return make_tuple(false, optional<QDomElement> (labelElement));
        } else if (cmp < 0) {
            // tagName less than labelBase64, we need to keep looking
        } else {
            // tagName greater than labelBase64, we passed it...
            if (createIfNecessary) {
                chronicle(globalDebugNodeLabeling,
                    "On " + _element.attribute("id") + ", creating label "
                    + labelBase64 + " BEFORE " + labelElement.tagName(),
                    HERE);

                QDomElement newLabelElement =
                    globalEngine->_document.createElement(labelBase64);

                hopefully(newLabelElement == mutableElement.insertBefore(
                    newLabelElement, labelElement
                ), HERE);
                return make_tuple(
                    true, optional<QDomElement> (newLabelElement)
                );
            }
            return make_tuple(false, nullopt); // null
        }
    }
    if (createIfNecessary) {
        chronicle(globalDebugNodeLabeling,
            "On " + _element.attribute("id") + "," +
            " creating label " + labelBase64 + " AT END ",
            HERE
        );

        // we reached the end of the tags and we did not see one that
        // compared greater than label
        QDomElement newLabelElement =
            globalEngine->_document.createElement(labelBase64);

        hopefully(newLabelElement == mutableElement.appendChild(
                newLabelElement
            ),
            HERE
        );
        return make_tuple(true, optional<QDomElement> (newLabelElement));
    }
    return make_tuple(false, nullopt);
}


bool NodePrivate::hasAnyLabels () const {
    return _element.hasChildNodes();
}


bool NodePrivate::hasLabel (Label const & label) const {
    return static_cast<bool>(maybeGetLabelElement(label));
}


Label NodePrivate::getFirstLabel (codeplace const & cp) const {
    hopefully(hasAnyLabels(), cp);
    return Label (_element.firstChildElement().tagName());
}


Label NodePrivate::getLastLabel (codeplace const & cp) const {
    hopefully(hasAnyLabels(), cp);
    return Label (_element.lastChildElement().tagName());
}


bool NodePrivate::hasLabelAfter (Label const & label) const {
    return not getLabelElement(label).nextSiblingElement().isNull();
}


Label NodePrivate::getLabelAfter (
    Label const & label,
    codeplace const & cp
) const
{
    QDomElement labelElement = getLabelElement(label);
    hopefully(not labelElement.nextSiblingElement().isNull(), cp);
    return Label (labelElement.nextSiblingElement().tagName());
}


bool NodePrivate::hasLabelBefore (Label const & label) const {
    return not getLabelElement(label).previousSiblingElement().isNull();
}


Label NodePrivate::getLabelBefore (
    Label const & label,
    codeplace const & cp
) const
{
    QDomElement labelElement = getLabelElement(label);
    hopefully(not labelElement.previousSiblingElement().isNull(), cp);
    return Label (labelElement.previousSiblingElement().tagName());
}



//
// Node In Label Enumeration
//

NodePrivate const & NodePrivate::getFirstChildInLabel (
    Label const & label,
    codeplace const & cp
) const
{
    QDomElement labelElement = getLabelElement(label);
    QDomElement childElement = labelElement.firstChildElement();
    hopefully(not childElement.isNull(), cp);
    hopefully(childElement.parentNode().parentNode() == _element, HERE);
    return NodeFromDomElement(childElement);
}


NodePrivate & NodePrivate::getFirstChildInLabel (
    Label const & label,
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getFirstChildInLabel(label, cp));
}


NodePrivate const & NodePrivate::getLastChildInLabel (
    Label const & label,
    codeplace const & cp
) const
{
    QDomElement labelElement = getLabelElement(label);
    QDomElement childElement = labelElement.lastChildElement();
    hopefully(not childElement.isNull(), cp);
    return NodeFromDomElement(childElement);
}


NodePrivate & NodePrivate::getLastChildInLabel (
    Label const & label,
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getLastChildInLabel(label, cp));
}


bool NodePrivate::hasNextSiblingInLabel () const {
    return not _element.nextSiblingElement().isNull();
}


NodePrivate const & NodePrivate::getNextSiblingInLabel (
    codeplace const & cp
) const
{
    QDomElement nextElement = _element.nextSiblingElement();
    hopefully(not nextElement.isNull(), cp);
    return NodeFromDomElement(nextElement);
}


NodePrivate & NodePrivate::getNextSiblingInLabel (
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getNextSiblingInLabel(cp));
}


bool NodePrivate::hasPreviousSiblingInLabel () const {
    return not _element.previousSiblingElement().isNull();
}


NodePrivate const & NodePrivate::getPreviousSiblingInLabel (
    codeplace const & cp
) const {
    QDomElement previousElement = _element.previousSiblingElement();
    hopefully(not previousElement.isNull(), cp);
    return NodeFromDomElement(_element.previousSiblingElement());
}


NodePrivate & NodePrivate::getPreviousSiblingInLabel (
    codeplace const & cp
) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getPreviousSiblingInLabel(cp));
}



//
// Modifications
//

void NodePrivate::setTag(Tag const & tag) {
    hopefully(hasTag(), HERE);
    _element.setTagName(tag.toBase64());
}


NodePrivate::insert_result NodePrivate::insertChildAsFirstInLabel (
    unique_ptr<NodePrivate> newChild,
    Label const & label
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    bool wasLabelElementCreated;
    QDomElement labelElement;
    tie(wasLabelElementCreated, labelElement) =
        getLabelElementCreateIfNecessary(label);

    auto nextChild = [&] () -> NodePrivate const * {
        if (wasLabelElementCreated) {
            hopefully(
                labelElement.appendChild(newChild->_element)
                == newChild->_element,
                HERE
            );
            return nullptr;
        } else {
            QDomElement nextChildElement = labelElement.firstChildElement();
            hopefully(newChild->_element == labelElement.insertBefore(
                newChild->_element, nextChildElement
                ),
                HERE
            );
            return &NodeFromDomElement(nextChildElement);
        }
    } ();

    hopefully(labelElement.firstChildElement() == newChild->_element, HERE);

    return insert_result (
        *newChild.release(),
        insert_info (nullptr, label, nullptr, nextChild)
    );
}


NodePrivate::insert_result NodePrivate::insertChildAsLastInLabel (
    unique_ptr<NodePrivate> newChild,
    Label const & label
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    bool wasLabelElementCreated;
    QDomElement labelElement;
    tie(wasLabelElementCreated, labelElement) =
        getLabelElementCreateIfNecessary(label);

    auto previousChild = [&] () -> NodePrivate const * {
        if (wasLabelElementCreated) {
            hopefully(
                labelElement.appendChild(newChild->_element) 
                == newChild->_element,
                HERE
            );
            return nullptr;
        } else {
            QDomElement previousChildElement = labelElement.lastChildElement();
            hopefully(newChild->_element == labelElement.insertAfter(
                newChild->_element, previousChildElement
            ), HERE);
            return &NodeFromDomElement(previousChildElement);
        }
    } ();

    hopefully(labelElement.lastChildElement() == newChild->_element, HERE);

    return insert_result (
        *newChild.release(),
        insert_info (nullptr, label, previousChild, nullptr)
    );
}


NodePrivate::insert_result NodePrivate::insertSiblingAfter (
    unique_ptr<NodePrivate> newSibling
) {
    hopefully(not newSibling->hasParent(), HERE);
    NodePrivate & nodeParent = getParent(HERE);

    QDomElement labelElement = nodeParent.getLabelElement(getLabelInParent(HERE));
    hopefully(newSibling->_element == labelElement.insertAfter(
        newSibling->_element, _element
    ), HERE);

    Label labelInParent = LabelFromDomElement(labelElement);

    QDomElement nextChildElement
        = newSibling->_element.nextSibling().toElement();

    auto nextChild = [&]() -> NodePrivate const * {
        if (nextChildElement.isNull()) {
            return nullptr;
        } else {
            return &NodeFromDomElement(nextChildElement);
        }
    }();

    return insert_result (
        *newSibling.release(),
        insert_info (&nodeParent, labelInParent, nullptr, nextChild)
    );
}


NodePrivate::insert_result NodePrivate::insertSiblingBefore (
    unique_ptr<NodePrivate> newSibling
) {
    hopefully(not newSibling->hasParent(), HERE);
    NodePrivate & nodeParent = getParent(HERE);

    QDomElement labelElement = nodeParent.getLabelElement(getLabelInParent(HERE));
    hopefully(newSibling->_element == labelElement.insertBefore(
        newSibling->_element, _element
    ), HERE);

    Label labelInParent = LabelFromDomElement(labelElement);

    QDomElement previousChildElement =
        newSibling->_element.previousSibling().toElement();

    auto previousChild = [&]() -> NodePrivate const * {
        if (previousChildElement.isNull()) {
            return nullptr;
        } else {
            return &NodeFromDomElement(previousChildElement);
        }
    }();

    return insert_result (
        *newSibling.release(),
        insert_info (&nodeParent, labelInParent, previousChild, nullptr)
    );
}


auto NodePrivate::detach ()
    -> tuple<unique_ptr<NodePrivate>, NodePrivate::detach_info>
{
    auto previousChild = [&] () -> NodePrivate const * {
        auto previousChildElement = _element.previousSibling().toElement();
        if (previousChildElement.isNull())
            return nullptr;
        return &NodeFromDomElement(previousChildElement);
    } ();

    auto nextChild = [&] () -> NodePrivate const * {
        auto nextChildElement = _element.previousSibling().toElement();
        if (nextChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(nextChildElement);
    } ();

    Label labelInParent = getLabelInParent(HERE);
    NodePrivate & nodeParent = getParent(HERE);
    bool removeLabel = false;
    if (
        (&nodeParent.getFirstChildInLabel(labelInParent, HERE) == this)
        and (&nodeParent.getLastChildInLabel(labelInParent, HERE) == this)
    ) {
        removeLabel = true;
    }

    QDomElement labelElement
        = nodeParent.getLabelElement(labelInParent);
    labelElement.removeChild(_element);
    if (removeLabel) {
        nodeParent._element.removeChild(labelElement);
    }

    return make_tuple(
        unique_ptr<NodePrivate> (this),
        detach_info (nodeParent, labelInParent, previousChild, nextChild)
    );
}


auto NodePrivate::replaceWith (
    unique_ptr<NodePrivate> nodeReplacement
)
    -> tuple<unique_ptr<NodePrivate>, NodePrivate::detach_info>
{
    auto previousChild = [&] () -> NodePrivate const * {
        auto previousChildElement = _element.previousSibling().toElement();
        if (previousChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(previousChildElement);
    } ();

    auto nextChild = [&] () -> NodePrivate const * {
        auto nextChildElement = _element.previousSibling().toElement();
        if (nextChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(nextChildElement);
    } ();

    hopefully(not nodeReplacement->_element.isNull(), HERE);
    hopefully(nodeReplacement->_element.parentNode().isNull(), HERE);
    Label labelInParent = getLabelInParent(HERE);
    NodePrivate const & nodeParent (getParent(HERE));
    QDomElement labelElement = nodeParent.getLabelElement(labelInParent);
    QDomNode oldChild = labelElement.replaceChild(
        nodeReplacement->_element, _element
    );
    hopefully(not oldChild.isNull(), HERE);
    hopefully(oldChild == _element, HERE);

    nodeReplacement.release();

    return make_tuple(
        unique_ptr<NodePrivate> (this),
        detach_info (nodeParent, labelInParent, previousChild, nextChild)
    );
}


void NodePrivate::setText (
    QString const & data
) {
    hopefully(_element.tagName() == "text", HERE);
    _element.setAttribute("text", data);
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

} // namespace methyl
