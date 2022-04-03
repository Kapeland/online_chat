#include "Windows.h"
#include "iostream"
#include "process.h"
#include <string>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

bool connectionToSever = true;

void Receive(void* param){
	int cntOfAttempts = 10; //  чтобы отследить отсоединение сервера

	while(true){
		// Клиент принимает данные с сервера
		SOCKET clientSocket = *(SOCKET*)(param);
		char recvbuf[2048] = {};        // Приемный буфер
		if(recv(clientSocket, recvbuf, 2048, 0) == SOCKET_ERROR){
			cntOfAttempts--;
			if(cntOfAttempts == 0){
				cout << "Server has been disconnected." << endl;
				connectionToSever = false;
				return;
			}
		}else{
			if(strcmp(recvbuf, "FRIEND LIST") == 0){
				char recvbuf1[2048] = {};
				recv(clientSocket, recvbuf1, 2048, 0);
				cout << "Your friend list: " << recvbuf1 << endl;
			}else{
				cout << recvbuf << endl;
			}
		}
	}
}

void Send(void* param){
	int cntOfAttempts = 3; //  чтобы отследить отсоединение сервера
	while(true){
		// Клиент отправляет данные на сервер
		SOCKET clientSocket = *(SOCKET*)(param);
		char sendbufC[2048] = {'\0'};        // Отправляем буфер
		cin.getline(sendbufC, 2048);
		string tmpStr = sendbufC;
		if(tmpStr.empty()){
			continue;
		}
		if(tmpStr == "---addFriend"){
			send(clientSocket, sendbufC, strlen(sendbufC), 0);
			string strFriend;
			cout << "Enter your friend's login and he will be added if his login exist." << endl;
			cin >> strFriend;
			send(clientSocket, strFriend.c_str(), strFriend.length(), 0);
			continue;
		}
		if(tmpStr == "---getFriendList"){
			send(clientSocket, sendbufC, strlen(sendbufC), 0);
			continue;
		}
		if(send(clientSocket, sendbufC, strlen(sendbufC), 0) == SOCKET_ERROR){
			if(cntOfAttempts == 0){
				cout << "Server has been disconnected." << endl;
				connectionToSever = false;
				return;
			}
			cntOfAttempts--;
			cout << "Probably server is not working.\n Please try again later" << endl;
		}else
			cout << "[I] said:" << sendbufC << endl;
	}
}

int main(){
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if(WSAStartup(sockVersion, &wsaData) != 0){
		cout << "Error in initialization of socket!" << endl; // TODO: change error message
	}

	SOCKADDR_IN SeverAddress; // Адрес сервера - это целевой адрес для подключения
	SeverAddress.sin_family = AF_INET;
	SeverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	SeverAddress.sin_port = htons(60000);// Устанавливаем номер порта

	//Создаём клиентский сокет
	SOCKET clientSocket;
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR){
		cout << "Error in creating socket!" << endl;
	}

	// Запускаем соединение
	if(connect(clientSocket, (PSOCKADDR)&SeverAddress, sizeof(SeverAddress)) == SOCKET_ERROR){
		cout << "Something went wrong during connecting to server!" << endl;
		exit(EXIT_FAILURE);
	}else{
		string sendBuf;
		cout << R"(Please type "LogIn" or "SignUp" to create a new account.)" << endl;
		cout << "Also, after authorization, you can type \"---getFriendList\" to get your list of friends." << endl;
		cout << "Or, after authorization, you can type \"---addFriend\" to add someone to your friend list." << endl;
		cin >> sendBuf;
		if(sendBuf != "LogIn" && sendBuf != "SignUp"){
			string tmpStr;
			while(tmpStr != "LogIn" && tmpStr != "SignUp"){
				cout << "Incorrect commands. Please try again." << endl;
				cin >> tmpStr;
			}
			sendBuf = tmpStr;
		}
		if(sendBuf == "LogIn"){
			string password, login;
			cout << "Login:";
			cin >> login;
			cout << "Password:";
			cin >> password;
			sendBuf = login + " " + password;
		}
		send(clientSocket,
			 sendBuf.c_str(),
			 sendBuf.length(),
			 0); //тут идёт запись инфы в сокет клиента, поскольку он отправляется серверу

		if(sendBuf == "SignUp"){
			string Login, Password;

			cout << "Please enter your new login and password." << endl;
			cout << "Login:";
			cin >> Login;
			cout << "Password:";
			cin >> Password;

			sendBuf = Login + " " + Password;
			send(clientSocket,
				 sendBuf.c_str(),
				 sendBuf.length(),
				 0); //тут идёт запись инфы в сокет клиента, поскольку он отправляется серверу
			char confirmationBuf[2048] = {};
			bool errorFlag = false;
			while(true){
				if(recv(clientSocket, confirmationBuf, 2048, 0) == INVALID_SOCKET){
					cout << "An error occurred during connecting to server.\nPlease try again later." << endl;
					connectionToSever = false;
					errorFlag = true;
					break;
				}
				string specialStr = confirmationBuf;
				if(specialStr == "Correct!"){
					break;
				}
				cout << confirmationBuf << " Please try again." << endl;
				cout << "Login:";
				cin >> Login;
				cout << "Password:";
				cin >> Password;
				sendBuf = Login + " " + Password;
				send(clientSocket,
					 sendBuf.c_str(),
					 sendBuf.length(),
					 0); //тут идёт запись инфы в сокет клиента, поскольку он отправляется серверу

			}
			if(!errorFlag){
				cout << "Welcome " << Login << '!' << endl;
				// Создаем два дочерних потока
				_beginthread(Send, 0, &clientSocket);
				_beginthread(Receive, 0, &clientSocket);
			}
		}else{
			string receivedStr;
			char receivedBuf[2048] = {};
			int cntOfAttempts = 5;

			recv(clientSocket, receivedBuf, 2048, 0);

			receivedStr = receivedBuf;
			if(receivedStr == "Correct!"){
				string Login = sendBuf.substr(0, sendBuf.find(' '));
				cout << "Welcome " << Login << '!' << endl;
				_beginthread(Receive, 0, &clientSocket);
				_beginthread(Send, 0, &clientSocket);
			}else{
				string Login, Password, sendStr;
				while(receivedStr != "Correct!"){
					if(cntOfAttempts == 0){
						cout << "Please try again later." << endl;
						exit(EXIT_SUCCESS);
					}
					cntOfAttempts--;
					memset(receivedBuf, '\0', sizeof(char) * 2048);

					cout << "Incorrect login or password.\nPlease try again." << endl;
					cin >> Login >> Password;

					sendStr = Login + " " + Password;

					send(clientSocket,
						 sendStr.c_str(),
						 sendStr.length(),
						 0); //тут идёт запись инфы в сокет клиента, поскольку он отправляется серверу
					recv(clientSocket, receivedBuf, 2048, 0);
					receivedStr = receivedBuf;
				}
				cout << "Welcome " << Login << '!' << endl;
				_beginthread(Receive, 0, &clientSocket);
				_beginthread(Send, 0, &clientSocket);
			}
		}
	}
	while(connectionToSever){
		Sleep(2000);
	}

	// Закрываем сокет
	if(clientSocket != INVALID_SOCKET){
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	WSACleanup();

	return 0;
}
