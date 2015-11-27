/* msgQueue.c - implementation of VxWorks-like message queue */

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
This module implements the functions for a message queue. The message queue
manipulation functions in this file are similar with the Wind River VxWorks
kernel message queue interfaces.

The memory architecture for a message queue:
----------------   -----------------------------------------------------------
| local memory |-->|                     shared memory                       |
----------------   -----------------------------------------------------------
       ^                                     ^
       |                                     |
----------------   -----------------------------------------------------------
|    MSG_Q     |   | MSG_SM | MSG_NODE list |       message queue data       |
----------------   -----------------------------------------------------------
                                    ^                         ^
                                    |                         |
             ---------------------------------------------    |
             | MSG_NODE1 | MSG_NODE2 | ... | MSG_NODE(N) |    |
             ---------------------------------------------    |
                                                              |
                                              ---------------------------------
                                              | data1 | data2 | ... | data(N) |
                                              ---------------------------------

The message queue in memory can be divided into two parts, local memory and
shared memory, but these two parts are not closed by. The local memory can be
accessed in an process, also can be accessed between threads in the process.
The shared memory can be accessed between threads in a process if the message
queue name is NULL; or can be accessed between processes if the message queue
name is not NULL. There is one data structure MSG_Q in local memory; three data
structures -- MSG_SM, MSG_NODE list and message queue data in shared memory.
The structure MSG_Q saves the kernel objects handlers and the shared memory
address; MSG_SM saves the message queue attributes; MSG_NODE list saves all the
nodes for the message queue, and each node saves all attribute of each message;
the message queue data area saves all the data for all the message. All the
structures defined below.

If you meet some problem with this module, please feel free to contact
me via e-mail: gushengyuan2002@163.com
*/

/* includes */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "msgQueue.h"

/* defines */

/* invalid node index for message queue node */
#define MSG_Q_INVALID_NODE -1

/* objects name prefix */

#define _MSG_Q_SEM_P_      "_MSG_Q_SEM_P_" /* prefix for pruducer semaphore */
#define _MSG_Q_SEM_C_      "_MSG_Q_SEM_C_" /* prefix for consumer semaphore */
#define _MSG_Q_MUTEX_      "_MSG_Q_MUTEX_" /* prefix for mutex */
#define _MSG_Q_SHMEM_      "_MSG_Q_SHMEM_" /* prefix for shared memory */
#define MSG_Q_PREFIX_LEN   16              /* max length of object prefix */

/* version string */
#define MSG_Q_VERSION      "0.01"

/* magic string */
#define MSG_Q_MAGIC        "\1\3\5\7\a\7\5\3\1"

/* magic string length */
#define MAGIC_LEN          12

/* debug printable switch */
#if defined(DEBUG) || defined(_DEBUG)
#define PRINTF(fmt, ...) \
        printf("FAIL - %s@%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define PRINTF(fmt, ...)
#endif

/* typedefs */

/* message node structure */
typedef struct tagMSG_NODE {
    unsigned long length;     /* message length */
    int index;                /* node index */
    int free;                 /* next free message index */
    int used;                 /* next used message index */
}MSG_NODE, *P_MSG_NODE;

/* message queue attributes */
typedef struct tagMSG_SM {
    char version[VERSION_LEN];  /* library version */
    char magic[MAGIC_LEN];      /* verify string */
    int maxMsgs;                /* max messages that can be queued */
    UINT maxMsgLength;          /* max bytes in a message */
    int options;                /* message queue options */
    int msgNum;                 /* message number in the queue */
    int sendTimes;              /* number of sent */
    int recvTimes;              /* number of received */
    int head;                   /* head index of the queue */
    int tail;                   /* tail index of the queue */
    int free;                   /* next free index of the queue */
}MSG_SM, *P_MSG_SM;

/* objects handlers for message queue */
typedef struct tagMSG_Q {
    HANDLE semPId;    /* semaphore for producer */
    HANDLE semCId;    /* semaphore for consumer */
    HANDLE mutex;     /* mutex for shared data protecting */
    HANDLE hFile;     /* file handle for the shared memory file mapping */
    MSG_SM * psm;     /* shared memory */
}MSG_Q, *P_MSG_Q;

/* implementations */

/*
 * message queue verification.
 */
static int msgQVerify
    (
    MSG_Q_ID msgQId,
    const char * pSource
    )
{
    P_MSG_Q qid = (P_MSG_Q)msgQId;
    MSG_SM * psm = NULL;

    if (pSource == NULL) {
        pSource = "unknown";
    }

    /* verify the input argument */
    if (qid == NULL){
        PRINTF("%s: message queue id is NULL!\n", pSource);
        return -1;
    }

    /* get and check the shared memory pointer */
    psm = qid->psm;
    if (psm == NULL) {
        PRINTF("%s: share memory is NULL.\n", pSource);
        return -1;
    }

    /* check the version pointer */
    if (psm->version == NULL) {
        PRINTF("%s: version string is NULL.\n", pSource);
        return -1;
    }

    /* check the version string */
    if (strncmp(psm->version, MSG_Q_VERSION, VERSION_LEN) != 0) {
        PRINTF("%s: version string: %s, but expect: %s!\n",
            pSource, psm->version, MSG_Q_VERSION);
        return -1;
    }

    /* check the magic pointer */
    if (psm->magic == NULL) {
        PRINTF("%s: magic string is NULL.\n", pSource);
        return -1;
    }

    /* check the magic string */
    if (strncmp(psm->magic, MSG_Q_MAGIC, MAGIC_LEN) != 0) {
        PRINTF("%s: magic string: %s, but expect: %s!\n",
            pSource, psm->magic, MSG_Q_MAGIC);
        return -1;
    }

    return 0;
}

/*
 * create and initialize a message queue, queue pended tasks in FIFO order
 */
MSG_Q_ID msgQCreateEx
    (
    int maxMsgs,
    int maxMsgLength,
    int options,
    const char * pstrName
    )
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    HANDLE semPId = NULL;   /* semaphore for producer */
    HANDLE semCId = NULL;   /* semaphore for consumer */
    HANDLE mutex = NULL;    /* mutex for protect accessing to shared data */
    HANDLE hFile = NULL;
    char * strName = NULL;
    MSG_SM * psm = NULL;
    MSG_NODE * pNode = NULL;
    int index = 0;
    int memSize = 0;

    /* check the inputed parameters */

    if (maxMsgs <= 0) {
        PRINTF("invalid maxMsgs %d.\n", maxMsgs);
        return NULL;
    }

    if (maxMsgLength <= 0) {
        PRINTF("invalid maxMsgLength %d.\n", maxMsgLength);
        return NULL;
    }

    if (options != MSG_Q_FIFO && options != MSG_Q_PRIORITY) {
        PRINTF("invalid options %d.\n", options);
        return NULL;
    }

    /* allocate the object name memory */
    if (pstrName != NULL) {
        int len = strlen(pstrName) + MSG_Q_PREFIX_LEN + 1;
        strName = (char*)malloc(len);
        if (strName == NULL) {
            PRINTF("allocate memory failed with errno %d!\n", errno);
            return NULL;
        }
        memset(strName, 0, len);
    }

    /* create the producer semaphore */

    if (pstrName != NULL) {
        sprintf(strName, "%s%s", _MSG_Q_SEM_P_, pstrName);
    }

    semPId = CreateSemaphore(NULL, 0, maxMsgs, strName);
    if (semPId == NULL) {
        PRINTF("create semaphore with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* create the consumer semaphore */

    if (pstrName != NULL) {
        sprintf(strName, "%s%s", _MSG_Q_SEM_C_, pstrName);
    }

    semCId = CreateSemaphore(NULL, maxMsgs, maxMsgs, strName);
    if (semCId == NULL) {
        PRINTF("create semaphore with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* create the mutex */

    if (pstrName != NULL) {
        sprintf(strName, "%s%s", _MSG_Q_MUTEX_, pstrName);
    }

    mutex = CreateMutex(NULL, FALSE, strName);
    if (mutex == NULL) {
        PRINTF("create mutex with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* allocate the message queue control block memory */

    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    if (qid == NULL) {
        PRINTF("allocate memory with errno %d!\n", errno);
        goto FailedExit;
    }
    memset(qid, 0, sizeof(MSG_Q));

    /* allocate the message queue memory */

    memSize = sizeof(MSG_SM) + maxMsgs * (sizeof(MSG_NODE) + maxMsgLength);
    if (pstrName == NULL) {
        /* allocate memory for inter-thread message queue */
        psm = (MSG_SM*)malloc(memSize);
    }
    else {
        /* allocate shared memory for inter-process message queue */
        sprintf(strName, "%s%s", _MSG_Q_SHMEM_, pstrName);
        hFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, memSize, strName);
        if (hFile == NULL) {
            PRINTF("CreateFileMapping with errno %d!\n", (int)GetLastError());
            goto FailedExit;
        }

        psm = (MSG_SM*)MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (psm == NULL) {
            PRINTF("MapViewOfFile with errno %d!\n", (int)GetLastError());
            goto FailedExit;
        }
    }

    /*
     * NOTES: check the magic string of the shared memory to see if the message
     * queue was created before. the same object can be create more than one
     * time if the object name is not null -- the object reference is returned
     * when the same object is created. In order to avoid overwrite the shared
     * memory when get the same object reference, must check the magic string
     * of the shared memory.
     */

    if (strcmp(psm->magic, MSG_Q_MAGIC) != 0 || pstrName == NULL) {
        /* clear all the shared memory */
        memset(psm, 0, memSize);

        /* set message queue attributes in shared memory */
        strcpy(psm->version, MSG_Q_VERSION);
        strcpy(psm->magic, MSG_Q_MAGIC);
        psm->maxMsgs = maxMsgs;
        psm->maxMsgLength = maxMsgLength;
        psm->options = options;
        psm->msgNum = 0;
        psm->sendTimes = 0;
        psm->recvTimes = 0;
        psm->head = MSG_Q_INVALID_NODE;
        psm->tail = MSG_Q_INVALID_NODE;
        psm->free = 0;

        /* set the message queue nodes links */
        pNode = (MSG_NODE*)((char*)psm + sizeof(MSG_SM));
        for(index = 0; index < psm->maxMsgs; index++) {
            pNode->length = 0;
            pNode->index = index;
            pNode->free = index + 1;
            pNode->used = MSG_Q_INVALID_NODE;
            pNode++;
        }

        /* set the next free pointer as INVALID for the last Node */
        pNode--;
        pNode->free = MSG_Q_INVALID_NODE;
    }

    /* set the message queue objects handlers */

    qid->semPId = semPId;
    qid->semCId = semCId;
    qid->mutex = mutex;
    qid->hFile = hFile;
    qid->psm = psm;

    if (strName != NULL)
        free(strName);

    return (MSG_Q_ID)qid;

FailedExit:
    if (hFile != NULL && psm != NULL)
        UnmapViewOfFile(psm);
    if (hFile == NULL && psm != NULL)
        free(psm);
    if (hFile != NULL)
        CloseHandle(hFile);
    if (semPId != NULL)
        CloseHandle(semPId);
    if (semCId != NULL)
        CloseHandle(semCId);
    if (mutex != NULL)
        CloseHandle(mutex);
    if (strName != NULL)
        free(strName);
    if (qid != NULL)
        free(qid);

    return NULL;
}

/*
 * open an existed message queue -- inter-process message queue
 */
MSG_Q_ID msgQOpen
    (
    const char * pstrName
    )
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    HANDLE semPId = NULL;   /* semaphore for producer */
    HANDLE semCId = NULL;   /* semaphore for consumer */
    HANDLE mutex = NULL;    /* mutex for protect accessing to shared data */
    HANDLE hFile = NULL;
    char * strName = NULL;
    MSG_SM * psm = NULL;

    if (pstrName == NULL) {
        PRINTF("input NULL parameter.\n");
        return NULL;
    }
    else {
        /* allocate the object name memory */
        int len = strlen(pstrName) + MSG_Q_PREFIX_LEN + 1;
        strName = (char*)malloc(len);
        if (strName == NULL) {
            PRINTF("allocate memory failed with errno %d!\n", errno);
            return NULL;
        }
        memset(strName, 0, len);
    }

    /* allocate the message queue control block memory */

    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    if (qid == NULL) {
        PRINTF("allocate memory failed with errno %d!\n", errno);
        goto FailedExit;
    }
    memset(qid, 0, sizeof(MSG_Q));

    /* open the message queue share data */

    sprintf(strName, "%s%s", _MSG_Q_SHMEM_, pstrName);
    hFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, strName);
    if (hFile == NULL) {
        PRINTF("OpenFileMapping with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    psm = (MSG_SM*)MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (psm == NULL) {
        PRINTF("MapViewOfFile with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    qid->psm = psm;

    /* valid verification */

    if (msgQVerify(qid, __func__) == -1) {
        goto FailedExit;
    }

    /* open the producer semaphore */

    sprintf(strName, "%s%s", _MSG_Q_SEM_P_, pstrName);
    semPId = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, strName);
    if (semPId == NULL) {
        PRINTF("open semaphore with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* open the consumer semaphore */

    sprintf(strName, "%s%s", _MSG_Q_SEM_C_, pstrName);
    semCId = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, strName);
    if (semCId == NULL) {
        PRINTF("open semaphore with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* open the protecting mutex */

    sprintf(strName, "%s%s", _MSG_Q_MUTEX_, pstrName);
    mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, strName);
    if (mutex == NULL) {
        PRINTF("open mutex with errno %d!\n", (int)GetLastError());
        goto FailedExit;
    }

    /* set the message queue attributes */

    qid->semPId = semPId;
    qid->semCId = semCId;
    qid->mutex = mutex;
    qid->hFile = hFile;
    qid->psm = psm;

    if (strName != NULL)
        free(strName);

    return qid;

FailedExit:
    if (psm != NULL)
        UnmapViewOfFile(psm);
    if (hFile != NULL)
        CloseHandle(hFile);
    if (semPId != NULL)
        CloseHandle(semPId);
    if (semCId != NULL)
        CloseHandle(semCId);
    if (mutex != NULL)
        CloseHandle(mutex);
    if (strName != NULL)
        free(strName);
    if (qid != NULL)
        free(qid);

    return NULL;
}

/*
 * delete a message queue
 */
int msgQDelete
    (
    MSG_Q_ID msgQId
    )
{
    int status = 0;
    int failed = 0;
    P_MSG_Q qid = (P_MSG_Q)msgQId;

    /* verify if the message queue is valid */
    if (msgQVerify(qid, __func__) == -1) {
        return -1;
    }

    status = CloseHandle(qid->semPId);
    if(status == 0) {
        PRINTF("close semaphore with errno %d!\n", (int)GetLastError());
        failed++;
    }

    status = CloseHandle(qid->semCId);
    if(status == 0) {
        PRINTF("close semaphore with errno %d!\n", (int)GetLastError());
        failed++;
    }

    status = CloseHandle(qid->mutex);
    if(status == 0) {
        PRINTF("close mutex with errno %d!\n", (int)GetLastError());
        failed++;
    }

    if (qid->hFile != NULL) {

        /*
         * NOTES: the shared memory must not be cleared when delete this message
         * queue. the close operation in this routine only reduce the reference
         * count for the objects, and the objects will be destroyed when the
         * objects reference count equal to zero. the objects can be reused if
         * the objects reference count is not zero. In order to protect the
         * shared memory, avoid to call memset(qid->psm, 0x0, sizeof(MSG_SM));
         * or any other clear operation.
         */

        status = UnmapViewOfFile(qid->psm);
        if(status == 0) {
            PRINTF("UnmapViewOfFile with errno %d!\n", (int)GetLastError());
            failed++;
        }

        status = CloseHandle(qid->hFile);
        if(status == 0) {
            PRINTF("close mapped file with errno %d!\n", (int)GetLastError());
            failed++;
        }
    }
    else {
        free((void*)qid->psm);
    }
    free((void*)qid);

    return failed == 0 ? 0 : -1;
}

/*
 * receive a message from a message queue
 */
int msgQReceive
    (
    MSG_Q_ID msgQId,
    char * buffer,
    UINT maxNBytes,
    int timeout
    )
{
    unsigned long status = 0;
    unsigned long timeLimit = 0;
    MSG_NODE * pNode = NULL;
    P_MSG_Q qid = (P_MSG_Q)msgQId;
    MSG_SM * psm = NULL;

    if(buffer == NULL) {
        PRINTF("input buffer equals NULL.\n");
        return -1;
    }

    /* verify if the message queue is valid */
    if (msgQVerify(qid, __func__) == -1) {
        return -1;
    }

    /* get the shared memory pointer */
    psm = qid->psm;

    /* timeout conversion */
    if(timeout <= -1) timeLimit = INFINITE; /* wait forever */
    else timeLimit = (unsigned long)timeout;

    /* message is available if the producer semaphore can be taken */
    status = WaitForSingleObject(qid->semPId, timeLimit);
    if(status != WAIT_OBJECT_0) {
        if(status == WAIT_FAILED) {
            PRINTF("wait for semaphore with errno:%d!\n", (int)GetLastError());
        }
        /* WAIT_TIMEOUT */
        return -1;
    }

    /* take the mutex for shared memory protecting */
    status = WaitForSingleObject(qid->mutex, INFINITE);
    if(status != WAIT_OBJECT_0) {
        /* release the producer semaphore if failed to take the mutex */        
        if(0 == ReleaseSemaphore(qid->semPId, 1, NULL)) {
            PRINTF("release semaphore with errno:%d!\n", (int)GetLastError());
            return -1;
        }

        if(status == WAIT_ABANDONED || status == WAIT_FAILED) {
            PRINTF("wait for mutex with errno:%d!\n", (int)GetLastError());
        }
        /* WAIT_TIMEOUT */
        return -1;
    }

    /* get the message node we want to process */
    pNode = (MSG_NODE*)((char*)psm + sizeof(MSG_SM) + \
        psm->tail * sizeof(MSG_NODE));

    /* calculate the message length */
    maxNBytes = (maxNBytes > pNode->length) ? pNode->length : maxNBytes;

    /* copy the message to buffer */
    memcpy(buffer, (char*)psm + sizeof(MSG_SM) + \
        psm->maxMsgs * sizeof(MSG_NODE) + \
        psm->maxMsgLength * pNode->index, maxNBytes);

    /* update the tail of the used message link */
    psm->tail = pNode->used;

    /* free and append the message node to the free message link */
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->free = psm->free;
    psm->free = pNode->index;

    /* there is no message if the tail equals to MSG_Q_INVALID_NODE */
    if (psm->tail == MSG_Q_INVALID_NODE)
        psm->head = MSG_Q_INVALID_NODE;

    /* update the message counting attributes */
    psm->msgNum--;
    psm->recvTimes++;

    /* release mutex */
    status = ReleaseMutex(qid->mutex);
    if(status == 0) {
        PRINTF(" release mutex with errno:%d!\n", (int)GetLastError());
        return -1;
    }

    /* release the consumer semaphore */
    status = ReleaseSemaphore(qid->semCId, 1, NULL);
    if(status == 0) {
        PRINTF("release semaphore with errno:%d!\n", (int)GetLastError());
        return -1;
    }

    return 0;
}

/*
 * send a message to a message queue
 */
int msgQSend
    (
    MSG_Q_ID msgQId,
    char * buffer,
    UINT nBytes,
    int timeout,
    int priority
    )
{
    int status = 0;
    unsigned long timeLimit = 0;
    MSG_NODE * pNode = NULL;
    P_MSG_Q qid = (P_MSG_Q)msgQId;
    MSG_SM * psm = NULL;

    if(buffer == NULL) {
        PRINTF("input buffer equals NULL.\n");
        return -1;
    }

    if(priority != MSG_PRI_NORMAL && priority != MSG_PRI_URGENT) {
        PRINTF("invalid priority %d.\n", priority);
        return -1;
    }

    /* verify if the message queue is valid */
    if (msgQVerify(qid, __func__) == -1) {
        return -1;
    }

    /* get the shared memory pointer */
    psm = qid->psm;

    /* check the message length */
    if(nBytes > psm->maxMsgLength) {
        PRINTF("nBytes %d exceed the maxMsgLength %d.\n",
            nBytes, psm->maxMsgLength);
        return -1;
    }

    /* timeout conversion */
    if(timeout <= -1) timeLimit = INFINITE; /* wait forever */
    else timeLimit = (unsigned long)timeout;

    /* there is free slot in queue if the consumer semaphore can be taken */
    status = WaitForSingleObject(qid->semCId, timeLimit);
    if(status != WAIT_OBJECT_0) { /* failed */
        if(status == WAIT_FAILED) {
            PRINTF("wait for semaphore with errno:%d!\n", (int)GetLastError());
        }
        /* WAIT_TIMEOUT */
        return -1;
    }

    /* take the mutex for shared memory protecting */
    status = WaitForSingleObject(qid->mutex, INFINITE);
    if(status != WAIT_OBJECT_0) {
        /* release the consumer semaphore if failed to take the mutex */
        if(0 == ReleaseSemaphore(qid->semCId, 1, NULL)) {
            PRINTF("release semaphore with errno:%d!\n", (int)GetLastError());
            return -1;
        }

        if(status == WAIT_ABANDONED || status == WAIT_FAILED) {
            PRINTF("wait for mutex with errno:%d!\n", (int)GetLastError());
        }
        /* WAIT_TIMEOUT */
        return -1;
    }

    /* get a free message node we want to use */
    pNode = (MSG_NODE*)((char*)psm + sizeof(MSG_SM) + \
        psm->free * sizeof(MSG_NODE));

    /* update the next free message node number */
    psm->free = pNode->free;

    /* set the node attributes */
    pNode->free = MSG_Q_INVALID_NODE;
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->length = nBytes;

    /* copy the buffer to the message node */
    memcpy((char*)psm + sizeof(MSG_SM) + \
        psm->maxMsgs * sizeof(MSG_NODE) + \
        psm->maxMsgLength * pNode->index, buffer, nBytes);

    /* both the head and tail pointer to this node if it's the first message */
    if (psm->head == MSG_Q_INVALID_NODE) {
        psm->head = pNode->index;
        psm->tail = pNode->index;
    }
    else {
        /* or send a normal message or urgent message */
        if (priority == MSG_PRI_NORMAL) {
            /* get the head message node */
            MSG_NODE * temp = (MSG_NODE*)((char*)psm + sizeof(MSG_SM) + \
                psm->head * sizeof(MSG_NODE));

            /* append the new message node to the head message node */
            temp->used = pNode->index;
            psm->head = pNode->index;
        }
        else {
            /* append the new message node to the tail message node */
            pNode->used = psm->tail;
            psm->tail = pNode->index;
        }
    }

    /* update the message counting attributes */
    psm->msgNum++;
    psm->sendTimes++;

    /* release the mutex */
    status = ReleaseMutex(qid->mutex);
    if(status == 0) {
        PRINTF("release mutex with errno:%d!\n", (int)GetLastError());
        return -1;
    }

    /* release the producer semaphore */
    status = ReleaseSemaphore(qid->semPId, 1, NULL);
    if(status == 0) {
        PRINTF("release semaphore with errno:%d!\n", (int)GetLastError());
        return -1;
    }

    return 0;
}

/*
 * get the status of message queue
 */
int msgQStat
    (
    MSG_Q_ID msgQId,
    MSG_Q_STAT * msgQStatus
    )
{
    P_MSG_Q qid = (P_MSG_Q)msgQId;
    MSG_SM * psm = NULL;

    /* verify if the message queue is valid */
    if (msgQVerify(qid, __func__) == -1) {
        return -1;
    }

    /* save all the message queue attributes */

    psm = qid->psm;
    msgQStatus->maxMsgs = psm->maxMsgs;
    msgQStatus->maxMsgLength = psm->maxMsgLength;
    msgQStatus->msgNum = psm->msgNum;
    msgQStatus->options = psm->options;
    msgQStatus->recvTimes = psm->recvTimes;
    msgQStatus->sendTimes = psm->sendTimes;
    strncpy(msgQStatus->version, psm->version, VERSION_LEN);

    return 0;
}

/*
 * show the status of message queue
 */
int msgQShow
    (
    MSG_Q_ID msgQId
    )
{
    P_MSG_Q qid = (P_MSG_Q)msgQId;
    MSG_SM * psm = NULL;

    /* verify if the message queue is valid */
    if (msgQVerify(qid, __func__) == -1) {
        return -1;
    }

    /*
     * show all the message queue attributes,
     * the private attributes like magic, free, head and tail won't be show.
     */

    psm = qid->psm;
    printf("msgQueue.version      = %s\n", psm->version);
    printf("msgQueue.maxMsg       = %d\n", psm->maxMsgs);
    printf("msgQueue.maxMsgLength = %d\n", psm->maxMsgLength);
    printf("msgQueue.msgNum       = %d\n", psm->msgNum);
    printf("msgQueue.options      = %d\n", psm->options);
    printf("msgQueue.recvTimes    = %d\n", psm->recvTimes);
    printf("msgQueue.sendTimes    = %d\n", psm->sendTimes);

    return 0;
}
