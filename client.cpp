#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json; 
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

DWORD WINAPI clientReceive(LPVOID lpParam);
DWORD WINAPI clientSend(LPVOID lpParam);
void HELLO(SOCKET client_socket, SOCKADDR_IN address);

int main(int argc, char const* argv[]) {
	WSAData w_data;
	SOCKET client_socket;
	SOCKADDR_IN address;
	WSAStartup(MAKEWORD(2, 2), &w_data);

	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		std::cout << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
		return -1;
	}
	HELLO(client_socket, address);

	HANDLE t1 = CreateThread(NULL, 0, clientReceive, &client_socket, 0, NULL);
	if (t1 == NULL) {
		std::cout << "Thread creation error: " << GetLastError();
		exit(-1);
	}
	HANDLE t2 = CreateThread(NULL, 0, clientSend, &client_socket, 0, NULL);
	if (t2 == NULL) {
		std::cout << "Thread creation error: " << GetLastError();
		exit(-1);
	}
	WaitForSingleObject(t1, INFINITE);
	WaitForSingleObject(t2, INFINITE);
	closesocket(client_socket);
	WSACleanup(); 
	return 0;
}

void HELLO(SOCKET client_socket, SOCKADDR_IN address) {
	std::cout << "Write \"HELLO\" to connect to " << SERVER_IP << " with port " << DEFAULT_PORT << std::endl;
	char buffer[BUFFER_SIZE]; 
	fgets(buffer, BUFFER_SIZE, stdin);
	while (strcmp(buffer, "HELLO\n") ) {
		fgets(buffer, BUFFER_SIZE, stdin);
	}
	address.sin_port = htons(DEFAULT_PORT);
	address.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &address.sin_addr);
	if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		std::cout << "Server connection failed with error: " << WSAGetLastError() << std::endl;
		exit(-1);
	}
	if (recv(client_socket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
		std::cout << "recv function failed with error: " << WSAGetLastError() << std::endl;
		exit(-1);
	}

	if (send(client_socket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
		std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
		exit(-1);
	}
}
DWORD WINAPI clientReceive(LPVOID lpParam) { //Получение данных от сервера
	char buffer[BUFFER_SIZE];
	SOCKET client_socket = *(SOCKET*)lpParam;
	while (true) {
		if (recv(client_socket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
			std::cout << "recv function failed with error: " << WSAGetLastError() << std::endl;
			return -1;
		}
		if (strcmp(buffer, "exit\n") == 0) {
			std::cout << "Server disconnected." << std::endl;
			return 1;
		}
		std::cout << "Server: " << buffer;
		memset(buffer, 0, sizeof(buffer));
	}
	return 1;
}

DWORD WINAPI clientSend(LPVOID lpParam) {
	char buffer[BUFFER_SIZE];
	SOCKET client_socket = *(SOCKET*)lpParam;
	while (true) {
		fgets(buffer, BUFFER_SIZE, stdin);
		if (send(client_socket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
			std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
			return -1;
		}
		if (strcmp(buffer, "exit\n") == 0) {
			std::cout << "Client disconnected." << std::endl;
			return 1;
		}
	}
	return 1;
}