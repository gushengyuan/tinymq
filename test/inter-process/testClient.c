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
    HANDLE hReceiver = NULL;
    unsigned int tReceiver = 0;
    char c = 0;
    MSG_Q_ID msgQTest = NULL;
    int maxMsgs = 3;

    /* Fix the eclipse CDT output issue */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    msgQTest = msgQOpen("test");
    if(msgQTest == NULL) {
		msgQTest = msgQCreateEx(maxMsgs, 100, MSG_Q_FIFO, "test");
		printf("%s: message queue created.\n", __func__);
    }
	else {
		printf("%s: message queue opened.\n", __func__);
	}

    hReceiver = (HANDLE)CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)msgQReceiver,
    		msgQTest, 0, (DWORD*)&tReceiver);

    while((c = getchar()) != 'q') {
        msgQShow(msgQTest);
    }

    CloseHandle(hReceiver);

    msgQShow(msgQTest);

    msgQDelete(msgQTest);

    return 0;
}
