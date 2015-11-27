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

unsigned int msgQSender(void *param) {
    int i = 0;
    int priority = 0;
    int rc = 0;
    MSG_Q_STAT stat;
    MSG_Q_ID msgQTest = (MSG_Q_ID)param;

	rc = msgQStat(msgQTest, &stat);
	if(rc != 0)	{
        printf("Get the message queue state failed in %s.\n", __func__);
        exit(1);
    }

    Sleep(1000);
    while(1) {
        do {
            char buf[64] = {0};
            sprintf(buf, "%s-%08d", "ab", i);
            srand((unsigned)time(NULL));
            if(0 == (rand() % 2))
                priority = MSG_PRI_NORMAL;
            else priority = MSG_PRI_URGENT;
            rc = msgQSend(msgQTest, buf, strlen(buf), WAIT_FOREVER, priority);
            if(rc != 0) {
                printf("send message failed in %s.\n", __func__);
                exit(1);
            }
            printf("Send %08d time to recever\n", i);
            i++;
        }
        while((i % (stat.maxMsgs * 3)) == 0);
//		Sleep(1000);
    }
    return 0;
}

unsigned int msgQReceiver(void *param) {
    char buf[64] = {0};
    int i = 0;
    int rc = 0;
    MSG_Q_ID msgQTest = (MSG_Q_ID)param;

    while(1) {
        memset(buf, 0, sizeof(buf));
        rc = msgQReceive(msgQTest, buf, sizeof(buf), WAIT_FOREVER);
        if(rc != 0) {
            printf("receive message failed in %s.\n", __func__);
            exit(1);
        }
        printf("Recv %08d time of %s\n", i, buf);
        Sleep(100);
        i++;
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

    msgQTest = msgQCreate(maxMsgs, 100, MSG_Q_FIFO);
    if(msgQTest == NULL) {
        printf("create message queue failed in %s.\n", __func__);
        exit(1);
    }

    hSender = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQSender,
    		msgQTest, 0, (DWORD*)&tSender);

    hReceiver = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQReceiver,
    		msgQTest, 0, (DWORD*)&tReceiver);

    while((c = getchar()) != 'q') {
        msgQShow(msgQTest);
    }

    CloseHandle(hReceiver);
    CloseHandle(hSender);

    msgQDelete(msgQTest);

    return 0;
}
