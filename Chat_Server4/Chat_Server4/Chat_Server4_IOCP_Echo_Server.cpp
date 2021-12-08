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

// 클라이언트와 연결된 소켓정보를 담기 위한 구조체
// 변수가 언제할당되고 어떻게 전달되고 어떻게 활용되는지 관찰

typedef struct // buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode; // READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;

// IO에 사용되는 버퍼
// Overlapped I/O 에 반드시 필요한 Overlapped 구조체 변수를 담아서 구조체를 정의하였다.
// 구조체 변수의 주소값은 구조체 첫 번째 멤버의 주소값과 일치.


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

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); // CP 오브젝트 생성 , 코어의 수 만큼 쓰레드가 CP 오브젝트에 할당 가능.
	GetSystemInfo(&sysInfo); // 현재 실행중인 시스템 정보 확인
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) 
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}
	/* dwNumberOfProcessor : CPU 수 저장.
	CPU의 수 만큼 쓰레드 생성 후. 15행에서 생성한 CP 오브젝트 핸들 전달.
	쓰레드는 위 핸들로 인해 CP 오브젝트에 할당
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

		// PER_HANDLE_DATA 구조체 변수를 동적 할당 이후 클라이언트와 연결된 소켓, 클라이언트 주소 정보 할당.

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0); 
		/* 81번째 행에서 생성된 소켓 연결. Overlapped IO가 완료시 연결된 CP 오브젝트에 완료된 정보 할당함. */

		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA)); // PER_IO_DATA 구조체 변수 동적 할당. WSARecv 함수 호출에 필요한 Data들을 한번에 가지고옴.
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ; // IOCP는 입력 출력의 완료를 구분지어주지 않음. 따라서 이를 별도로 기록해둬야한다. PER_IO_DATA의 rwMode가 바로 이러한 목적으로 삽입.
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);

		// OVERLAPPED 구조체 변수의 주소값을 전달. 이 값은 getQueued 함수가 반환하며 얻을 수 있음.
		// 이는 PER_IO_DATA 구조체 변수의 주소값을 전달한 것과 동일.
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
			if (bytesTrans == 0) // EOF 전송 시
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
