/**
 * test.cpp
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the virtual operating system package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Test cases and examples for the message queue module.
 *
 * NOTE: The message queue manipulation functions in this file are similar
 * with the Wind River VxWorks kernel message queue interfaces.
 *
 * If you meet some problem with this module, please feel free to contact
 * me via e-mail: gushengyuan2002@163.com
**/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include "msgQueue.h"

typedef struct tagMSG_Q_TEST {
    MSG_Q_ID msgQId;
    MSG_Q_ID send;
    MSG_Q_ID recv;
    int count;
}MSG_Q_TEST;

typedef struct tagMSG_Q_RESULT {
    int count;
    int time;
}MSG_Q_RESULT;

int tc_create_delete(void) {
    int i = 0;
    int rc = 0;
    MSG_Q_ID * msgQId = NULL;
    int count = 10000;
    int slice = 0;
    int result = 0;

    printf("start of test %s.\n", __func__);

    /* allocate memory for MSG_Q_ID */
    msgQId = (MSG_Q_ID*)malloc(sizeof(MSG_Q_ID) * count);
    memset(msgQId, 0, sizeof(MSG_Q_ID) * count);

    /* test for msgQCreate */
    slice = GetTickCount();
    for(i = 0; i < count; i++) {
        msgQId[i] = msgQCreate(100, 100, MSG_Q_FIFO);
        if(msgQId[i] == NULL) {
            printf("%s: create message queue failed in loop %d.\n", __func__, i);
            result = -1;
            break;
        }
    }
    slice = GetTickCount() - slice;
    printf("finish msgQCreate(...) in %d times with %d ms.\n", i, slice);

    /* test for msgQDelete */
    slice = GetTickCount();
    for(i = 0; i < count; i++) {
        if (msgQId[i] != NULL)
        rc = msgQDelete(msgQId[i]);
        if(rc != 0) {
            printf("%s: delete message queue failed in loop %d.\n", __func__, i);
            result = -1;
            break;
        }
    }
    slice = GetTickCount() - slice;
    printf("finish msgQDelete(...) in %d times with %d ms.\n", i, slice);

    free(msgQId);
    printf("end of testing %s.\n", __func__);

    return result;
}

int tc_create_delete_ex(void) {
    int i = 0;
    int rc = 0;
    MSG_Q_ID * msgQId = NULL;
    int count = 10000;
    int slice = 0;
    int result = 0;
    char buf[64] = {0};

    printf("start of test %s.\n", __func__);

    /* allocate memory for MSG_Q_ID */
    msgQId = (MSG_Q_ID*)malloc(sizeof(MSG_Q_ID) * count);
    memset(msgQId, 0, sizeof(MSG_Q_ID) * count);

    /* test for msgQCreateEx */
    slice = GetTickCount();
    for(i = 0; i < count; i++) {
        sprintf(buf, "test_%d", i);
        msgQId[i] = msgQCreateEx(100, 100, MSG_Q_FIFO, buf);
        if(msgQId[i] == NULL) {
            printf("%s: create message queue failed in loop %d.\n", __func__, i);
            result = -1;
            break;
        }
    }
    slice = GetTickCount() - slice;
    printf("finish msgQCreateEx(...) in %d times with %d ms.\n", i, slice);

    /* test for msgQDelete */
    slice = GetTickCount();
    for(i = 0; i < count; i++) {
        if (msgQId[i] != NULL)
        rc = msgQDelete(msgQId[i]);
        if(rc != 0) {
            printf("%s: delete message queue failed in loop %d.\n", __func__, i);
            result = -1;
            break;
        }
    }
    slice = GetTickCount() - slice;
    printf("finish msgQDelete(...) in %d times with %d ms.\n", i, slice);

    free(msgQId);
    printf("end of testing %s.\n", __func__);

    return result;
}

unsigned int msgQSender(void *param) {
    int i = 0;
    int priority = 0;
    int rc = 0;
    MSG_Q_TEST * msgQTest = (MSG_Q_TEST*)param;
    MSG_Q_ID msgQId = msgQTest->msgQId;
    MSG_Q_ID send = msgQTest->send;
    int count = msgQTest->count;
    int slice = 0;
    MSG_Q_RESULT ret = {0};

    slice = GetTickCount();
    for (i = 0; i < count; i++) {
        char buf[64] = {0};
        sprintf(buf, "%s-%08d", "ab", i);
        srand((unsigned)time(NULL));
        if(0 == (rand() % 2))
            priority = MSG_PRI_NORMAL;
        else priority = MSG_PRI_URGENT;
        rc = msgQSend(msgQId, buf, strlen(buf), WAIT_FOREVER, priority);
        if(rc != 0) {
            printf("send message failed in %s.\n", __func__);
            break;
        }
    }
    slice = GetTickCount() - slice;

    ret.count = i;
    ret.time = slice;
    rc = msgQSend(send, (char*)&ret, sizeof(ret), WAIT_FOREVER, MSG_PRI_NORMAL);
    if(rc != 0) {
        printf("send message failed in %s.\n", __func__);
        return -1;
    }

    return 0;
}

unsigned int msgQReceiver(void *param) {
    char buf[64] = {0};
    int i = 0;
    int rc = 0;
    MSG_Q_TEST * msgQTest = (MSG_Q_TEST*)param;
    MSG_Q_ID msgQId = msgQTest->msgQId;
    MSG_Q_ID recv = msgQTest->recv;
    int count = msgQTest->count;
    int slice = 0;
    MSG_Q_RESULT ret = {0};

    slice = GetTickCount();
    for (i = 0; i < count; i++) {
        memset(buf, 0, sizeof(buf));
        rc = msgQReceive(msgQId, buf, sizeof(buf), WAIT_FOREVER);
        if(rc != 0) {
            printf("receive message failed in %s.\n", __func__);
            break;
        }
    }
    slice = GetTickCount() - slice;

    ret.count = i;
    ret.time = slice;
    rc = msgQSend(recv, (char*)&ret, sizeof(ret), WAIT_FOREVER, MSG_PRI_NORMAL);
    if(rc != 0) {
        printf("send message failed in %s.\n", __func__);
        return -1;
    }

    return 0;
}

int tc_send_recv(int buffer, int tests, int ext) {
    HANDLE hSender = NULL;
    HANDLE hReceiver = NULL;
    unsigned int tSender = 0;
    unsigned int tReceiver = 0;
    MSG_Q_ID msgQId = NULL;
    MSG_Q_ID send = NULL;
    MSG_Q_ID recv = NULL;
    MSG_Q_TEST msgQTest = {0};
    MSG_Q_RESULT ret = {0};
    int rc = 0;

    if (ext) {
        msgQId = msgQCreateEx(buffer, 100, MSG_Q_FIFO, "self");
    }
    else {
        msgQId = msgQCreate(buffer, 100, MSG_Q_FIFO);
    }
    if(msgQId == NULL) {
        printf("create message queue failed in %s.\n", __func__);
        exit(1);
    }

    send = msgQCreate(4, 100, MSG_Q_FIFO);
    if(send == NULL) {
        printf("create message queue failed in %s.\n", __func__);
        exit(1);
    }

    recv = msgQCreate(4, 100, MSG_Q_FIFO);
    if(recv == NULL) {
        printf("create message queue failed in %s.\n", __func__);
        exit(1);
    }

    /* test structure */
    msgQTest.msgQId = msgQId;
    msgQTest.send = send;
    msgQTest.recv = recv;
    msgQTest.count = tests;

    hSender = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQSender,
    		&msgQTest, 0, (DWORD*)&tSender);

    hReceiver = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQReceiver,
    		&msgQTest, 0, (DWORD*)&tReceiver);

    /* wait for result */
    rc = msgQReceive(send, (char*)&ret, sizeof(ret), WAIT_FOREVER);
    if(rc != 0) {
        printf("receive message failed in %s.\n", __func__);
        return -1;
    }
    printf("finish msgQSend(...) in %d times with %d ms.\n", ret.count, ret.time);

    rc = msgQReceive(recv, (char*)&ret, sizeof(ret), WAIT_FOREVER);
    if(rc != 0) {
        printf("receive message failed in %s.\n", __func__);
        return -1;
    }
    printf("finish msgQReceive(...) in %d times with %d ms.\n", ret.count, ret.time);

    CloseHandle(hReceiver);
    CloseHandle(hSender);

    msgQDelete(send);
    msgQDelete(recv);
    msgQDelete(msgQId);

    return 0;
}

int tc_msgQSend_msgQReveive(int maxMsgs, int tests){
    int rc = 0;

    printf("start of test %s with buffer %d.\n", __func__, maxMsgs);
    rc = tc_send_recv(maxMsgs, tests, 0);
    printf("end of testing %s with buffer %d.\n", __func__, maxMsgs);

    return rc;
}

int tc_msgQSend_msgQReveive_Ex(int maxMsgs, int tests){
    int rc = 0;

    printf("start of test %s with buffer %d.\n", __func__, maxMsgs);
    rc = tc_send_recv(maxMsgs, tests, 1);
    printf("end of testing %s with buffer %d.\n", __func__, maxMsgs);

    return rc;
}

int main(int argc, char **argv) {
    int tests = 1000000;
    int buffer[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    unsigned int index = 0;

    /* Fix the eclipse CDT output issue */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    tc_create_delete();
    tc_create_delete_ex();

    for (index = 0; index < sizeof(buffer)/sizeof(int); index++) {
        tc_msgQSend_msgQReveive(buffer[index], tests);
    }

    for (index = 0; index < sizeof(buffer)/sizeof(int); index++) {
        tc_msgQSend_msgQReveive_Ex(buffer[index], tests);
    }

	return 0;
}
