/*
 * wxMessageQueue.h
 *
 *  Created on: 2011-8-28
 *      Author: Nescafe
 */

#ifndef WXMESSAGEQUEUE_H_
#define WXMESSAGEQUEUE_H_

/* include */
#include "msgQueue.h"

class wxMessageQueue
{
public:
	/** constructor */
	wxMessageQueue(
	         int maxMsgs,      /** max messages that can be queued */
	         int maxMsgLength, /** max bytes in a message */
	         int options,      /** message queue options, ignored on Windows platform */
	         const char * pstrName = NULL  /** message name, NULL for inter-thread, or for inter-process */
			);

	wxMessageQueue(
			MSG_Q_ID msgQId    /** message queue id */
			);

	/** destructor */
	~wxMessageQueue();

	/** open a message queue, be sure to delete this object one it's not need */
	static wxMessageQueue * Open(
	         const char * pstrName   /** message name */
	         );

	/** receive a message from a message queue */
	int Receive(
	         char * buffer,    /** buffer to receive message */
	         UINT maxNBytes,   /** length of buffer */
	         int timeout       /** ticks to wait */
	         );

	/** send a message to a message queue */
	int Send(
	         char * buffer,   /** message to send */
	         UINT nBytes,     /** length of message */
	         int timeout,     /** ticks to wait */
	         int priority     /** MSG_PRI_NORMAL or MSG_PRI_URGENT */
	         );

	/** get the status of message queue */
	int Stat(
	         MSG_Q_STAT * msgQStatus
	         );

	/** show the status of message queue */
	int Show();

protected:
	wxMessageQueue();

private:
	MSG_Q_ID m_msgQId;
};

#endif /* WXMESSAGEQUEUE_H_ */
