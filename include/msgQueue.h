/* msgQueue.h - declaration of VxWorks-like message queue */

/*
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the virtual operating system package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 */

/*
modification history
--------------------
01a,11nov11,sgu  created
*/

/*
DESCRIPTION
This module defines types and prototypes for a message queue. The message queue
manipulation functions in this file are similar with the Wind River VxWorks
kernel message queue interfaces.

If you meet some problem with this module, please feel free to contact
me via e-mail: gushengyuan2002@163.com
*/

#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

/* defines */

/* wait forever for timeout flag */
#define WAIT_FOREVER    -1

/* version string length */
#define VERSION_LEN     8

/* create an inter-thread message queue */
#define msgQCreate(maxMsgs, maxMsgLength, options) \
        msgQCreateEx(maxMsgs, maxMsgLength, options, NULL)

/* typedefs */

typedef unsigned int UINT;
typedef void* MSG_Q_ID;    /* message queue identify */

/* message queue options for task waiting for a message */
enum MSG_Q_OPTION{
    MSG_Q_FIFO     = 0x0000,
    MSG_Q_PRIORITY = 0x0001
};

/* message sending options for sending a message */
enum MSG_Q_PRIORITY{
    MSG_PRI_NORMAL = 0x0000, /* put the message at the end of the queue */
    MSG_PRI_URGENT = 0x0001  /* put the message at the frond of the queue */
};

/* message queue status */
typedef struct tagMSG_Q_STAT {
    char version[VERSION_LEN];  /* library version */
    int maxMsgs;                /* max messages that can be queued */
    UINT maxMsgLength;          /* max bytes in a message */
    int options;                /* message queue options */
    int msgNum;                 /* message number in the queue */
    int sendTimes;              /* number of sent */
    int recvTimes;              /* number of received */
}MSG_Q_STAT;

#ifdef __cplusplus
extern "C"
{
#endif

/* declarations */

/*******************************************************************************
 * msgQCreateEx - create a message queue, queue pended tasks in FIFO order
 *
 * create a message queue, queue pended tasks in FIFO order.
 * <name> message name, if name equals NULL, create an inter-thread message
 * queue, or create an inter-process message queue.
 *
 * RETURNS: MSG_Q_ID when success or NULL otherwise.
 */
MSG_Q_ID msgQCreateEx
    (
    int maxMsgs,     /* max messages that can be queued */
    int maxMsgLength,/* max bytes in a message */
    int options,     /* message queue options, ignored on Windows platform */
    const char *name /* message name */
    );

/*******************************************************************************
 * msgQOpen - open a message queue
 *
 * open a message queue.
 *
 * RETURNS: MSG_Q_ID when success or NULL otherwise.
 */
MSG_Q_ID msgQOpen
    (
    const char * name   /* message name */
    );

/*******************************************************************************
 * msgQDelete - delete a message queue
 *
 * delete a message queue.
 *
 * RETURNS: 0 when success or -1 otherwise.
 */
int msgQDelete
    (
    MSG_Q_ID msgQId /* message queue to delete */
    );

/*******************************************************************************
 * msgQReceive - receive a message from a message queue
 *
 * receive a message from a message queue.
 *
 * RETURNS: 0 when success or -1 otherwise.
 */
int msgQReceive
    (
    MSG_Q_ID msgQId,  /* message queue from which to receive */
    char * buffer,    /* buffer to receive message */
    UINT maxNBytes,   /* length of buffer */
    int timeout       /* ticks to wait */
    );

/*******************************************************************************
 * msgQSend - send a message to a message queue
 *
 * send a message to a message queue.
 *
 * RETURNS: 0 when success or -1 otherwise.
 */
int msgQSend
    (
    MSG_Q_ID msgQId, /* message queue on which to send */
    char * buffer,   /* message to send */
    UINT nBytes,     /* length of message */
    int timeout,     /* ticks to wait */
    int priority     /* MSG_PRI_NORMAL or MSG_PRI_URGENT */
    );

/*******************************************************************************
 * msgQStat - get the status of message queue
 *
 * get the detail status of a message queue.
 *
 * RETURNS: 0 when success or -1 otherwise.
 */
int msgQStat
    (
    MSG_Q_ID msgQId,
    MSG_Q_STAT * msgQStatus
    );

/*******************************************************************************
 * msgQShow - show the status of message queue
 *
 * show the detail status of a message queue.
 *
 * RETURNS: 0 when success or -1 otherwise.
 */
int msgQShow
    (
    MSG_Q_ID msgQId
    );

#ifdef __cplusplus
}
#endif

#endif
