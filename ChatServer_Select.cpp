
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

char* ids[64];
SOCKET clients[64];
int numClients;

CRITICAL_SECTION cs;

DWORD WINAPI ClientThread(LPVOID);


int main()
{
	char cmd[32], id[32], tmp[32];
	char buf[256];

	InitializeCriticalSection(&cs);

	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);

	numClients = 0;

	while (1)
	{
		printf("Waiting for new client...\n");
		SOCKET client = accept(listener, NULL, NULL);
		printf("New client accepted: %d\n", client);

		CreateThread(0, 0, ClientThread, &client, 0, 0);
		const char* okMsg = "[SERVER] Xin chao. Hay nhap id theo cu phap [CONNECT] [your_id].\n";
		send(client, okMsg, strlen(okMsg), 0);
	}

	DeleteCriticalSection(&cs);
}

DWORD WINAPI ClientThread(LPVOID lpParam)
{
	SOCKET client = *(SOCKET*)lpParam;

	char buf[256];
	int ret;

	char cmd[16], id[32], tmp[32], msg[32];

	const char* connectErrMsg = "[CONNECT] ERROR Dang nhap that bai.\n";
	const char* errorMSG = "[ERROR] SAI LENH/GIAO THUC\n";
	BOOL isLogin = false;
	// Xac thuc id cua client
	while (1)
	{
		if (!isLogin) {
			ret = recv(client, buf, sizeof(buf), 0);
			if (ret <= 0)
			{
				closesocket(client);
				return 0;
			}

			buf[ret] = 0;
			printf("%s\n", buf);

			// Xu ly du lieu
			ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
			if (ret == 2)
			{
				if (strcmp(cmd, "[CONNECT]") == 0)
				{
					int isExisted = 0;
					for (int i = 0; i < numClients; i++) {
						if (strcmp(ids[i], id) == 0) {
							isExisted = 1;
						}
					}
					if (!isExisted) {
						const char* okMsg = "[CONNECT] OK - Dang nhap thanh cong\n";
						send(client, okMsg, strlen(okMsg), 0);
						isLogin = true;
						// gui thong bao co user dang nhap
						char sendBuf[256];
						for (int i = 0; i < numClients; i++) {
							if (clients[i] != client) {
								strcpy(sendBuf, "[USER] ");
								strcat(sendBuf, "CONNECT ");
								strcat(sendBuf, id);
								send(clients[i], sendBuf, strlen(sendBuf), 0);
							}
						}
						// Luu thong tin cua client dang nhap thanh cong
						EnterCriticalSection(&cs);
						ids[numClients] = id;
						clients[numClients] = client;
						numClients++;
						LeaveCriticalSection(&cs);
					}
					else {
						send(client, connectErrMsg, strlen(connectErrMsg), 0);
					}

				}
				else
				{
					send(client, connectErrMsg, strlen(connectErrMsg), 0);
				}
			}
			else
			{
				send(client, connectErrMsg, strlen(connectErrMsg), 0);
			}
		}
		else {
			// Forward messages
			ret = recv(client, buf, sizeof(buf), 0);
			if (ret <= 0)
			{
				closesocket(client);
				return 0;
			}

			buf[ret] = 0;
			printf("Received: %s\n", buf);

			ret = sscanf(buf, "%s %s %s", cmd, tmp, msg);
			if (ret < 1)
			{
				send(client, errorMSG, strlen(errorMSG), 0);
				continue;
			}

			char sendBuf[256];

			// SEND ALL
			if (strcmp(cmd, "[SEND]") == 0)
			{
				if (strcmp(tmp, "ALL") == 0) {

					for (int i = 0; i < numClients; i++) {
						if (clients[i] != client) {
							strcpy(sendBuf, "[MESSAGE] ");
							strcat(sendBuf, "ALL ");
							strcat(sendBuf, msg);
							send(clients[i], sendBuf, strlen(sendBuf), 0);
						}
						else {
							strcpy(sendBuf, "[SEND] ALL OK\n");
							send(client, sendBuf, strlen(sendBuf), 0);
						}
					}
				}
				else {
					int i = 0;
					int isExisted = 0;
					for (; i < numClients; i++) {
						if (strcmp(tmp, ids[i]) == 0) {
							isExisted = 1;
							printf("Existed");
							break;
						}
					}
					if (isExisted) {
						strcpy(sendBuf, "[MESSAGE] ");
						strcat(sendBuf, id);
						strcat(sendBuf, " ");
						strcat(sendBuf, msg);
						send(clients[i], sendBuf, strlen(sendBuf), 0);
						strcpy(sendBuf, "[SEND] OK\n");
						send(client, sendBuf, strlen(sendBuf), 0);
					}
					else {
						strcpy(sendBuf, "[SEND] ERROR - GUI TIN NHAN THAT BAI");
						send(client, sendBuf, strlen(sendBuf), 0);
					}
				}
			}
			else if (strcmp(cmd, "[LIST]") == 0) {
				strcpy(sendBuf, "[LIST] ");
				strcat(sendBuf, "OK ");
				for (int i = 0; i < numClients; i++) {
					strcat(sendBuf, ids[i]);
					if(i < numClients - 1) strcat(sendBuf, ", ");
				}
				send(client, sendBuf, strlen(sendBuf), 0);
			}
			else if (strcmp(cmd, "[DISCONNECT]") == 0) {
				strcpy(sendBuf, "[DISCONNECT] ");
				int i = 0;
				for (; i < numClients; i++) {
					if (clients[i] == client) {
						strcat(sendBuf, "OK");
						send(clients[i], sendBuf, strlen(sendBuf), 0);
						EnterCriticalSection(&cs);
						if (i < numClients - 1)
							ids[i] = ids[numClients - 1];
						numClients = numClients - 1;
						LeaveCriticalSection(&cs);
						isLogin = false;
					}
				}
				if (i >= numClients) {
					strcat(sendBuf, "[DISCONNECT] ERROR");
					send(clients[i], sendBuf, strlen(sendBuf), 0);
				}
				else
				{
					// gui thong bao co user dang xuat
					for (int i = 0; i < numClients; i++) {
						if (clients[i] != client) {
							strcpy(sendBuf, "[USER] ");
							strcat(sendBuf, "DISCONNECT ");
							strcat(sendBuf, id);
							send(clients[i], sendBuf, strlen(sendBuf), 0);
						}
					}
				}
			}
			else {
				send(client, errorMSG, strlen(errorMSG), 0);
			}
		}
	}

	return 0;
}
