#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#pragma comment (lib,"ws2_32.lib")
#define BUF_SIZE 120
#define MAX_CLNT 1024

unsigned WINAPI HandleClnt(void *arg);
void SendMsg(char *msg, int len);
void ErrorHandling(const char *msg);

int clntCnt = 0;
HANDLE hMutex;

typedef struct Client_list
{
	SOCKET ClntSock;
	int Number = 0;
}Client_list;

Client_list All_Client[MAX_CLNT];

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSz;
	int Connect_Client_cnt = 1;
	HANDLE hThread;

	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() error!");
	}

	hMutex = CreateMutex(NULL, FALSE, NULL);
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		ErrorHandling("bind() error");
	}

	if (listen(hServSock, 150) == SOCKET_ERROR)
	{
		ErrorHandling("listen() error");
	}
	printf("IT's RUN\n");
	while (1)
	{
		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

		WaitForSingleObject(hMutex, INFINITE);
		All_Client[clntCnt].ClntSock = hClntSock;
		All_Client[clntCnt].Number = clntCnt + 1;
		clntCnt++;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&All_Client[clntCnt - 1], 0, NULL);


		printf("Connect client : %s, Client_cnt : %d \n", inet_ntoa(clntAdr.sin_addr), Connect_Client_cnt++);
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}
unsigned WINAPI HandleClnt(void *arg)
{
	SOCKET hClntSock = ((Client_list*)arg)->ClntSock;
	int number = ((Client_list*)arg)->Number;
	int strLen = 0;
	char msg[BUF_SIZE];
	
	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) != 0)
	{
		msg[strLen] = 0;
		printf("[Logged] : %s", msg);
		SendMsg(msg, strLen);

	}
	

	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < clntCnt; i++)
	{
		if (hClntSock == All_Client[i].ClntSock)
		{
			while (i++ < clntCnt - 1)
			{
				All_Client[i].ClntSock = All_Client[i+1].ClntSock;
			}
			break;
		}
	}
	clntCnt--;
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;
}
void SendMsg(char *msg, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		send(All_Client[i].ClntSock, msg, len, 0);
	}
	ReleaseMutex(hMutex);
}
void ErrorHandling(const char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}


