#include "ChatWidgetItem.h"
#include "ContactRegistry.h"
#include "exceptions/InternalErrorException.h"

#include <QDateTime>

ChatWidgetItem::ChatWidgetItem(Contact* contact, ContactIdWithMessageId const& senderAndMessageId, QWidget* parent)
	: QWidget(parent), contact(contact), senderAndMessageId(senderAndMessageId), hasTimeSendSet(false), timeSend(), hasTimeReceivedSet(false), timeReceived(), hasTimeSeenSet(false), timeSeen(), hasTimeAgreeSet(false), timeAgree(), messageState(MessageState::STATE_ENQUEUED), messageAgreeState(MessageAgreeState::STATE_NONE), messageType(MessageType::TYPE_TEXT) {
	// Connect the context menu
	setContextMenuPolicy(Qt::CustomContextMenu);
	if (!connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)))) {
		throw InternalErrorException() << "Could not connect signal customContextMenuRequested in ChatWidgetItem.";
	}
	// 400px minimal width
	this->setMinimumWidth(400);
	this->setMinimumHeight(60);
	this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	mySizeHint = QSize(60, 400);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	sizePolicy.setHeightForWidth(true);
	this->setSizePolicy(sizePolicy);
}

ChatWidgetItem::ChatWidgetItem(Contact* contact, ContactIdWithMessageId const& senderAndMessageId, QDateTime const& timeSend, QDateTime const& timeReceived, QWidget* parent)
	: QWidget(parent), contact(contact), senderAndMessageId(senderAndMessageId), hasTimeSendSet(true), timeSend(timeSend), hasTimeReceivedSet(true), timeReceived(timeReceived), hasTimeSeenSet(false), timeSeen(), hasTimeAgreeSet(false), timeAgree(), messageState(MessageState::STATE_RECEIVED), messageAgreeState(MessageAgreeState::STATE_NONE), messageType(MessageType::TYPE_TEXT) {
	// Connect the context menu
	setContextMenuPolicy(Qt::CustomContextMenu);
	if (!connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)))) {
		throw InternalErrorException() << "Could not connect signal customContextMenuRequested in ChatWidgetItem.";
	}
	// 400px minimal width
	this->setMinimumWidth(400);
	this->setMinimumHeight(60);
	this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	mySizeHint = QSize(60, 400);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	sizePolicy.setHeightForWidth(true);
	this->setSizePolicy(sizePolicy);
}

Contact* ChatWidgetItem::getContact() const {
	return contact;
}

ContactIdWithMessageId const& ChatWidgetItem::getSenderAndMessageId() const {
	return senderAndMessageId;
}

QDateTime const& ChatWidgetItem::getMessageTimeSend() const {
	return timeSend;
}

QDateTime const& ChatWidgetItem::getMessageTimeSeen() const {
	return timeSeen;
}

QDateTime const& ChatWidgetItem::getMessageTimeReceived() const {
	return timeReceived;
}

QDateTime const& ChatWidgetItem::getMessageTimeAgree() const {
	return timeAgree;
}

ChatWidgetItem::MessageState const& ChatWidgetItem::getMessageState() const {
	return messageState;
}

ChatWidgetItem::MessageAgreeState const& ChatWidgetItem::getMessageAgreeState() const {
	return messageAgreeState;
}

ChatWidgetItem::MessageType ChatWidgetItem::getMessageType() const {
	return messageType;
}

void ChatWidgetItem::setMessageState(MessageState const& state, QDateTime const& when) {
	switch (state) {
	case MessageState::STATE_ENQUEUED:
		break;
	case MessageState::STATE_FAILED:
		messageState = state;
		break;
	case MessageState::STATE_SEND:
		if (messageState == MessageState::STATE_ENQUEUED) {
			messageState = state;
		}
		hasTimeSendSet = true;
		timeSend = when;
		break;
	case MessageState::STATE_RECEIVED:
		if (messageState == MessageState::STATE_SEND) {
			messageState = state;
		}
		hasTimeReceivedSet = true;
		timeReceived = when;
		break;
	case MessageState::STATE_READ:
		if (messageState == MessageState::STATE_RECEIVED) {
			messageState = state;
		}
		hasTimeSeenSet = true;
		timeSeen = when;
		break;
	default:
		break;
	}
	onDataUpdated();
}

void ChatWidgetItem::setMessageAgreeState(MessageAgreeState const& state, QDateTime const& when) {
	this->messageAgreeState = state;
	switch (state) {
	case MessageAgreeState::STATE_NONE:
		break;
	case MessageAgreeState::STATE_AGREE:
	case MessageAgreeState::STATE_DISAGREE:
		hasTimeAgreeSet = true;
		timeAgree = when;
		messageAgreeState = state;
	}
	onDataUpdated();
}

QSize ChatWidgetItem::sizeHint() const {
	return mySizeHint;
}

void ChatWidgetItem::setOptimalSizeHint(int width) {
	mySizeHint = QSize(width, heightForWidth(width));
	this->setMinimumHeight(heightForWidth(width));
	this->updateGeometry();
}

QString ChatWidgetItem::getFormattedContactName() const {
	return contact->getContactName();
}

void ChatWidgetItem::onDataUpdated() {
	static const QString templateString(QStringLiteral("From %1, sent on %2. Status: %3"));

	QString text(templateString.arg(getFormattedContactName()).arg(timeSend.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))));
	switch (messageState) {
	case MessageState::STATE_ENQUEUED:
		text = text.arg(QStringLiteral("Sending"));
		break;
	case MessageState::STATE_FAILED:
		text = text.arg(QStringLiteral("FAILED"));
		break;
	case MessageState::STATE_SEND:
		text = text.arg(QStringLiteral("Sent"));
		break;
	case MessageState::STATE_RECEIVED:
		text = text.arg(QStringLiteral("Received"));
		break;
	case MessageState::STATE_READ:
		text = text.arg(QStringLiteral("Read"));
		break;
	default:
		text = text.arg(QStringLiteral("ERROR"));
		break;
	}

	if (messageAgreeState == MessageAgreeState::STATE_AGREE) {
		text = text.append(QStringLiteral(" (Agreed)"));
	} else if (messageAgreeState == MessageAgreeState::STATE_DISAGREE) {
		text = text.append(QStringLiteral(" (Disagreed)"));
	}

	setFromTimeString(text);
}

void ChatWidgetItem::showContextMenu(const QPoint& pos) {
	QPoint globalPos = mapToGlobal(pos);
	QMenu listMessagesContextMenu;

	QAction* actionSend = nullptr;
	QAction* actionReceived = nullptr;
	QAction* actionSeen = nullptr;
	QAction* actionAgree = nullptr;
	QAction* actionCopy = nullptr;

	if (hasTimeSendSet) {
		actionSend = new QAction(QString("Send: %1").arg(timeSend.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))), &listMessagesContextMenu);
	} else {
		actionSend = new QAction(QString("Send: -unknown-"), &listMessagesContextMenu);
	}
	listMessagesContextMenu.addAction(actionSend);
	if (hasTimeReceivedSet) {
		actionReceived = new QAction(QString("Received: %1").arg(timeReceived.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))), &listMessagesContextMenu);
	} else {
		actionReceived = new QAction(QString("Received: -unknown-"), &listMessagesContextMenu);
	}
	listMessagesContextMenu.addAction(actionReceived);
	if (hasTimeSeenSet) {
		actionSeen = new QAction(QString("Seen: %1").arg(timeSeen.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))), &listMessagesContextMenu);
	} else {
		actionSeen = new QAction(QString("Seen: -unknown-"), &listMessagesContextMenu);
	}
	listMessagesContextMenu.addAction(actionSeen);

	if (messageAgreeState == MessageAgreeState::STATE_AGREE) {
		actionAgree = new QAction(QString("Agreed: %1").arg(timeAgree.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))), &listMessagesContextMenu);
	} else if (messageAgreeState == MessageAgreeState::STATE_DISAGREE) {
		actionAgree = new QAction(QString("Disagreed: %1").arg(timeAgree.toString(QStringLiteral("HH:mm:ss, on dd.MM.yyyy"))), &listMessagesContextMenu);
	} else {
		actionAgree = new QAction(QString("Not agreed/disagreed"), &listMessagesContextMenu);
	}
	listMessagesContextMenu.addAction(actionAgree);

	actionCopy = new QAction(QString("Copy to Clipboard"), &listMessagesContextMenu);
	listMessagesContextMenu.addAction(actionCopy);

	appendCustomContextMenuEntries(pos, listMessagesContextMenu);

	QAction* selectedItem = listMessagesContextMenu.exec(globalPos);
	if (selectedItem != nullptr) {
		if (!handleCustomContextMenuEntrySelection(selectedItem)) {
			if (selectedItem == actionSend) {
				//
			} else if (selectedItem == actionSeen) {
				//
			} else if (selectedItem == actionReceived) {
				//
			} else if (selectedItem == actionAgree) {
				//
			} else if (selectedItem == actionCopy) {
				copyToClipboard();
			}
		}
	}
}

void ChatWidgetItem::appendCustomContextMenuEntries(QPoint const& pos, QMenu& menu) {
	// Empty base definition, override in subclass if needed.
}

bool ChatWidgetItem::handleCustomContextMenuEntrySelection(QAction* selectedAction) {
	// Empty base definition, override in subclass if needed.
	return false;
}

void ChatWidgetItem::copyToClipboard() {
	// Empty base definition, override in subclass if needed.
}