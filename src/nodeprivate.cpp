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
    return globalEngine->_mapIdToNode.value(id.toUuid(), nullptr);
}

NodePrivate & NodePrivate::NodeFromDomElement(const QDomElement& element) {
    QString const idString (element.attribute("id"));
    auto result = NodePrivate::maybeGetFromId(
        UuidFromBase64String(idString.toLatin1())
    );
    hopefully(result, HERE);

    // we are the implementation, we can do this...
    return const_cast<NodePrivate &>(*result); 
}

Label NodePrivate::LabelFromDomElement(const QDomElement& element) {
    return UuidFromBase64String(element.tagName().toLatin1());
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
        return NodePrivate::createText(original.getText());

    auto clone = NodePrivate::create(original.getTag());
    if (not original.hasAnyLabels())
        return clone;

    Label label = original.getFirstLabel();
    bool moreLabels = true;
    unsigned int labelCount = 0;
    while (moreLabels) {
        const NodePrivate * childOfOriginal =
            &original.getFirstChildInLabel(label);

        labelCount++;
        bool moreChildren = true;
        unsigned int childCount = 0;
        while (moreChildren) {
            childCount++;
            NodePrivate const * previousChildInLabel;
            clone->insertChildAsLastInLabel(
                childOfOriginal->makeCloneOfSubtree(),
                label,
                &previousChildInLabel);
            moreChildren = childOfOriginal->hasNextSiblingInLabel();
            if (moreChildren)
                childOfOriginal = &childOfOriginal->getNextSiblingInLabel();
        }
        moreLabels = original.hasNextLabel(label);
        if (moreLabels)
            label = original.getNextLabel(label);
    }
    hopefully(clone->isSubtreeCongruentTo(original), HERE);
    return clone;
}

NodePrivate::~NodePrivate ()
{
    if (hasAnyLabels()) {
        Label label = getFirstLabel();
        bool moreLabels = true;
        while (moreLabels) {
            Label lastLabel = label;
            moreLabels = hasNextLabel(label);
            if (moreLabels)
                label = getNextLabel(label);
            while (hasLabel(lastLabel)) {
                Label dummyLabel;
                NodePrivate const * nodeParent;
                NodePrivate const * previousChild;
                NodePrivate const * nextChild;
                getFirstChildInLabel(lastLabel).detach(
                    &dummyLabel,
                    &nodeParent,
                    &previousChild,
                    &nextChild
                );
            }
        }
    }

    methyl::Identity const id = getId();
    // do not call any node member routines after this point...
    hopefully(globalEngine->_mapIdToNode.remove(id.toUuid()) == 1, HERE);
}

NodePrivate::NodePrivate (methyl::Identity const & id, QString const & str)
{
    _element = globalEngine->_document.createElement("text");

    QString idBase64 = Base64StringFromUuid(id.toUuid());
    _element.setAttribute("id", idBase64);

    // way to atomic insert and check for non-prior existence?
    hopefully(not globalEngine->_mapIdToNode.contains(id.toUuid()), HERE);
    globalEngine->_mapIdToNode.insert(id.toUuid(), this); // track it

    _element.setAttribute("text", str);

    chronicle(globalDebugNodeCreate, 
        "NodePrivate::Node() with methyl::Identity " + idBase64
        + " and data = " + str,
        HERE
    );
}

NodePrivate::NodePrivate (methyl::Identity const & id, Tag const & tag)
{
    QString tagBase64 = Base64StringFromUuid(tag.toUuid());
    _element = globalEngine->_document.createElement(tagBase64);

    QString idBase64 = Base64StringFromUuid(id.toUuid());
    _element.setAttribute("id", idBase64);

    hopefully(
        globalEngine->_mapIdToNode.find(id.toUuid())
        == globalEngine->_mapIdToNode.end(), HERE
    );
    globalEngine->_mapIdToNode.insert(id.toUuid(), this); // track it

    chronicle(globalDebugNodeCreate, 
        "NodePrivate::Node() with methyl::Identity " + idBase64
        + " and tag = " + tagBase64,
        HERE
    );
}


// read-only accessors

NodePrivate const & NodePrivate::getDoc() const {
    return NodeFromDomElement(globalEngine->_document.documentElement());
}

NodePrivate & NodePrivate::getDoc() {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getDoc());
}


methyl::Identity NodePrivate::getId() const {
    return UuidFromBase64String(_element.attribute("id").toLatin1());
}


// Parent specification

bool NodePrivate::hasParent() const {
    return not _element.parentNode().isNull();
}

NodePrivate const & NodePrivate::getParent() const
{
    hopefully(hasParent(), HERE);
    const QDomElement parentLabelElement = _element.parentNode().toElement();
    const QDomElement parentNodeElement =
        parentLabelElement.parentNode().toElement();

    hopefully(not parentNodeElement.isNull(), HERE);
    return NodeFromDomElement(parentNodeElement);
}

NodePrivate & NodePrivate::getParent()
{
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getParent());
}

Label NodePrivate::getLabelInParent() const
{
    hopefully(hasParent(), HERE);
    const QDomElement immediateParent =  _element.parentNode().toElement();
    return LabelFromDomElement(immediateParent);
}

bool NodePrivate::hasTag() const
{
    return _element.tagName() != "text";
}

Tag NodePrivate::getTag() const
{
    Tag result = UuidFromBase64String(_element.tagName().toLatin1());
    return result;

}
QString NodePrivate::getText() const {
    hopefully(_element.tagName() == "text", HERE);
    return _element.attribute("text");
}

// label enumeration; no user-controllable ordering
// invariant order from methyl::Identity

tuple<bool, optional<QDomElement>> NodePrivate::maybeGetLabelElementCore(
    Label const & label,
    bool createIfNecessary
) const {
    QString labelBase64 = Base64StringFromUuid(label);
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

bool NodePrivate::hasAnyLabels() const
{
    return _element.hasChildNodes();
}

bool NodePrivate::hasLabel(Label const & label) const
{
    return static_cast<bool>(maybeGetLabelElement(label));
}

Label NodePrivate::getFirstLabel() const
{
    hopefully(hasAnyLabels(), HERE);
    return UuidFromBase64String(
        _element.firstChildElement().tagName().toLatin1()
    );
}

Label NodePrivate::getLastLabel() const
{
    hopefully(hasAnyLabels(), HERE);
    return UuidFromBase64String(
        _element.lastChildElement().tagName().toLatin1()
    );
}

bool NodePrivate::hasNextLabel(Label const & label) const
{
    return not getLabelElement(label).nextSiblingElement().isNull();
}

Label NodePrivate::getNextLabel(Label const & label) const
{
    QDomElement labelElement = getLabelElement(label);
    hopefully(not labelElement.nextSiblingElement().isNull(), HERE);
    return UuidFromBase64String(
        labelElement.nextSiblingElement().tagName().toLatin1()
    );
}

bool NodePrivate::hasPreviousLabel(Label const & label) const
{
    return not getLabelElement(label).previousSiblingElement().isNull();
}

Label NodePrivate::getPreviousLabel(Label const & label) const
{
    QDomElement labelElement = getLabelElement(label);
    hopefully(not labelElement.previousSiblingElement().isNull(), HERE);
    return UuidFromBase64String(
        labelElement.previousSiblingElement().tagName().toLatin1()
    );
}

// node in label enumeration
NodePrivate const & NodePrivate::getFirstChildInLabel(Label const & label) const
{
    QDomElement labelElement = getLabelElement(label);
    QDomElement childElement = labelElement.firstChildElement();
    hopefully(not childElement.isNull(), HERE);
    hopefully(childElement.parentNode().parentNode() == _element, HERE);
    return NodeFromDomElement(childElement);
}

NodePrivate & NodePrivate::getFirstChildInLabel(Label const & label) {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getFirstChildInLabel(label));
}

NodePrivate const & NodePrivate::getLastChildInLabel(Label const & label) const
{
    QDomElement labelElement = getLabelElement(label);
    QDomElement childElement = labelElement.lastChildElement();
    hopefully(not childElement.isNull(), HERE);
    return NodeFromDomElement(childElement);
}

NodePrivate & NodePrivate::getLastChildInLabel(Label const & label)
{
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getLastChildInLabel(label));
}

bool NodePrivate::hasNextSiblingInLabel() const
{
    return not _element.nextSiblingElement().isNull();
}

NodePrivate const & NodePrivate::getNextSiblingInLabel() const
{
    return NodeFromDomElement(_element.nextSiblingElement());
}

NodePrivate & NodePrivate::getNextSiblingInLabel() {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getNextSiblingInLabel());
}

bool NodePrivate::hasPreviousSiblingInLabel() const {
    return not _element.previousSiblingElement().isNull();
}

NodePrivate const & NodePrivate::getPreviousSiblingInLabel() const {
    return NodeFromDomElement(_element.previousSiblingElement());
}

NodePrivate & NodePrivate::getPreviousSiblingInLabel() {
    NodePrivate const & constRef = *this;
    return const_cast<NodePrivate &>(constRef.getPreviousSiblingInLabel());
}

// modifications

void NodePrivate::setTag(Tag const & tag) {
    hopefully(hasTag(), HERE);
    _element.setTagName(Base64StringFromUuid(tag.toUuid()));
}

// set this node as the first node in the given label
NodePrivate & NodePrivate::insertChildAsFirstInLabel(
    unique_ptr<NodePrivate> newChild,
    const Label & label,
    NodePrivate const * * nextChildInLabelOut
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    bool wasLabelElementCreated;
    QDomElement labelElement;
    tie(wasLabelElementCreated, labelElement) =
        getLabelElementCreateIfNecessary(label);

    *nextChildInLabelOut = [&] () -> NodePrivate const * {
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
    return *newChild.release();
}

// set this node as the first node in the given label
NodePrivate & NodePrivate::insertChildAsLastInLabel(
    unique_ptr<NodePrivate> newChild,
    Label const & label,
    NodePrivate const * * previousChildInLabelOut
) {
    hopefully(not newChild->hasParent(), HERE);
    hopefully(hasTag(), HERE);

    bool wasLabelElementCreated;
    QDomElement labelElement;
    tie(wasLabelElementCreated, labelElement) =
        getLabelElementCreateIfNecessary(label);

    *previousChildInLabelOut = [&] () -> NodePrivate const * {
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
    return *newChild.release();
}

NodePrivate & NodePrivate::insertSiblingAfter(
    unique_ptr<NodePrivate> newChild,
    NodePrivate const * * parentOut,
    Label * labelInParentOut,
    NodePrivate const * * nextChildInLabelOut
) {
    hopefully(not newChild->hasParent(), HERE);
    *parentOut = &getParent();

    QDomElement labelElement = getLabelElement(getLabelInParent());
    hopefully(newChild->_element == labelElement.insertAfter(
        newChild->_element, _element),
        HERE
    );
    *labelInParentOut = LabelFromDomElement(labelElement);

    QDomElement nextChildElement = newChild->_element.nextSibling().toElement();
    if (nextChildElement.isNull()) {
        *nextChildInLabelOut = nullptr;
    } else {
        *nextChildInLabelOut = &NodeFromDomElement(nextChildElement);
    }
    *labelInParentOut = LabelFromDomElement(labelElement);
    return *newChild.release();
}

NodePrivate & NodePrivate::insertSiblingBefore(
    unique_ptr<NodePrivate> newChild,
    NodePrivate const * * parentOut,
    Label* labelInParentOut,
    NodePrivate const * * previousChildInLabelOut
) {
    hopefully(not newChild->hasParent(), HERE);
    *parentOut = &getParent();

    QDomElement labelElement = getLabelElement(getLabelInParent());
    hopefully(newChild->_element == labelElement.insertBefore(
        newChild->_element, _element
    ), HERE);

    *labelInParentOut = LabelFromDomElement(labelElement);

    QDomElement previousChildElement =
        newChild->_element.previousSibling().toElement();

    if (previousChildElement.isNull()) {
        *previousChildInLabelOut = nullptr;
    } else {
        *previousChildInLabelOut = &NodeFromDomElement(previousChildElement);
    }
    return *newChild.release();
}

// detach from parent
unique_ptr<NodePrivate> NodePrivate::detach(
    Label * labelInParentOut,
    NodePrivate const * * nodeParentOut,
    NodePrivate const * * previousChildOut,
    NodePrivate const * * nextChildOut
) {
    *previousChildOut = [&] () -> NodePrivate const * {
        auto previousChildElement = _element.previousSibling().toElement();
        if (previousChildElement.isNull())
            return nullptr;
        return &NodeFromDomElement(previousChildElement);
    } ();

    *nextChildOut = [&] () -> NodePrivate const * {
        auto nextChildElement = _element.previousSibling().toElement();
        if (nextChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(nextChildElement);
    } ();

    Label labelBack = getLabelInParent();
    NodePrivate & nodeParent = getParent();
    bool removeLabel = false;
    if (
        (&nodeParent.getFirstChildInLabel(labelBack) == this)
        and (&nodeParent.getLastChildInLabel(labelBack) == this)
    ) {
        removeLabel = true;
    }

    QDomElement labelElement = nodeParent.getLabelElement(labelBack);
    labelElement.removeChild(_element);
    if (removeLabel) {
        nodeParent._element.removeChild(labelElement);
    }

    *labelInParentOut = labelBack;
    *nodeParentOut = &nodeParent;
    return unique_ptr<NodePrivate>(this);
}

unique_ptr<NodePrivate> NodePrivate::replaceWith(
    unique_ptr<NodePrivate> nodeReplacement,
    Label * labelInParentOut,
    NodePrivate const * * nodeParentOut,
    NodePrivate const * * previousChildOut,
    NodePrivate const * * nextChildOut
) {
    *previousChildOut = [&] () -> NodePrivate const * {
        auto previousChildElement = _element.previousSibling().toElement();
        if (previousChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(previousChildElement);
    } ();

    *nextChildOut = [&] () -> NodePrivate const * {
        auto nextChildElement = _element.previousSibling().toElement();
        if (nextChildElement.isNull())
            return nullptr;

        return &NodeFromDomElement(nextChildElement);
    } ();

    hopefully(not nodeReplacement->_element.isNull(), HERE);
    hopefully(nodeReplacement->_element.parentNode().isNull(), HERE);
    const Label labelInParent = getLabelInParent();
    NodePrivate const & nodeParent (getParent());
    QDomElement labelElement = nodeParent.getLabelElement(labelInParent);
    QDomNode oldChild = labelElement.replaceChild(
        nodeReplacement->_element, _element
    );
    hopefully(not oldChild.isNull(), HERE);
    hopefully(oldChild == _element, HERE);

    *labelInParentOut = labelInParent;
    *nodeParentOut = &nodeParent;
    nodeReplacement.release();
    return unique_ptr<NodePrivate> (this);
}


/// simple constant accessors

void NodePrivate::setText(
    QString const & data
) {
    hopefully(_element.tagName() == "text", HERE);
    _element.setAttribute("text", data);
}

} // namespace methyl


