#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#define BUF_SIZE 1024
void ErrorHandling(const char *message);

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;
