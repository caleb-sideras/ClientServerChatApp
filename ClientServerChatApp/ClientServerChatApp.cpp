#define _CRT_SECURE_NO_WARNINGS                 // turns of deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS         // turns of deprecated warnings for winsock
#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
#include <thread>
#include <fstream>
#include <ios>

// decouple in header files later
void ServerCode(void);
void ClientCode(void);
int tcp_recv_whole(SOCKET s, char* buf, int len);
int tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length);
std::wstring getWideStringFromString(std::string string);
std::string getStringFromWideString(std::wstring wstring);
void receiveClient(SOCKET s);

int main()
{

	WSADATA wsadata;
	int iResult = WSAStartup(WINSOCK_VERSION, &wsadata);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	int choice;
	do
	{
		printf("Would you like to Create a Server or Client?\n");
		printf("1> Server\n");
		printf("2> Client\n");
		std::cin >> choice;
	} while (choice != 1 && choice != 2);

	//Server
	if (choice == 1)
	{
		ServerCode();
	}

	//Client
	if (choice == 2)
	{
		ClientCode();
	}

	return WSACleanup();
}


void ServerCode(void)
{
	const uint8_t MAX_CLIENTS = 3;

	std::unordered_map < SOCKET, std::string > clients;
	FILE* pFile;
	pFile = fopen("serverlog.txt", "w");

	SOCKET broadcastSock;
	broadcastSock = socket(AF_INET, SOCK_DGRAM, 0);
	char broadcast = '1';


	if (setsockopt(broadcastSock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) < 0) {
		closesocket(broadcast);
		printf("DEBUG// Error in setting Broadcast option\n");
		return;
	}

	char sendMSG[] = "192.168.1.64:31337";
	char recvbuff[50] = "";
	int recvbufflen = 50;

	struct sockaddr_in Recv_addr;
	Recv_addr.sin_family = AF_INET;
	Recv_addr.sin_port = htons(9009);
	//Recv_addr.sin_addr.s_addr = INADDR_ANY;
	Recv_addr.sin_addr.S_un.S_addr = INADDR_BROADCAST;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("DEBUG// Socket function incorrect\n");
		return;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(31337);

	int result = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		printf("DEBUG// Bind function incorrect\n");
		return;
	}

	result = listen(listenSocket, 1);
	if (result == SOCKET_ERROR)
	{
		printf("DEBUG// Listen function incorrect\n");
		return;
	}


	fd_set masterSet;
	FD_ZERO(&masterSet);
	FD_SET(listenSocket, &masterSet);
	printf("Waiting...\n");


	timeval timeout;
	timeout.tv_sec = 1;

	while (true) {

		fd_set readySet = masterSet;

		int readyFD = select(0, &readySet, NULL, NULL, &timeout);

		sendto(broadcastSock, sendMSG, strlen(sendMSG) + 1, 0, (sockaddr*)&Recv_addr, sizeof(Recv_addr));

		if (FD_ISSET(listenSocket, &readySet)) {

			SOCKET ComSocket = accept(listenSocket, NULL, NULL);
			if (ComSocket == INVALID_SOCKET)
			{
				printf("DEBUG// Accept function incorrect\n");
				closesocket(ComSocket);
				WSACleanup();
				return;

			}
			else
			{
				FD_SET(ComSocket, &masterSet);
			}
		}

		for (int i = 0; i < readyFD; i++) //(int)clients.size()
		{
			if (FD_ISSET(readySet.fd_array[i], &readySet))
			{

				uint8_t size = 0;
				char* buffer = new char[size];

				result = tcp_recv_whole(readySet.fd_array[i], (char*)&size, 1);
				if ((result == SOCKET_ERROR) || (result == 0))
				{
					//delete[] buffer;

					// NOT ENTIRELY SURE! 

					//int error = WSAGetLastError();
					//printf("DEBUG// recv is incorrect\n");


					//// read something if message length ==0 (so basically nothing) then set mesg length to 2?
					//char* buffer = new char[2];
					//result = tcp_recv_whole(readySet.fd_array[i], (char*)buffer, 2);
					//if ((result == SOCKET_ERROR) || (result == 0))
					//{
					//	int error = WSAGetLastError();
					//	printf("DEBUG// recv is incorrect\n");
					//	return;

					//}
					//else if (result == 2)
					//{
					//	msgLen[i] = 2; //?
					//	printf("DEBUG// I used the recv function\n");
					//}


				}
				else
				{

					result = tcp_recv_whole(readySet.fd_array[i], (char*)buffer, size);
					if ((result == SOCKET_ERROR) || (result == 0))
					{
						//delete[] buffer;
						int error = WSAGetLastError();
						printf("DEBUG// recv is incorrect\n");
						return;

					}
					else
					{
						// Checking for register
						std::string multiByteMsg = std::string(buffer).c_str();
						std::wstring msg = getWideStringFromString(multiByteMsg);
						std::wstring identifier = msg.substr(0, msg.find(L":"));
						std::wstring username = msg.substr(msg.find(L":") + 1, (int)size);
						std::string usernamestr = getStringFromWideString(username);

						//string input
						std::string message(buffer, size);
						//delete[] buffer;

						

						if (identifier == L"$register") {
							char server_log_msg[30];
							strcpy(server_log_msg, message.c_str());

							FILE* fp;
							fp = fopen("serverlog.txt", "a");
							if (fp)
							{
								for (int i = 0; i < strlen(server_log_msg); i++)
									putc(message[i], fp);
							}
							fclose(fp);
						}
						else {
							std::unordered_map<SOCKET, std::string>::const_iterator got = clients.find(readySet.fd_array[i]);
							if (got == clients.end())
								std::cout << "not found";
							else {


								//std::string serverRegister = got->second + ":" + message;
								char server_log_msg[30];
								strcpy(server_log_msg, message.c_str());

								FILE* fp;
								fp = fopen("serverlog.txt", "a");
								if (fp)
								{
									putc('\n', fp);
									for (int i = 0; i < strlen(server_log_msg); i++)
										putc(message[i], fp);
								}
								fclose(fp);

								//printf(serverRegister.c_str());
								std::cout<< got->second << ":" << message << std::endl;
								printf("\n");
							}	
						}
						;

						if (identifier == L"$register") {

							if (readySet.fd_count >= MAX_CLIENTS)
							{
								size = 8;
								char sendbuffer[30];
								memset(sendbuffer, 0, 30);
								strcpy(sendbuffer, "SV_FULL");

								result = tcp_send_whole(readySet.fd_array[i], (char*)&size, 1);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}

								result = tcp_send_whole(readySet.fd_array[i], sendbuffer, size);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}

								FD_CLR(readySet.fd_array[i], &masterSet);
							}
							else
							{
								printf("Client has joined the server!\n");

								clients.insert(std::pair<SOCKET, std::string>(readySet.fd_array[i], usernamestr));
								std::wcout << L"Added user: " << username << std::endl;

								size = 10;
								char sendbuffer[30];
								memset(sendbuffer, 0, 30);
								strcpy(sendbuffer, "SV_SUCCESS");

								result = tcp_send_whole(readySet.fd_array[i], (char*)&size, 1);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}

								result = tcp_send_whole(readySet.fd_array[i], sendbuffer, size);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}

							}


						}
						else if (message == "$getlist") {

							printf("GetList Called");
							printf("\n");

							std::string list;
							for (auto& it : clients) {

								list.append(it.second);

							}

							printf(list.c_str());
							printf("\n");

							uint8_t msg_size = list.length();
							char sendbuffer[30];
							memset(sendbuffer, 0, 30);
							strcpy(sendbuffer, list.c_str());

							result = tcp_send_whole(readySet.fd_array[i], (char*)&msg_size, 1);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							result = tcp_send_whole(readySet.fd_array[i], sendbuffer, msg_size);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

						}
						else if (message == "$getlog") {
							std::cout << "GetLog Called" << std::endl;

							size = 7;
							char sendbuffer[30];
							memset(sendbuffer, 0, 30);
							strcpy(sendbuffer, "$getlog");

							result = tcp_send_whole(readySet.fd_array[i], (char*)&size, 1);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							result = tcp_send_whole(readySet.fd_array[i], sendbuffer, size);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							char buf[30];
							std::ifstream file("serverlog.txt");

							file.seekg(0, std::ios::end);
							int tempsize = file.tellg();
							file.seekg(0, std::ios::beg);
							std::cout << tempsize << std::endl;

							result = tcp_send_whole(readySet.fd_array[i], (char*)&tempsize, 1);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							while (file.getline(buf, 30))
							{
								std::cout << buf << "\n";
								int bufflen = strlen(buf) + 1;

								result = tcp_send_whole(readySet.fd_array[i], (char*)&bufflen, 1);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}

								result = tcp_send_whole(readySet.fd_array[i], buf, bufflen);
								if ((result == SOCKET_ERROR) || (result == 0))
								{
									int error = WSAGetLastError();
									printf("DEBUG// send is incorrect\n");
									return;

								}
								//delete[] buffer;
							}

							file.close();
							
						}
						else if (message == "$exit") {

							size = 5;
							char sendbuffer[30];
							memset(sendbuffer, 0, 30);
							strcpy(sendbuffer, "$exit");

							result = tcp_send_whole(readySet.fd_array[i], (char*)&size, 1);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							result = tcp_send_whole(readySet.fd_array[i], sendbuffer, size);
							if ((result == SOCKET_ERROR) || (result == 0))
							{
								int error = WSAGetLastError();
								printf("DEBUG// send is incorrect\n");
								return;

							}

							std::cout << "Exit Called" << std::endl;
							FD_CLR(readySet.fd_array[i], &masterSet);
							clients.erase(readySet.fd_array[i]);
							closesocket(readySet.fd_array[i]);

							

						}
						else {

							uint8_t size = message.length() + 1;
							char sendbuffer[30];
							memset(sendbuffer, 0, 30);
							strcpy(sendbuffer, message.c_str());

							for (auto& it : clients) {

								if (it.first != readySet.fd_array[i])
								{

									SOCKET sockFd = (SOCKET)it.first;

									result = tcp_send_whole(sockFd, (char*)&size, 1);
									if ((result == SOCKET_ERROR) || (result == 0))
									{
										int error = WSAGetLastError();
										printf("DEBUG// send is incorrect FIRST\n");
										return;

									}

									result = tcp_send_whole(sockFd, sendbuffer, size);
									if ((result == SOCKET_ERROR) || (result == 0))
									{
										int error = WSAGetLastError();
										printf("DEBUG// send is incorrect SECOND\n");
										return;

									}
								}
							}
						}
					}
				}
			}
			else {
				//close 
			}
		}




	}
}

void ClientCode(void)
{
	SOCKET udpIn;
	udpIn = socket(AF_INET, SOCK_DGRAM, 0);

	char broadcast = '1';
	setsockopt(udpIn, SOL_SOCKET, SO_REUSEADDR, (char*)&broadcast, sizeof(broadcast));

	if (setsockopt(udpIn, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		closesocket(udpIn);
		printf("DEBUG// Error in setting Broadcast option\n");
		return;
	}

	struct sockaddr_in Recv_addr;
	struct sockaddr_in Sender_addr;

	int len = sizeof(struct sockaddr_in);

	char recvbuff[50];
	int recvbufflen = 50;

	Recv_addr.sin_family = AF_INET;
	Recv_addr.sin_port = htons(9009);
	Recv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(udpIn, (sockaddr*)&Recv_addr, sizeof(Recv_addr)) < 0)
	{
		closesocket(udpIn);
		printf("DEBUG// Error in binding Broadcast option\n");
		return;

	}

	recvfrom(udpIn, recvbuff, recvbufflen, 0, (sockaddr*)&Sender_addr, &len);

	std::string multiByteMsg = std::string(recvbuff).c_str();
	std::wstring msg = getWideStringFromString(multiByteMsg);
	std::wstring wip = msg.substr(0, msg.find(L":"));
	std::wstring wport = msg.substr(msg.find(L":") + 1, strlen(recvbuff));
	std::string ip2 = getStringFromWideString(wip);
	std::string port2 = getStringFromWideString(wport);

	

	const char* address = ip2.c_str();
	int port = std::stoi(port2);

	std::cout << address << std::endl;
	std::cout << port << std::endl;

	//Socket
	SOCKET ComSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ComSocket == INVALID_SOCKET)
	{
		printf("DEBUG// Socket function incorrect\n");
		return;
	}

	//Connect
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(address);
	serverAddr.sin_port = htons(port);

	int result = connect(ComSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		printf("DEBUG// Connect function incorrect\n");
		return;
	}

	//possible issue with no null terminator
	std::string username;
	printf("Please enter username\n");
	std::cin >> username;

	std::string serverRegister = "$register:" + username;

	//printf("Command: %d \n", msgToSend);

	//Communication
	uint8_t size = strlen(serverRegister.c_str()) + 1;
	char sendbuffer[30];
	memset(sendbuffer, 0, 30);
	strcpy(sendbuffer, serverRegister.c_str());

	result = tcp_send_whole(ComSocket, (char*)&size, 1);
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		printf("DEBUG// send is incorrect\n");
		return;

	}

	result = tcp_send_whole(ComSocket, sendbuffer, size);
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		printf("DEBUG// send is incorrect\n");
		return;

	}

	while (true)
	{

		//Communication
		uint8_t size = 0;

		result = tcp_recv_whole(ComSocket, (char*)&size, 1);
		if ((result == SOCKET_ERROR) || (result == 0))
		{
			int error = WSAGetLastError();
			printf("DEBUG// recv is incorrect\n");
			return;

		}

		char* recvbuffer = new char[size];

		result = tcp_recv_whole(ComSocket, (char*)recvbuffer, size);
		if ((result == SOCKET_ERROR) || (result == 0))
		{
			int error = WSAGetLastError();
			printf("DEBUG// recv is incorrect\n");
			return;

		}

		std::string server_message(recvbuffer, size);
		delete[] recvbuffer;

		printf(server_message.c_str());
		printf("\n");

		if (server_message == "SV_SUCCESS")
		{
			std::thread th1(receiveClient, ComSocket);
			while (true)
			{

				//char message[30];
				std::string message;
				std::cout << username << ":";
				std::cin >> message;

				uint8_t msg_size = strlen(message.c_str());
				char sendbuffer[30];
				memset(sendbuffer, 0, 30);
				strcpy(sendbuffer, message.c_str());

				result = tcp_send_whole(ComSocket, (char*)&msg_size, 1);
				if ((result == SOCKET_ERROR) || (result == 0))
				{
					int error = WSAGetLastError();
					printf("DEBUG// send is incorrect\n");
					return;

				}

				result = tcp_send_whole(ComSocket, sendbuffer, msg_size);
				if ((result == SOCKET_ERROR) || (result == 0))
				{
					int error = WSAGetLastError();
					printf("DEBUG// send is incorrect\n");
					return;

				}

			}

		}
		else if (server_message == "SV_FULL")
		{
			shutdown(ComSocket, SD_BOTH);
		}
	}


}


int tcp_recv_whole(SOCKET s, char* buf, int len)
{
	int total = 0;

	do
	{
		int ret = recv(s, buf + total, len - total, 0);
		if (ret < 1)
			return ret;
		else
			total += ret;

	} while (total < len);

	return total;
}

int tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length)
{
	int result;
	int bytesSent = 0;

	while (bytesSent < length)
	{
		result = send(skSocket, (const char*)data + bytesSent, length - bytesSent, 0);

		if (result <= 0)
			return result;

		bytesSent += result;
	}

	return bytesSent;
}

std::wstring getWideStringFromString(std::string string) {
	int sz = MultiByteToWideChar(CP_UTF8, 0, &string[0], -1, NULL, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(CP_UTF8, 0, &string[0], -1, &res[0], sz);
	return res;
}

std::string getStringFromWideString(std::wstring wstring) {
	int sz = WideCharToMultiByte(CP_UTF8, 0, &wstring[0], -1, 0, 0, 0, 0);
	std::string res(sz, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstring[0], -1, &res[0], sz, 0, 0);
	return res;
}

void receiveClient(SOCKET s)
{

	while (true)
	{
		uint8_t size = 0;
		char* buffer = new char[size];

		int result = tcp_recv_whole(s, (char*)&size, 1);
		if ((result == SOCKET_ERROR) || (result == 0) || result <0)
		{
			std::terminate();
			int error = WSAGetLastError();
			printf("DEBUG// recv is incorrect\n");
		}
		else {

		}
		result = tcp_recv_whole(s, buffer, size);
		if ((result == SOCKET_ERROR) || (result == 0))
		{
			std::terminate();
			int error = WSAGetLastError();
			printf("DEBUG// recv is incorrect\n");
		}
		else 
		{
			std::string message(buffer, size);
			if (message=="$exit")
			{
				closesocket(s);
				shutdown(s, SD_BOTH);
				return;
				/*std::terminate();*/
			}
			else if (message == "$getlog") 
			{
				int filesize = 0;
				char* sizebuffer = new char[size];

				int result = tcp_recv_whole(s, (char*)&filesize, 1);
				if ((result == SOCKET_ERROR) || (result == 0) || result < 0)
				{
					std::terminate();
					int error = WSAGetLastError();
					printf("DEBUG// recv is incorrect\n");
				}

				int sizeiter = 0;
				FILE* init;
				init = fopen("serverrecvlog.txt", "w");

				FILE* fp;
				fp = fopen("serverrecvlog.txt", "a");
				while (sizeiter <= filesize)
				{
					int linesize = 0;
					char* linebuffer = new char[size];
					

					int result = tcp_recv_whole(s, (char*)&linesize, 1);
					if ((result == SOCKET_ERROR) || (result == 0) || result < 0)
					{
						std::terminate();
						int error = WSAGetLastError();
						printf("DEBUG// recv is incorrect\n");
					}
					else {

					}
					result = tcp_recv_whole(s, linebuffer, linesize);
					if ((result == SOCKET_ERROR) || (result == 0))
					{
						std::terminate();
						int error = WSAGetLastError();
						printf("DEBUG// recv is incorrect\n");
					}

					char c2[30];
					strcpy(c2, linebuffer);

					if (fp)
					{
						putc('\n', fp);
						for (int i = 0; i < strlen(c2); i++)
							putc(c2[i], fp);
					}
					

					sizeiter = sizeiter + linesize + 1;
				}
				fclose(fp);
			}
			else {
				printf(buffer);
				printf("\n");
			}
			
		}
	}
	


}
