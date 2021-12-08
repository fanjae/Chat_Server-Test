#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#pragma warning (disable:4996)
#pragma commnet (lib,"ws2_32.lib")

#define BUF_SIZE 100
#define READ 3
#define WRITE 5

typedef struct // socket info
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// Ŭ���̾�Ʈ�� ����� ���������� ��� ���� ����ü
// ������ �����Ҵ�ǰ� ��� ���޵ǰ� ��� Ȱ��Ǵ��� ����

typedef struct // buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode; // READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;

// IO�� ���Ǵ� ����
// Overlapped I/O �� �ݵ�� �ʿ��� Overlapped ����ü ������ ��Ƽ� ����ü�� �����Ͽ���.
// ����ü ������ �ּҰ��� ����ü ù ��° ����� �ּҰ��� ��ġ.


DWORD WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(const char *message);

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags = 0;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() Error");
	}

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); // CP ������Ʈ ���� , �ھ��� �� ��ŭ �����尡 CP ������Ʈ�� �Ҵ� ����.
	GetSystemInfo(&sysInfo); // ���� �������� �ý��� ���� Ȯ��
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) 
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}
	/* dwNumberOfProcessor : CPU �� ����.
	CPU�� �� ��ŭ ������ ���� ��. 15�࿡�� ������ CP ������Ʈ �ڵ� ����.
	������� �� �ڵ�� ���� CP ������Ʈ�� �Ҵ�
	*/

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
	listen(hServSock, 5);

	while (1)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		// PER_HANDLE_DATA ����ü ������ ���� �Ҵ� ���� Ŭ���̾�Ʈ�� ����� ����, Ŭ���̾�Ʈ �ּ� ���� �Ҵ�.

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0); 
		/* 81��° �࿡�� ������ ���� ����. Overlapped IO�� �Ϸ�� ����� CP ������Ʈ�� �Ϸ�� ���� �Ҵ���. */

		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA)); // PER_IO_DATA ����ü ���� ���� �Ҵ�. WSARecv �Լ� ȣ�⿡ �ʿ��� Data���� �ѹ��� �������.
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ; // IOCP�� �Է� ����� �ϷḦ ������������ ����. ���� �̸� ������ ����ص־��Ѵ�. PER_IO_DATA�� rwMode�� �ٷ� �̷��� �������� ����.
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);

		// OVERLAPPED ����ü ������ �ּҰ��� ����. �� ���� getQueued �Լ��� ��ȯ�ϸ� ���� �� ����.
		// �̴� PER_IO_DATA ����ü ������ �ּҰ��� ������ �Ͱ� ����.
	}
	return 0;
}

DWORD WINAPI EchoThreadMain(LPVOID pComPort)
{
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	DWORD flags = 0;

	
	while(1)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->rwMode == READ)
		{
			puts("message received!");
			if (bytesTrans == 0) // EOF ���� ��
			{
				closesocket(sock);
				free(handleInfo);
				free(ioInfo);
				continue;
			}

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsabuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsabuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}

	}
	
}
