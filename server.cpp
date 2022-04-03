#include <iostream>
#include "Windows.h"
#include "process.h"
#include <map>
#include <vector>
#include <fstream>
#include <string>

using namespace std;
#pragma comment(lib, "WS2_32.lib")  // Отображение загрузки ws2_32.dll ws2_32.dll - последняя версия сокета

#define MAXVALUEOFUSERS 1000 // at current moment
int cntOfUsers = 0; // количество пользователей в данный момент

map <string, SOCKET> dataBaseCurrent;
map <string, string> dataBase;
map <string, vector <string>> dataBaseFriends;

SOCKET connections[MAXVALUEOFUSERS];

void writeToFile(){
	ofstream fout;
	fout.open("dataBase.txt", ios::out);
	for(const auto&item: dataBase){
		fout << item.first << " " << item.second << endl;
	}
	fout.close();
}

void writeToFileFriends(){
	ofstream fout;
	fout.open("dataBaseFriends.txt", ios::out);
	for(const auto&item: dataBaseFriends){
		fout << item.first << endl;
		for(const auto&Friend: item.second){
			fout << Friend << endl;
		}
	}
	fout << "*****" << endl;
	fout.close();
}

void readFromFile(){
	dataBase.clear();
	ifstream fin;
	fin.open("dataBase.txt", ios::in);
	string login, password;
	while(fin >> login >> password){
		dataBase[login] = password;
	}
	fin.close();
}

void readFromFileFriends(){
	dataBaseFriends.clear();
	ifstream fin;
	fin.open("dataBaseFriends.txt", ios::in);

	string login, password, str;
	while(fin >> str){
		if(!str.empty()){
			string newStr;
			while(true){
				fin >> newStr;
				if(newStr == "*****"){
					break;
				}
				dataBaseFriends[str].push_back(newStr);
			}
		}
	}
	fin.close();
}


int checkLogin(const string&login, const string&password){ // 1 - найден, 0 - нет
	for(const auto&item: dataBase){
		if(item.first == login && item.second == password){
			return 1;
		}
	}
	return 0;
}

void sendEveryoneC(const int i, const string&login){ // сообщить всем, что пользователь присоединился
	const string&friendNameStr = login;
	bool specialMsgCreated = false;

	for(int j = 0; j < MAXVALUEOFUSERS; j++){
		if(j == i){
			continue;
		}else{
			if(connections[j] != SOCKET_ERROR){
				string usersLogin;
				for(const auto&item: dataBaseCurrent){
					if(item.second == connections[j]){
						usersLogin = item.first;
						break;
					}
				}
				for(const auto&Friend: dataBaseFriends){
					if(Friend.first == usersLogin){
						for(const auto&Name: Friend.second){
							if(Name == friendNameStr){
								string specialMsg = "Your friend -->" + login + " joined to chat!";
								specialMsgCreated = true;
								send(connections[j], specialMsg.c_str(), specialMsg.length(), 0);
								break;
							}
						}
					}
					if(specialMsgCreated){
						break;
					}
				}
				if(specialMsgCreated){
					continue;
				}

				string sendBuf = login + " joined to chat!";
				send(connections[j], sendBuf.c_str(), sendBuf.length(), 0);
			}else{
				continue;
			}
		}

	}
}

void sendEveryoneMsg(const int i,
					 const char* msg){ // Сообщить всем пользователям сообщение от другого пользователя кроме отправителя
	string friendNameStr;
	bool specialMsgCreated = false;

	for(const auto&item: dataBaseCurrent){
		if(item.second == connections[i]){
			friendNameStr = item.first;
		}
	}


	for(int j = 0; j < MAXVALUEOFUSERS; j++){
		if(j == i){
			continue;
		}else{
			if(connections[j] != SOCKET_ERROR){
				string usersLogin;
				for(const auto&item: dataBaseCurrent){
					if(item.second == connections[j]){
						usersLogin = item.first;
						break;
					}
				}
				for(const auto&Friend: dataBaseFriends){
					if(Friend.first == usersLogin){
						for(const auto&Name: Friend.second){
							if(Name == friendNameStr){
								string tmpstr = "Your friend -->";
								string specialMsg = tmpstr + msg;
								specialMsgCreated = true;
								send(connections[j], specialMsg.c_str(), specialMsg.length(), 0);
								break;
							}
						}
					}
					if(specialMsgCreated){
						break;
					}
				}
				if(specialMsgCreated){
					continue;
				}
				send(connections[j], msg, strlen(msg), 0);
			}else{
				continue;
			}
		}
	}
}

void __RPC_CALLEE receive(int index){      // Функция потока, принимающая данные
	int cntOfAttempts = 10; //  чтобы отследить отсоединение клиента
	while(true){
		// Сервер принимает данные
		char recvbuf[2048] = {};        // Приемный буфер
		if(recv(connections[index], recvbuf, 2048, 0) == SOCKET_ERROR){
			cntOfAttempts--;
			if(cntOfAttempts == 0){
				for(const auto&item: dataBaseCurrent){
					if(item.second == connections[index]){
						string message = item.first + " is not connected anymore.";
						cout << message << endl;
						sendEveryoneMsg(index, message.c_str());
						dataBaseCurrent.erase(item.first);
						closesocket(connections[index]);
						connections[index] = INVALID_SOCKET;
						return;
					}
				}
			}
		}else{
			if(strcmp(recvbuf, "---addFriend") == 0){
				char recvbuFF[2048] = {};
				recv(connections[index], recvbuFF, 2048, 0);
				string receivedFriendLogin = recvbuFF;
				for(const auto&item: dataBaseCurrent){
					if(item.second == connections[index]){
						dataBaseFriends[item.first].push_back(receivedFriendLogin);
						break;
					}
				}
				writeToFileFriends();
				continue;
			}
			if(strcmp(recvbuf, "---getFriendList") == 0){
				string userLogin;
				bool specialMsgCreated = false;
				for(auto item: dataBaseCurrent){
					if(item.second == connections[index]){
						userLogin = item.first;
						break;
					}
				}
				for(const auto&item: dataBaseFriends){
					if(item.first == userLogin){
						string tmpStr;
						for(const auto&name: item.second){
							tmpStr += (name + " ");
						}
						string tmpStr2 = "FRIEND LIST";
						send(connections[index], tmpStr2.c_str(), tmpStr2.length(), 0);
						send(connections[index], tmpStr.c_str(), tmpStr.length(), 0);
						specialMsgCreated = true;
						break;
					}
				}
				if(!specialMsgCreated){
					string tmp;
					send(connections[index], tmp.c_str(), tmp.length(), 0);
				}
				continue;
			}
			for(const auto&item: dataBaseCurrent){
				if(item.second == connections[index]){
					cout << "[" << item.first << "] said:" << recvbuf << endl;
					string message = "[" + item.first + "] said:" + recvbuf;
					sendEveryoneMsg(index, message.c_str());
					break;
				}
			}
		}
	}
}

int main(){
	cout << "----------- server -----------" << endl;

	// Инициализация сокета
	WSADATA wsaData;    // Эта структура используется для хранения данных Windows Sockets, возвращаемых после вызова функции WSAStartup.
	WORD sockVersion = MAKEWORD(2, 2);    // Информация о номере версии библиотеки сетевого программирования Windows
	if(WSAStartup(sockVersion,
				  &wsaData) != 0) // Функция WSAStartup предназначена для инициализации и загрузки сети Windows в программе
	{
		cout << "Error of initialization the program" << endl;
		exit(EXIT_FAILURE);
	}

	SOCKADDR_IN severAddress;  // Адрес сервера: есть IP-адрес, номер порта и семейство протоколов
	severAddress.sin_family = AF_INET; //тут хранится тип протокола
	severAddress.sin_addr.s_addr = inet_addr("127.0.0.1");// Заполняем адрес сервера
	severAddress.sin_port = htons(60000);// Устанавливаем номер порта

	// Создаем серверный сокет
	SOCKET severSocket;
	if((severSocket = socket(AF_INET,
							 SOCK_STREAM,
							 0)) == SOCKET_ERROR){ //второй параметр нужен для того, чтобы убедиться в доставке + блокировка приёма других данных
		cout << "Not successful creation of server's socket" << endl;
		exit(EXIT_FAILURE);
	}

	// Привязываем к сокету адрес, чтобы этот сокет стал доступен клиентам
	if(bind(severSocket, (PSOCKADDR)&severAddress, sizeof(severAddress)) == SOCKET_ERROR){
		cout << "Not successful attaching of socket!" << endl;
		exit(EXIT_FAILURE);
	}

	readFromFile();
	readFromFileFriends(); // чтобы к началу листенинга было понятно, что всё работает

	// Мониторинг сервера
	if(listen(severSocket,
			  SOMAXCONN) == SOCKET_ERROR) // Второй параметр мониторинга: сколько клиентских запросов может обрабатываться одновременно.
	{
		cout << "Crash of listening!" << endl;
		return 0;
	}else
		cout << "Server is listening:" << endl;

// Сервер принимает запрос на подключение
	SOCKADDR_IN revClientAddress;    // Адрес и порт сокета

	int addrlen = sizeof(revClientAddress);


	for(int i = 0; i < MAXVALUEOFUSERS; i++){

		connections[i] = accept(severSocket, (PSOCKADDR)&revClientAddress, &addrlen);

		if(connections[i] != INVALID_SOCKET){
			// Ф-ция accept возвращает сокет клиента (2-3 параметры присваиваются в момент соединения)
			char receivedBuf[2048] = "";

			// Читает данные из сокета. Флаг 0 говорит о том, что данные удаляются из сокета
			if(recv(connections[i], receivedBuf, 2048, 0) == INVALID_SOCKET){
				continue;
			}


			string receivedStr = receivedBuf;

			if(receivedStr == "SignUp"){ // Создание нового пользователя
				char receivedBuf1[2048] = "";

				recv(connections[i], receivedBuf1, 2048, 0);

				string receivedStr1 = receivedBuf1;
				string Login = receivedStr1.substr(0, receivedStr1.find(' '));
				string Password = receivedStr1.substr(Login.length() + 1);

				const char errorMsg[2048] = "Your login is already exist.";
				const char specialConfirmationMsg[2048] = "Correct!";
				char receivedBuf2[2048] = {};

				bool errorFlag = false;

				while(dataBase.count(Login) > 0){
					send(connections[i], errorMsg, strlen(errorMsg), 0);
					if(recv(connections[i], receivedBuf2, 2048, 0) == INVALID_SOCKET){
						connections[i] = INVALID_SOCKET;
						closesocket(connections[i]);
						errorFlag = true;
						break;
					}
					receivedStr1 = receivedBuf2;
					Login = receivedStr1.substr(0, receivedStr1.find(' '));
					Password = receivedStr1.substr(Login.length() + 1);
				}
				if(errorFlag){
					continue;
				}
				send(connections[i], specialConfirmationMsg, strlen(errorMsg), 0);
				dataBase[Login] = Password;
				writeToFile();
				dataBaseCurrent[Login] = connections[i];
				cntOfUsers++;
				cout << Login << " joined to chat!" << endl;

				sendEveryoneC(i, Login);
				CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)receive, (LPVOID)i, NULL, nullptr);
			}else{ // Уже получили на вход логин/пароль
				string Login = receivedStr.substr(0, receivedStr.find(' '));
				string Password = receivedStr.substr(Login.length() + 1);
				string confirmationMsg = "Correct!";

				if(checkLogin(Login, Password)){

					send(connections[i], confirmationMsg.c_str(), confirmationMsg.length(), 0);

					dataBaseCurrent[Login] = connections[i];
					cntOfUsers++;
					sendEveryoneC(i, Login);
					cout << Login << " joined to chat!" << endl;

					CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)receive, (LPVOID)i, NULL, nullptr);
				}else{
					int cntOfAttempts = 5;
					bool flagSuccess = false; // Было найдено совпадение
					bool flagError = false; // Пользователь отказался вводить данные

					char sendErrorMsg[2048] = "Incorrect login or password!";   // Отправляем буфер

					while(cntOfAttempts > 0){ // Получение логина/пароля до тех пор, пока не будет совпадения
						memset(receivedBuf, '\0', sizeof(char) * 2048);

						send(connections[i], sendErrorMsg, strlen(sendErrorMsg), 0);
						if(recv(connections[i], receivedBuf, 2048, 0) == SOCKET_ERROR){
							flagError = true;
							break;
						}

						receivedStr = receivedBuf;
						Login = receivedStr.substr(0, receivedStr.find(' '));
						Password = receivedStr.substr(Login.length() + 1);

						if(checkLogin(Login, Password)){
							flagSuccess = true;
							break;
						}else{
							cntOfAttempts--;
						}
					}
					if(flagError){
						connections[i] = INVALID_SOCKET;
						closesocket(connections[i]);
						continue;
					}
					if(flagSuccess){
						send(connections[i], confirmationMsg.c_str(), strlen(sendErrorMsg), 0);

						dataBaseCurrent[Login] = connections[i];
						cntOfUsers++;
						cout << Login << " joined to chat!" << endl;

						sendEveryoneC(i, Login);
						CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)receive, (LPVOID)i, NULL, nullptr);
					}else{
						send(connections[i], sendErrorMsg, strlen(sendErrorMsg), 0);
					}
				}
			}
		}else{
			continue;
		}

	}

	const char specialMsg[2048] = "The server is overloaded. Please try again later.";
	sendEveryoneMsg(-1, specialMsg); // Всем пользователям отправка

	// Закрываем сокет
	for(const auto&item: dataBaseCurrent){
		if(item.second != SOCKET_ERROR){
			closesocket(item.second);
		}
	}
	closesocket(severSocket);
	dataBaseCurrent.clear();
	dataBase.clear();
	dataBaseFriends.clear();

	// завершение
	WSACleanup();
	cout << "Сервер остановлен!" << endl;
	return 0;
}