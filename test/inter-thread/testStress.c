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

HANDLE mutex = NULL;

int tc_msgQCreateEx_parameters(void) {
    MSG_Q_ID msgQId = NULL;
    int fails = 0;

    printf("start of test %s.\n", __func__);

    msgQId = msgQCreateEx(0, 100, MSG_Q_FIFO, NULL);
    if (msgQId != NULL) {
        printf("Failed to test maxMsgs with 0.\n");
    	fails++;
    }

    msgQId = msgQCreateEx(-1, 100, MSG_Q_FIFO, NULL);
    if (msgQId != NULL) {
    	printf("Failed to test maxMsgs with -1.\n");
    	fails++;
    }

    msgQId = msgQCreateEx(100, 0, MSG_Q_FIFO, NULL);
    if (msgQId != NULL) {
    	printf("Failed to test maxMsgLength with 0.\n");
    	fails++;
    }

    msgQId = msgQCreateEx(100, -1, MSG_Q_FIFO, NULL);
    if (msgQId != NULL) {
    	printf("Failed to test maxMsgLength with -1.\n");
    	fails++;
    }

    msgQId = msgQCreateEx(100, 100, MSG_Q_FIFO + 100, NULL);
    if (msgQId != NULL) {
    	printf("Failed to test options with invalid value.\n");
    	fails++;
    }

    printf("end of testing %s.\n", __func__);
    return fails;
}

int tc_open_delete_parameters(void) {
    int rc = 0;
    MSG_Q_ID msgQId = NULL;
    char *ptr = NULL;
    int fails = 0;

    printf("start of test %s.\n", __func__);

    msgQId = msgQOpen(NULL);
    if (msgQId != NULL) {
    	printf("Failed to test msgQOpen with invalid value.\n");
    	fails++;
    }

    msgQId = msgQOpen("test_001");
    if (msgQId != NULL) {
    	printf("Failed to test msgQOpen with unknown name.\n");
    	fails++;
    }

    ptr = (char*)malloc(1000);
    if (ptr == NULL) {
    	printf("Failed to allocate memory.\n");
    	fails++;
        goto exit;
    }
    memset(ptr, 0, 1000);

    rc = msgQDelete((MSG_Q_ID)ptr);
    if (rc == 0) {
    	printf("Failed to test msgQDelete with invalid message queue.\n");
    	fails++;
    }

    if (ptr != NULL) {
        free(ptr);
    }

exit:
    printf("end of testing %s.\n", __func__);
    return fails;
}

unsigned int msgQStressSender(void *param) {
    int i = 0;
    int index = 0;
    int priority = 0;
    int rc = 0;
    MSG_Q_STAT stat = {{0},0};
    MSG_Q_ID msgQTest = (MSG_Q_ID)param;

	rc = msgQStat(msgQTest, &stat);
	if(rc != 0) {
        printf("Get the message queue state failed in %s.\n", __func__);
        exit(1);
    }

    while(1) {
        /* protect common data */
        rc = WaitForSingleObject(mutex, INFINITE);
        if(rc != WAIT_OBJECT_0) {
            printf("wait for mutex with errno:%d!\n", (int)GetLastError());
            return -1;
        }

        for (index = 0; index <= stat.maxMsgs; index++) {
            char buf[64] = {0};
            sprintf(buf, "%s-%08d", "ab", i);
            srand((unsigned)time(NULL));
            if(0 == (rand() % 2))
                priority = MSG_PRI_NORMAL;
            else priority = MSG_PRI_URGENT;
            rc = msgQSend(msgQTest, buf, strlen(buf), 2000, priority);
            if(rc != 0) {
                printf("send message failed in %s.\n", __func__);
                break;
            }
            printf("Send %08d time to recever\n", i);
            i++;
            Sleep(5 + i % 2);
        }

        rc = ReleaseMutex(mutex);
        if(rc == 0) {
            printf(" release mutex with errno:%d!\n", (int)GetLastError());
            return -1;
        }
    }
    return 0;
}

unsigned int msgQStressReceiver(void *param) {
    char buf[64] = {0};
    int i = 0;
    int index = 0;
    int rc = 0;
    MSG_Q_STAT stat = {{0},0};
    MSG_Q_ID msgQTest = (MSG_Q_ID)param;

	rc = msgQStat(msgQTest, &stat);
	if(rc != 0) {
        printf("Get the message queue state failed in %s.\n", __func__);
        exit(1);
    }

    Sleep(1000);

    while(1) {
        /* protect common data */
        rc = WaitForSingleObject(mutex, INFINITE);
        if(rc != WAIT_OBJECT_0) {
            printf("wait for mutex with errno:%d!\n", (int)GetLastError());
            return -1;
        }

        for (index = 0; index <= stat.maxMsgs; index++) {
            memset(buf, 0, sizeof(buf));
            rc = msgQReceive(msgQTest, buf, sizeof(buf), 2000);
            if(rc != 0) {
                printf("receive message failed in %s.\n", __func__);
                break;
            }
            printf("Recv %08d time of %s\n", i, buf);
            i++;
        }

        rc = ReleaseMutex(mutex);
        if(rc == 0) {
            printf(" release mutex with errno:%d!\n", (int)GetLastError());
            return -1;
        }

        Sleep(100);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    HANDLE hSender = NULL;
    HANDLE hReceiver = NULL;
    unsigned int tSender = 0;
    unsigned int tReceiver = 0;
    char c = 0;
    MSG_Q_ID msgQTest = NULL;
    int maxMsgs = 8;

    /* Fix the eclipse CDT output issue */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    tc_msgQCreateEx_parameters();

    tc_open_delete_parameters();

    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        printf("create mutex with errno %d!\n", (int)GetLastError());
        return -1;
    }

    msgQTest = msgQCreate(maxMsgs, 100, MSG_Q_FIFO);
    if(msgQTest == NULL) {
        printf("create message queue failed in %s.\n", __func__);
        exit(1);
    }

    hSender = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQStressSender,
    		msgQTest, 0, (DWORD*)&tSender);

    hReceiver = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQStressReceiver,
    		msgQTest, 0, (DWORD*)&tReceiver);

    while((c = getchar()) != 'q') {
        msgQShow(msgQTest);
    }

    CloseHandle(hReceiver);
    CloseHandle(hSender);

    msgQDelete(msgQTest);

    return 0;
}
