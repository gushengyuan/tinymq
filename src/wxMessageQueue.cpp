/*
 * wxMessageQueue.cpp
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the virtual operating system package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Implements of functions for message queue.
 *
 * NOTE: The message queue manipulation functions in this file are similar
 * with the Wind River VxWorks kernel message queue interfaces.
 *
 * If you meet some problem with this module, please feel free to contact
 * me via e-mail: gushengyuan2002@163.com
 */

#include <stdio.h>
#include <stdlib.h>
#include "wxMessageQueue.h"

wxMessageQueue::wxMessageQueue(int maxMsgs, int maxMsgLength, int options, const char * pstrName)
{
	m_msgQId = msgQCreateEx(maxMsgs, maxMsgLength, options, pstrName);
}

wxMessageQueue::wxMessageQueue(MSG_Q_ID msgQId)
{
	m_msgQId = msgQId;
}

wxMessageQueue::wxMessageQueue()
{
	m_msgQId = NULL;
}

wxMessageQueue::~wxMessageQueue()
{
	msgQDelete(m_msgQId);
}

wxMessageQueue * wxMessageQueue::Open(
         const char * pstrName   /* message name */
         )
{
	MSG_Q_ID msgQId = msgQOpen(pstrName);
	if (msgQId == NULL)
		return NULL;

	return new wxMessageQueue(msgQId);
}

int wxMessageQueue::Receive(
         char * buffer,    /* buffer to receive message */
         UINT maxNBytes,   /* length of buffer */
         int timeout       /* ticks to wait */
         )
{
	return msgQReceive(m_msgQId, buffer, maxNBytes, timeout);
}

int wxMessageQueue::Send(
         char * buffer,   /* message to send */
         UINT nBytes,     /* length of message */
         int timeout,     /* ticks to wait */
         int priority     /* MSG_PRI_NORMAL or MSG_PRI_URGENT */
         )
{
	return msgQSend(m_msgQId, buffer, nBytes, timeout, priority);
}

int wxMessageQueue::Stat(MSG_Q_STAT * msgQStatus)
{
	return msgQStat(m_msgQId, msgQStatus);
}

int wxMessageQueue::Show()
{
	return msgQShow(m_msgQId);
}

