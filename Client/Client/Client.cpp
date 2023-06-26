#include <iostream>
#include <WinSock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

SOCKET connection;
bool EXIT_PROGRAM;


/**
 * \brief Changes the color of the text in the console. 
 *  Accepts the color code as a parameter.
 */
void changeTextColor(int colorCode) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (colorCode == 0x0008) {
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	} else {
		SetConsoleTextAttribute(hConsole, colorCode);
	}
}


/**
 * \brief Accepts messages from other users.
 *
 * \context The function works in a separate thread. It checks the connection to the server. 
 * If a message is received from other users, it is displayed on the console.
 */
void ClientHandler() {
	int msg_size;
	char userName[32];
	while (true) {
		bool connection_close = false;

		if (recv(connection, userName, sizeof(userName), NULL) == SOCKET_ERROR)
			connection_close = true;
		if (recv(connection, (char*)&msg_size, sizeof(int), NULL) == SOCKET_ERROR)
			connection_close = true;

		char* msg = new char[msg_size + 1];
		msg[msg_size] = '\0';

		if (recv(connection, msg, msg_size, NULL) == SOCKET_ERROR)
			connection_close = true;

		if (connection_close) {
			closesocket(connection);
			EXIT_PROGRAM = true;
			delete[] msg;
			return;
		}

		changeTextColor(FOREGROUND_BLUE);
		std::cout << userName << ":";
		changeTextColor(FOREGROUND_INTENSITY);
		std::cout <<" " << msg << std::endl;
		changeTextColor(FOREGROUND_GREEN);

		delete[] msg;
	}
}


/**
 * \brief The function initializes a socket.
 *
 * \context The function sets parameters of the connection to the server, 
 * such as a ip address of the server and a port. 
 * The function also creates a separate thread to receive messages from other users, 
 * and also sends messages to other users.
 */
int main(int argc, char* argv[]) {

	WSADATA wsaData;
	WORD DLLVersion = MAKEWORD(2, 2);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "WSADATA Error" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeSockAddr = sizeof(addr);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	std::cout << "Enter your user name" << std::endl;
	char userName[32];
	std::cin >> userName;
	system("cls");

	connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Connection Failed" << std::endl;
		exit(1);
	}

	changeTextColor(FOREGROUND_RED);
	std::cout << "Connected. Your user name: " << userName << std::endl;
	changeTextColor(FOREGROUND_INTENSITY);

	send(connection, userName, sizeof(userName), NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

	std::string msg1;
	std::cin.ignore(32, '\n');
	while (!EXIT_PROGRAM) {
		changeTextColor(FOREGROUND_GREEN);
		std::getline(std::cin, msg1);
		int msg_size = msg1.size();
		send(connection, (char*)&msg_size, sizeof(int), NULL);
		send(connection, msg1.c_str(), msg_size, NULL);
		Sleep(10);
	}
	
	system("pause");
	return 0;
}