#include <iostream>
#include <WinSock2.h>
#include <set>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

std::mutex blockMutex;
std::set<long> usersID;
size_t ClientCount;
struct HOST;
void ClientHandler(HOST* host);


/**
 * Structure for establishing connections with users.
 */
struct HOST {
	SOCKET		 connection;		// connection socket.
	size_t		 userID;			// unique user id.
	HOST*		 next;				// the next host in the list.
	static HOST* first;				// the first host in the list.
	HANDLE		 threadID;			// thread id.
	char		 userName[32];		// user name.
};
HOST* HOST::first = NULL;


/**
 * \brief The function displays the number of online users 
 * currently connected to the server.
 */
void printOnline() {
	system("cls");
	system("color 0B");
	std::cout << "Online: " << ClientCount << std::endl;
}


/**
 * \brief The function returns a unique user id.
 *
 * \context The function checks which id are available. 
 * After allocating a unique id for the user, 
 * the function puts the value in set so that you can manage this id.
 *
 * \return Returns a unique id.
 */
size_t getFreeID() {
	long available_id = 0;
	blockMutex.lock();
	if (!usersID.empty()) {
		for (auto it = usersID.begin(); it != usersID.end(); ++it, ++available_id) {
			if (*it != available_id)
				break;
		}
	}
	usersID.insert(available_id);
	ClientCount++;
	blockMutex.unlock();
	return available_id;
}


/**
 * \brief The function adds a host to the list.
 *
 * \context The function creates a HOST structure, initializes it, 
 * and then starts a separate thread for the user.
 */
void addList(SOCKET& newConnection, char *userName) {
	HOST* newHost = new HOST();
	newHost->connection = newConnection;
	newHost->userID = getFreeID();
	newHost->next = NULL;

	if (newHost->first) {
		HOST* currentHost = newHost->first;
		while (currentHost->next)
			currentHost = currentHost->next;

		currentHost->next = newHost;
	}
	else {
		newHost->first = newHost;
	}

	strcpy(newHost->userName, userName);

	newHost->threadID = CreateThread(
		NULL, 
		NULL, 
		(LPTHREAD_START_ROUTINE)ClientHandler, 
		newHost, 
		NULL, 
		NULL
	);
}


/**
 * \brief Frees up memory and closes the connection with the user.
 *
 * \context The function closes the socket and the stream descriptor. 
 * Reduces the user counter and removes the id from the set.
 * And then outputs the number of users online.
 */
void Destructor(HOST* host) {
	HOST* current = HOST::first;
	HOST* previous = NULL;

	closesocket(host->connection);
	CloseHandle(host->threadID);

	while (current) {
		if (host->userID == current->userID)
		{
			blockMutex.lock();
			usersID.erase(host->userID);
			ClientCount--;
			blockMutex.unlock();

			if (previous == NULL) {
				HOST::first = current->next;
				delete current;
				break;
			} else {
				previous->next = current->next;
				delete current;
				break;
			}
		}
		previous = current;
		current = current->next;
	}
	printOnline();
}


/**
 * \brief The function sends a broadcast message to all users except the sender.
 */
void broadcast(char *msg, int msg_size, HOST *host) {
	HOST* current = HOST::first;
	while (current) {
		if (current->userID != host->userID) {
			send(current->connection, host->userName, sizeof(host->userName), NULL);	// user name
			send(current->connection, (char*)&msg_size, sizeof(int), NULL);				// message size in bytes
			send(current->connection, msg, msg_size, NULL);								// message
		}
		current = current->next;
	}
}


/**
 * \brief The function receives messages from users 
 * and sends them to everyone using a broadcast message.
 */
void ClientHandler(HOST *host) {
	printOnline();

	int msg_size;
	while (true) {
		if (recv(host->connection, (char*)&msg_size, sizeof(int), NULL) == SOCKET_ERROR) {
			Destructor(host);
			return;
		}

		char* msg = new char[msg_size + 1];
		msg[msg_size] = '\0';

		if (recv(host->connection, msg, msg_size, NULL) == SOCKET_ERROR) {
			Destructor(host);
			delete[] msg;
			return;
		}
		broadcast(msg, msg_size, host);

		delete[] msg;
	}
}


/**
 * \brief The function initializes a socket.
 *
 * \context The function sets the server parameters,
 * such as an ip address and a port, network protocol.
 * Also accepts connections with users and adds connections to the list.
 */
int main(int argc, char* argv[]) {

	WSADATA wsaData;
	WORD DLLVersion = MAKEWORD(2, 2);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeSockAddr = sizeof(addr);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(slisten, (SOCKADDR*)&addr, sizeSockAddr);
	listen(slisten, SOMAXCONN);

	printOnline();

	SOCKET new_connection;
	while (true) {
		char userName[32];
		new_connection = accept(slisten, (SOCKADDR*)&addr, &sizeSockAddr);
		if (recv(new_connection, userName, sizeof(userName), NULL) == SOCKET_ERROR) {
			closesocket(new_connection);
			continue;
		}
		addList(new_connection, userName);
	}

	return 0;
}