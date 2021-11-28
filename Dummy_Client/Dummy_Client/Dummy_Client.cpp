#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>
#include <process.h>

#pragma warning (disable:4996)
#pragma comment (lib,"ws2_32.lib")
#define BUF_SIZE 100
#define NAME_SIZE 20
#define TEST_USER 50

unsigned WINAPI SendMsg(void *arg);
unsigned WINAPI RecvMsg(void *arg);
void ErrorHandling(const char *msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

typedef struct user_information
{
	char ID[BUF_SIZE];
	SOCKET hSock;
}Client;

Client list[TEST_USER];
int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hSock[TEST_USER];
	SOCKADDR_IN servAdr[TEST_USER];
	HANDLE hSndThread[1005], hRevThread[1005];

	if (argc != 3)
	{
		printf("Usage : %s <IP> <port> ", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAstartup() error!");
	}

	// sprintf(name, "[%s]", argv[3]);
	for (int i = 0; i < TEST_USER; i++)
	{
		hSock[i] = socket(PF_INET, SOCK_STREAM, 0);
		memset(&servAdr[i], 0, sizeof(servAdr[i]));
		servAdr[i].sin_family = AF_INET;
		servAdr[i].sin_addr.s_addr = inet_addr(argv[1]);
		servAdr[i].sin_port = htons(atoi(argv[2]));

		if (connect(hSock[i], (SOCKADDR*)&servAdr[i], sizeof(servAdr[i])) == SOCKET_ERROR)
		{
			ErrorHandling("connect() error");
		}

		char temp_ID[50];
		sprintf(temp_ID, "User%d", i);
		strcpy(list[i].ID, temp_ID);
		list[i].hSock = hSock[i];

		hSndThread[i] = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void *)&list[i], 0, NULL);
		hRevThread[i] = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void *)&hSock[i], 0, NULL);
		
	}
	for (int i = 0; i < TEST_USER; i++)
	{
		WaitForSingleObject(hSndThread[i], INFINITE);
		WaitForSingleObject(hRevThread[i], INFINITE);
	}


	for (int i = 0; i < TEST_USER; i++)
	{
		closesocket(hSock[i]);
	}

	WSACleanup();
	return 0;
}
unsigned WINAPI SendMsg(void *arg) // send thread main
{
	SOCKET hSock = ((Client *)arg)->hSock;
	int cnt = 0;
	
	char nameMsg[NAME_SIZE + BUF_SIZE];
	while (1)
	{
		int strLen = 0;
		strcpy(msg, "Test");
		sprintf(nameMsg, "%s:%s\n", ((Client *)arg)->ID, msg);
		strLen = strlen(nameMsg);
		send(hSock, nameMsg, strLen, 0);
		Sleep(1000);
	}
	return 0;
}
unsigned WINAPI RecvMsg(void *arg) // read thread main
{
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	int strLen;
	while (1)
	{
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (strLen == -1)
		{
			return -1;
		}
		nameMsg[strLen] = 0;
		fputs(nameMsg, stdout);
	}
	return 0;
}

void ErrorHandling(const char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}