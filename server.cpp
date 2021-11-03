#include <iostream>
#include <string>
#include <winsock2.h>
#include <vector>
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;
#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT 5555
#define BUFFER_SIZE 1024

static int currentId = 0;

DWORD WINAPI newStream(LPVOID lpParam);
bool authOK(std::string login, std::string password);

struct Client {
	int Id;
	SOCKET client_sock;
	SOCKADDR_IN address;
	std::string login;
	bool isAuth = false;
	Client(SOCKADDR_IN serv_address) {
		Id = ++currentId;
		address = serv_address;
	}
};

static std::vector<Client*> clients;

int main(int argc, char const* argv[]) {
	WSAData w_data;
	WSAStartup(MAKEWORD(2, 2), &w_data);
	SOCKET serv_socket;
	SOCKADDR_IN address;

	serv_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (serv_socket == INVALID_SOCKET) {
		std::cout << "ERROR socket\n";
		exit(-1);
	}

	address.sin_port = htons(DEFAULT_PORT);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htons(INADDR_ANY);
	if (bind(serv_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		std::cout << "ERROR bind\n";
		exit(-1);
	} 

	if (listen(serv_socket, 10) == SOCKET_ERROR) { //Если не удалось получить запрос
		std::cout << "Listen function failed with error:" << WSAGetLastError() << std::endl;
		exit(-1);
	}
	
	while (true) {
		clients.push_back(new Client(address));
		int size = sizeof(address);
		if (clients.back()->client_sock = accept(serv_socket, (SOCKADDR*)&address, &size)) {
			HANDLE t = CreateThread(NULL, 0, newStream, clients.back(), 0, NULL); //Создание потока для получения данных
			if (t == NULL) { //Ошибка создания потока
				std::cout << "Thread Creation Error: " << WSAGetLastError() << std::endl;
			}
		}
	}
	closesocket(serv_socket);
	WSACleanup();
	return 0;
}

DWORD WINAPI newStream(LPVOID lpParam) {
	Client client = *(Client*)lpParam;
	std::cout << "Client connected!" << std::endl;
	char buffer[BUFFER_SIZE];

	while (true) { //Цикл работы сервера
		if (recv(client.client_sock, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
			std::cout << "recv function failed with error " << WSAGetLastError() << std::endl;
			break;
		}
		json j = json::parse(buffer);
		
		if (j["command"] == "HELLO") {
			json p = {{"id", client.Id}, {"command", "HELLO"}, {"auth_method", "plain-text"}};
			if (send(client.client_sock, p.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
				std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
				return -1;
			}
		}
		
		if (j["command"] == "login") {
			if (authOK(j["login"], j["password"])) {
				client.login = j["login"];
				client.isAuth = true;
				j = { {"id",client.Id}, {"command", "login"}, {"status", "ok"}, {"session", "UUID"} };
			}
			else {
				j = {{"id",client.Id}, {"command", "login"}, {"status", "failed"}, {"message", "Wrong login or password"}};
			}
			if (send(client.client_sock, j.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
				std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
				return -1;
			}
		}
		if (j["command"] == "message") {
			json p = {{"id",client.Id}, {"command", "message_reply"}, {"status", "ok"}, {"client_id", "id"}};
			if (send(client.client_sock, p.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
				std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
				return -1;
			}
			for (int i = 0; i < clients.size() - 1; ++i) {
				if (clients[i]->isAuth){
					json text = {{"id",clients[i]->Id}, {"command", "message"}, {"body", j["body"]}, {"sender_login", client.login}, {"session", "UUID"}};
					if (send(clients[i]->client_sock, text.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
						std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
						return -1;
					}
				}
			}
		}
		if (j["command"] == "ping") {
			json p = { {"id", client.Id}, {"command", "ping_reply"}, {"session", "UUID"} };
			if (send(client.client_sock, p.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
				std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
				return -1;
			}
		}
		if (j["command"] == "logout") {
			json p = { {"id", client.Id}, {"command", "logout_reply"}, {"session", "UUID"} };
			if (send(client.client_sock, p.dump().c_str(), BUFFER_SIZE, 0) == SOCKET_ERROR) {
				std::cout << "send function failed with error: " << WSAGetLastError() << std::endl;
				return -1;
			}
			client.isAuth = false;
			break;
		}
		std::cout << "Client " << client.Id << " : " << buffer << std::endl;
	}
	return 1;
}

bool authOK(std::string login, std::string password) {
	std::string line;

	std::ifstream in("database.txt"); 
	bool flag = false;
	if (in.is_open()) {
		while (getline(in, line)) {
			json j = json::parse(line);
			if ((login == j["login"]) && (password == j["password"]))
				flag = true;
		}
	}
	else
		std::cout << "ERROR\n";
	return flag;
}

