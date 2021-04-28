#include <stdio.h>

#include <winsock2.h>

#pragma comment(lib, "ws2_32")

DWORD WINAPI ClientThread(LPVOID);

int main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);
	
	char buf[256], fbuf[256];
	char user[32], pass[32], tmp[32];
	fd_set master;
	FD_ZERO(&master);
	FD_SET(listener, &master);

	while (true) {
		fd_set copy = master;
		int socketCount = select(0, &copy, NULL, NULL, NULL);
		for (int i = 0; i < socketCount; i++) {
			SOCKET sock = copy.fd_array[i];
			if (sock == listener) {
				SOCKET client = accept(listener, NULL, NULL);
				FD_SET(client,&master);
				while (1) {
					int ret = recv(client, buf, sizeof(buf), 0);
					if (ret <= 0)
					{
						closesocket(client);
						return 1;
					}

					buf[ret] = 0;

					int n = sscanf(buf, "%s %s %s", user, pass, tmp);
					if (n == 2)
					{
						// So sanh voi CSDL trong file
						FILE* f = fopen("C:\\test\\accounts.txt", "r");
						int found = 0;
						while (!feof(f))
						{
							fgets(fbuf, sizeof(buf), f);
							if (strcmp(buf, fbuf) == 0)
							{
								found = 1;
								break;
							}
						}
						fclose(f);
						if (found == 1)
						{
							// Tim thay tai khoan
							const char* msg = "Dang nhap thanh cong. Hay nhap lenh de thuc hien.\n";
							send(client, msg, strlen(msg), 0);

							break;
						}
						else
						{
							// Sai tai khoan
							const char* msg = "Sai tai khoan. Hay nhap lai.\n";
							send(client, msg, strlen(msg), 0);
						}
					}
					else
					{
						// Sai cu phap
						const char* msg = "Sai cu phap. Hay nhap lai.\n";
						send(client, msg, strlen(msg), 0);
					}
				}
			}
			else {
				while (1) {
					int ret = recv(sock, buf, sizeof(buf), 0);
					if (ret <= 0)
					{
						closesocket(sock);
						FD_CLR(sock, &master);
						return 1;
					}

					// Xoa ky tu xuong dong
					buf[ret - 1] = 0;

					char cmd[256];
					sprintf(cmd, "%s > c:\\test\\output.txt", buf);
					system(cmd);

					// Doc file ket qua va gui cho client
					FILE* f = fopen("C:\\test\\output.txt", "r");
					while (!feof(f))
					{
						fgets(fbuf, sizeof(buf), f);
						send(sock, fbuf, strlen(fbuf), 0);
					}
					fclose(f);
				}
			}
		}
	}

	WSACleanup();
	
}
