//Name: Rueben Tiow
//Last Modified Date: 6/3/2017
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

using namespace std;

const int MAX_MESSAGE_SIZE = 2000;
const int MAX_GUESSES = 2000;
const int IP_NUM = 1;
const int PORT_NUM = 2;


int main(int argc, char* argv[]){
	char* IP = argv[IP_NUM];
	unsigned short portNum = atoi(argv[PORT_NUM]);
	if(portNum < 12300 || portNum > 12399){
		cout << "Error: Invalid Port number" << endl;
		return 1;
	}
	int sendLength;
	int receiveLength;
	char buffer[MAX_MESSAGE_SIZE];
	char word[MAX_MESSAGE_SIZE];
	int numOfLetters;
	char* numOfL = (char*) &numOfLetters;
	unsigned int bytesSent;
	int atLoc;
	int curSize;
	char* sendSize;
	int receiveSize;
	
	//Create socket
	int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == -1){
		cout << "Error: Could not create socket" << endl;
		return 1;
	}
	
	//Convert address to int
	unsigned long destination;
	inet_pton(AF_INET, IP, (void*)&destination);
	
	//Fill socket information
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = destination;
	serverAddress.sin_port = htons(portNum);
	
	//Connect to server
	if(connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
		cout << "Error: Failed to connect to server" << endl;
		close(clientSocket);
		return 1;
	}
	cout << "Welcome to Hangman!" << endl;
	
	//Prompt user for name
	string userName;
	cout << "Please enter your name (no spaces):";
	cin >> userName;
	bool checkAlpha = false;
	while(!checkAlpha){
		checkAlpha = true;
		for(int i=0; i<(int)userName.length(); i++){
			if(!isalpha(userName[i])){
				cout << "Error: Name contains characters that are not letters, enter a valid new name:";
				cin >> userName;
				checkAlpha = false;
			}
		}
	}
	while(userName.length() >= MAX_MESSAGE_SIZE){
		cout << "Error: Name is too long, enter a valid new name:";
		cin >> userName;
	}
	
	//Send name to server
	strcpy(buffer, userName.c_str());
	bytesSent = sizeof(int);
	curSize = htonl(strlen(userName.c_str())+1);
	sendSize = (char*) &curSize;
	while(bytesSent > 0){
		sendLength = send(clientSocket, (void*) sendSize, bytesSent, 0);
		if(sendLength == -1){
			cout << "Error: Could not Send" << endl;
			close(clientSocket);
			return 1;
		}
		bytesSent -= sendLength;
		sendSize += sendLength;
	}
	bytesSent = ntohl(curSize);
	atLoc = 0;
	while (bytesSent > 0){
		sendLength = send(clientSocket, (void*) &buffer[atLoc], bytesSent, 0);
		if(sendLength == -1){
			cout << "Error: Could not Send" << endl;
			close(clientSocket);
			return 1;
		}
		bytesSent -= sendLength;
		atLoc += sendLength;
	}
	
	//Receive number of letters in word.
	receiveSize = 0;
	while(receiveSize < (int)sizeof(int)){
		receiveLength = recv(clientSocket, (void*) numOfL, sizeof(int)-receiveSize, 0);
		if(receiveLength <= 0){
			cout << "Error: Could not Receive" << endl;
			close(clientSocket);
			return 1;
		}
		receiveSize += receiveLength;
		numOfL += receiveLength;
	}
	int wordSize = ntohl(numOfLetters);
	for(int i=0; i<wordSize; i++){
		word[i] = '-';
	}
	
	//Make a guess.
	int numGuesses = 0;
	int trackGuessed = 0;
	string userGuess;
	int wordOrLetter;
	char* wOrL;
	char userOption;
	string displayCurWord = "";
	for(int i=0; i<wordSize; i++){
		displayCurWord += word[i];
	}
	cout << endl << "Turn " << numGuesses + 1 << endl;
	cout << "Word: " << displayCurWord << endl;
	while(numGuesses <= MAX_GUESSES && trackGuessed == 0){
		cout << "Guess the word (enter 1) or a letter (enter 2): ";
		cin >> userOption;
		int validOption = 0;
		while(validOption == 0){
			wordOrLetter = atoi(&userOption);
			if((wordOrLetter != 1) && (wordOrLetter != 2)){
				cout << "Erorr: Please enter a valid option, a word(enter 1) or a letter(enter 2): ";
				cin >> userOption;
			}else{
				validOption = 1;
			}
		}
		cout << "Enter your guess: ";
		cin >> userGuess;
		int validGuess;
		while(wordOrLetter == 2 && userGuess.length() > 1){
			cout << "Erorr: Please enter a valid guess: ";
			cin >> userGuess;
			validGuess = 0;
			while(validGuess == 0){
				for(int i=0; i<(int)strlen(userGuess.c_str()); i++){
					char checkAlpha = userGuess[i];
					if(!isalpha(checkAlpha)){
						cout << "Erorr: Please enter a valid guess: ";
						cin >> userGuess;
					}else{
						validGuess = 1;
					}
				}
			}
		}
		validGuess = 0;
		while(validGuess == 0){
			for(int i=0; i<(int)strlen(userGuess.c_str()); i++){
				char checkAlpha = userGuess[i];
				if(!isalpha(checkAlpha)){
					cout << "Erorr: Please enter a valid guess: ";
					cin >> userGuess;
				}else{
					validGuess = 1;
				}
			}
		}
		
		//Send the guess.
		bytesSent = sizeof(int);
		int wol = htonl(wordOrLetter);
		wOrL = (char*) &wol;
		while(bytesSent > 0){
			sendLength = send(clientSocket, (void*) wOrL, bytesSent, 0);
			if(sendLength == -1){
				cout << "Error: Could not Send" << endl;
				close(clientSocket);
				return 1;
			}
			bytesSent -= sendLength;
			wOrL += sendLength;
		}
		if(wordOrLetter == 2){
			char sendLetter = userGuess[0];
			sendLength = send(clientSocket, (void*) &sendLetter, sizeof(sendLetter), 0);
			if(sendLength == -1){
				cout << "Error: Could not Send" << endl;
				close(clientSocket);
				return 1;
			}
		}else{
			strcpy(buffer, userGuess.c_str());
			bytesSent = sizeof(int);
			curSize = htonl(strlen(userGuess.c_str())+1);
			sendSize = (char*) &curSize;
			while(bytesSent > 0){
				sendLength = send(clientSocket, (void*) sendSize, bytesSent, 0);
				if(sendLength == -1){
					cout << "Error: Could not Send" << endl;
					close(clientSocket);
					return 1;
				}
				bytesSent -= sendLength;
				sendSize += sendLength;
			}
			bytesSent = ntohl(curSize);
			atLoc = 0;
			while (bytesSent > 0){
				sendLength = send(clientSocket, (void*) &buffer[atLoc], bytesSent, 0);
				if(sendLength == -1){
					cout << "Error: Could not Send" << endl;
					close(clientSocket);
					return 1;
				}
				bytesSent -= sendLength;
				atLoc += sendLength;
			}
		}
		numGuesses++;
		
		//Receive response.
		int numOfI;
		char* iNumOfI = (char*) &numOfI;
		int index;
		char* iIndex = (char*) &index;
		int numOfTurns;
		char* iNumOfT = (char*) &numOfTurns;
		int responseType;
		char* iResponseT = (char*) &responseType;
		receiveSize = 0;
		while(receiveSize < (int)sizeof(int)){
			receiveLength = recv(clientSocket, (void*) iResponseT, sizeof(int)-receiveSize, 0);
			if(receiveLength <= 0){
				cout << "Error: Could not Receive" << endl;
				close(clientSocket);
				return 1;
			}
			receiveSize += receiveLength;
			iResponseT += receiveLength;
		}
		int response = ntohl(responseType);
		if(response == -1){
			cout << "Incorrect!" << endl << endl;
			string displayCurWord = "";
			for(int i=0; i<wordSize; i++){
				displayCurWord += word[i];
			}
			cout << "Turn " << numGuesses + 1 << endl;
			cout << "Word: "<< displayCurWord << endl;
		}else if(response == 2){
			receiveSize = 0;
			while(receiveSize < (int)sizeof(int)){
				receiveLength = recv(clientSocket, (void*) iNumOfI, sizeof(int)-receiveSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					close(clientSocket);
					return 1;
				}
				receiveSize += receiveLength;
				iNumOfI += receiveLength;
			}
			int curNumOfI = ntohl(numOfI);
			for(int i=0; i<curNumOfI; i++){
				int curResponse = 0;
				iIndex = (char*) &index;
				while(curResponse < (int)sizeof(int)){//receive loop
					receiveLength = recv(clientSocket, (void*) iIndex, sizeof(int)-curResponse, 0);
					if(receiveLength <= 0){
						cout << "Error: Could not Receive" << endl;
						close(clientSocket);
						return 1;
					}
					curResponse += receiveLength;
					iIndex += receiveLength;
				}
				int ind = ntohl(index);
				if(ind > wordSize){
					cout << "Error: Bad index bytes sent to server" << endl;
					return 1;
				}
				word[ind] = userGuess[0];
			}
			cout << "Correct!" << endl << endl;
			string displayCurWord = "";
			for(int i=0; i<wordSize; i++){
				displayCurWord += toupper(word[i]);
			}
			cout << "Turn " << numGuesses + 1 << endl;
			cout << "Word: " << displayCurWord << endl;
		}else if(response == 1){
			trackGuessed = 1;
			receiveSize = 0;
			while(receiveSize < (int)sizeof(int)){
				receiveLength = recv(clientSocket, (void*) iNumOfT, sizeof(int)-receiveSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					close(clientSocket);
					return 1;
				}
				receiveSize += receiveLength;
				iNumOfT += receiveLength;
			}
			int turns = ntohl(numOfTurns);
			string displayGuess;
			for(int i=0; i < (int)userGuess.length();i++){
				displayGuess += toupper(userGuess[i]);
			}
			cout << "Correct!" << endl << endl;
			cout << "Congratulations! You guessed the word " << displayGuess << "!!!" << endl;
			cout << "It took " << turns << " turns to guess the word correctly." << endl;
			
			//Receive victory message
			int numOfPlayers = 0;
			int numOP;
			char* iNumOP = (char*) &numOP;
			receiveSize = 0;
			while(receiveSize < (int)sizeof(int)){
				receiveLength = recv(clientSocket, (void*) iNumOP, sizeof(int)-receiveSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					close(clientSocket);
					return 1;
				}
				receiveSize += receiveLength;
				iNumOP += receiveLength;
			}
			numOfPlayers = ntohl(numOP);
			int sizeOfBoard;
			char* iSizeOfBoard = (char*) &sizeOfBoard;
			receiveSize = 0;
			while(receiveSize < (int)sizeof(int)){
				receiveLength = recv(clientSocket, (void*) iSizeOfBoard, sizeof(int)-receiveSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					close(clientSocket);
					return 1;
				}
				receiveSize += receiveLength;
				iSizeOfBoard += receiveLength;
			}
			int boardSize = ntohl(sizeOfBoard);
			int counter = 0;
			receiveSize = 0;
			while(receiveSize < boardSize){
				receiveLength = recv(clientSocket, (void*) &buffer[counter], boardSize-receiveSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					close(clientSocket);
					return 1;
				}
				receiveSize += receiveLength;
				counter += receiveLength;
			}
			string displayLeaderBoard = "";
			int track=0;
			for(int i=0; i<numOfPlayers; i++){
				displayLeaderBoard += to_string(i+1) + ": ";
				while(isalpha(buffer[track])){
					displayLeaderBoard += buffer[track];
					track++;
				}
				displayLeaderBoard += " Turns: ";
				while(isdigit(buffer[track])){
					displayLeaderBoard += buffer[track];
					track++;
				}
				displayLeaderBoard += '\n';
				if(track == boardSize){
					break;
				}
			}
			cout << endl << "LeaderBoard:" << endl << displayLeaderBoard << endl;
		}else{
			cout << "Error: Could not Receive" << endl;
			close(clientSocket);
			return 1;
		}
	}
	close(clientSocket);
	return 0;
}
