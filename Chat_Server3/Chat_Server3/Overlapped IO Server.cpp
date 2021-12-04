#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 1024
#pragma comment (lib,"ws2_32.lib")
#pragma warning (disable:4996)

void CALLBACK ReadCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK WriteCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHandling(const char * message);

typedef struct
{
	SOCKET hClntSock;
	char buf[BUF_SIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;
/* 소켓의 핸들과 버퍼,
버퍼 관련 정보를 담는 WSABUF형 변수 */

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hLisnSock, hRecvSock;
	SOCKADDR_IN lisnAdr, recvAdr;
	LPWSAOVERLAPPED lpOvLp = NULL;
	DWORD recvBytes;
	LPPER_IO_DATA hbInfo;
	int mode = 1, recvAdrSz, flagInfo = 0;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() error!");
	}

	hLisnSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	ioctlsocket(hLisnSock, FIONBIO, (u_long *)&mode); // for non-blocking mode socket
	// hLisnSock을 Non Blocking Mode로 변환.


	memset(&lisnAdr, 0, sizeof(lisnAdr));
	lisnAdr.sin_family = AF_INET;
	lisnAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	lisnAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hLisnSock, (SOCKADDR*)&lisnAdr, sizeof(lisnAdr)) == SOCKET_ERROR)
	{
		ErrorHandling("bind() error");
	}
	if (listen(hLisnSock, 5) == SOCKET_ERROR)
	{
		ErrorHandling("listen() error");
	}

	recvAdrSz = sizeof(recvAdr);
	while (1)
	{
		SleepEx(100, TRUE);
		hRecvSock = accept(hLisnSock, (SOCKADDR*)&recvAdr, &recvAdrSz); 
		if (hRecvSock == INVALID_SOCKET) 
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK) 
			{
				continue;
			}
			else
			{
				ErrorHandling("Accept() Error");
			}
		}
		puts("Client Connected....");
		lpOvLp = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED)); // Overlapped IO에 필요한 구조체 변수 할당 및 초기화
		memset(lpOvLp, 0, sizeof(WSAOVERLAPPED)); 

		hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA)); // PER_IO_DATA 구조체 변수 동적 할당 및 할당된 소켓의 핸들 정보 저장.
		hbInfo->hClntSock = (DWORD)hRecvSock;
		(hbInfo->wsaBuf).buf = hbInfo->buf;
		(hbInfo->wsaBuf).len = BUF_SIZE;

		lpOvLp->hEvent = (HANDLE)hbInfo; // hEvent에 할당한 변수의 주소값 저장.
		WSARecv(hRecvSock, &(hbInfo->wsaBuf), 1, &recvBytes, (LPDWORD) &flagInfo, lpOvLp, ReadCompRoutine);
		// WSARecv함수를 호출하면서 ReadCompRoutine 함수를 Completion Routine으로 지정함.
		// WSAOVERLAPPED 구조체 변수의 주소값은 Completion Routine의 세번째 매개변수에 전달됨.

	}
	closesocket(hRecvSock);
	closesocket(hLisnSock);
	WSACleanup();
	return 0;
}

// 데이터의 입력이 완료됨. 수신된 데이터를 에코 클라이언트에게 전송해야함.
void CALLBACK ReadCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{

	// 입력이 완료된 소켓의 핸들정보와 버퍼정보를 추출
	// PER_IO_DATA 구조체 변수의 주소값을 WSAOVERLAPPED 구조체 변수의 멤버 hEvent에 저장했기 때문.
	LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock = hbInfo->hClntSock;
	LPWSABUF bufInfo = &(hbInfo->wsaBuf);
	DWORD sentBytes;

	if (szRecvBytes == 0)
	{
		closesocket(hSock);
		free(lpOverlapped->hEvent);
		free(lpOverlapped);
		puts("Client disconnected....");
	}
	else
	{
		bufInfo->len = szRecvBytes;
		WSASend(hSock, bufInfo, 1, &sentBytes, 0, lpOverlapped, WriteCompRoutine);
	}
}

void CALLBACK WriteCompRoutine(DWORD dwError, DWORD szSendBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock = hbInfo->hClntSock;
	LPWSABUF bufInfo = &(hbInfo->wsaBuf);
	DWORD recvBytes;
	int flagInfo = 0;
	WSARecv(hSock, bufInfo, 1, &recvBytes, (LPDWORD) &flagInfo, lpOverlapped, ReadCompRoutine);
}

void ErrorHandling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
