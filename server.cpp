//Name: Rueben Tiow
//Last Modified Date: 6/3/2017
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <cstring>
#include <unistd.h>

using namespace std;

const int PORT_NUM = 1;
const int NUMOFWORDS = 57488;
const int MAX_PENDING = 10;
const int MAX_MESSAGE_SIZE = 1100;
const int MAX_GUESSES = 1100;
const int MAX_NUM_PLAYERS = 4;


struct Player{
	string name;
	int turns;
};

//Leaderboard Array
Player leaderB[MAX_NUM_PLAYERS];
int numOfPlayers = 0;

//Mutex for locking and unlocking leaderboard
pthread_mutex_t LockLeaderboard;

//Functions
string getWord();
void* processClient(void* sock);


int main(int argc, char* argv[]){
	srand(time(NULL));
	//Initialize lock
	pthread_mutex_init(&LockLeaderboard, NULL);
	
	//Initialize Leaderboard
	for(int i=0; i<4; i++){
		leaderB[i].name = "";
		leaderB[i].turns = 99999999;
	}
	
	//Assign port to socket (12300 - 12399)
	short port = atoi(argv[PORT_NUM]);
	struct sockaddr_in address;
	struct sockaddr_in clientAddress;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1){
		cout << "Error: Could not create socket" << endl;
		return 1;
	}
	if (bind(sock, (struct sockaddr*)&address, sizeof(address)) == -1){
		cout << "Error: Could not bind socket" << endl;
		return 1;
	}
	if(listen(sock, MAX_PENDING) == -1){
		cout << "Error: Could not set socket to listen" << endl;
		return 1;
	}
	pthread_t thread;
	while (true){
		socklen_t clientAddressLen = sizeof(clientAddress);
		int clientSocket = accept(sock, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if(clientSocket == -1){
			cout << "Error: Client socket failed to connect" << endl;
			close(clientSocket);
		}
		if (pthread_create(&thread, NULL, processClient, (void*) &clientSocket) != 0){
			cout << "Error: Could not create pthread" << endl;
			close(clientSocket);
		}
	}
	close(sock);
	return 0;
}


void* processClient(void* sock){
	int clientSocket = *(int*)sock;
	int wordSize;
	char* iWordSize;
	int sendLength;
	int receiveLength;
	char buffer[MAX_MESSAGE_SIZE];
	int responseSize;
	string userName = "";
	int numGuesses = 0;
	int trackGuessed = 0;
	
	string word = getWord();
	cout << word << endl;
	
	//Receive the name.
	int numOfLetters;
	char* numOfL = (char*) &numOfLetters;
	responseSize = 0;
	while(responseSize < (int)sizeof(int)){
		receiveLength = recv(clientSocket, (void*) numOfL, sizeof(int)-responseSize, 0);
		if(receiveLength <= 0){
			cout << "Error: Could not Receive" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		responseSize += receiveLength;
		numOfL += receiveLength;
	}
	if(ntohl(numOfLetters) > MAX_MESSAGE_SIZE){
		cout << "Error: Data Invalid" << endl;
		pthread_detach(pthread_self());
		close(clientSocket);
		return NULL;
	}
	int nameSize = ntohl(numOfLetters);
	int counter = 0;
	responseSize = 0;
	while(responseSize < nameSize){
		receiveLength = recv(clientSocket, (void*) &buffer[counter], nameSize-responseSize, 0);
		if(receiveLength <= 0){
			cout << "Error: Could not Receive" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		responseSize += receiveLength;
		counter += receiveLength;
	}
	for(int i=0; i<counter-1; i++){
		if(!isalpha(buffer[i])){
			cout << "Error: Data Invalid" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
	}
	userName = buffer;
	
	//Send number of letters in word.
	int bytesSent = sizeof(int);
	wordSize = htonl(word.size());
	iWordSize = (char*) &wordSize;
	while(bytesSent > 0){
		sendLength = send(clientSocket, (void*) iWordSize, bytesSent, 0);
		if(sendLength == -1){
			cout << "Error: Could not Send" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		bytesSent -= sendLength;
		iWordSize += sendLength;
	}
	while(numGuesses < MAX_GUESSES && trackGuessed == 0){
		int responseType;
		char* iResponseType;
		int numOfTurns;
		char* iNumOfTurns;
		int wordOrLetter;
		char* iWordOrLetter = (char*) &wordOrLetter;
		responseSize = 0;
		while(responseSize < (int)sizeof(int)){
			receiveLength = recv(clientSocket, (void*) iWordOrLetter, sizeof(int)-responseSize, 0);
			if(receiveLength <= 0){
				cout << "Error: Could not Receive" << endl;
				pthread_detach(pthread_self());
				close(clientSocket);
				return NULL;
			}
			responseSize += receiveLength;
			iWordOrLetter += receiveLength;
		}
		if(ntohl(wordOrLetter) > MAX_MESSAGE_SIZE){
			cout << "Error: Data Invalid" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		int wOrL = ntohl(wordOrLetter);
		if(wOrL == 1){
			numGuesses++;
			int sizeOfGuess;
			char* iSizeOfGuess = (char*) &sizeOfGuess;
			responseSize = 0;
			while(responseSize < (int)sizeof(int)){
				receiveLength = recv(clientSocket, (void*) iSizeOfGuess, sizeof(int)-responseSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					pthread_detach(pthread_self());
					close(clientSocket);
					return NULL;
				}
				responseSize += receiveLength;
				iSizeOfGuess += receiveLength;
			}
			if(ntohl(sizeOfGuess) > MAX_MESSAGE_SIZE){
				cout << "Error: Data Invalid" << endl;
				pthread_detach(pthread_self());
				close(clientSocket);
				return NULL;
			}
			int userGuessSize = ntohl(sizeOfGuess);
			int counter = 0;
			responseSize = 0;
			while(responseSize < userGuessSize){
				receiveLength = recv(clientSocket, (void*) &buffer[counter], userGuessSize-responseSize, 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					pthread_detach(pthread_self());
					close(clientSocket);
					return NULL;
				}
				responseSize += receiveLength;
				counter += receiveLength;
			}
			for(int i=0; i<counter-1; i++){
				if(!isalpha(buffer[i])){
					cout << "Error: Data Invalid" << endl;
					pthread_detach(pthread_self());
					close(clientSocket);
					return NULL;
				}
			}
			for(int i=0; i<counter; i++){
				buffer[i] = toupper(buffer[i]);
			}
			string userGuess = buffer;
			if(userGuess == word){
				trackGuessed = 1;
				pthread_mutex_lock(&LockLeaderboard);
				leaderB[3].name = userName;
				leaderB[3].turns = numGuesses;
				if(numOfPlayers < 3){
					numOfPlayers++;
				}
				int j;
				for(int i=1; i<4; i++){
					j=i;
					while(j>0 && leaderB[j-1].turns>leaderB[j].turns){
						Player temp;
						temp.name = leaderB[j].name;
						temp.turns = leaderB[j].turns;
						leaderB[j].name = leaderB[j-1].name;
						leaderB[j].turns = leaderB[j-1].turns;
						leaderB[j-1].name = temp.name;
						leaderB[j-1].turns = temp.turns;
						j--;
					}
				}
				pthread_mutex_unlock(&LockLeaderboard);
				bytesSent = sizeof(int);
				responseType = htonl(1);
				iResponseType = (char*) &responseType;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) iResponseType, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					iResponseType += sendLength;
				}
				bytesSent = sizeof(int);
				numOfTurns = htonl(numGuesses);
				iNumOfTurns = (char*) &numOfTurns;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) iNumOfTurns, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					iNumOfTurns += sendLength;
				}
				bytesSent = sizeof(int);
				pthread_mutex_lock(&LockLeaderboard);
				int nop = htonl(numOfPlayers);
				pthread_mutex_unlock(&LockLeaderboard);
				char* numOfP = (char*) &nop;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) numOfP, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					numOfP += sendLength;
				}
				string board = "";
				pthread_mutex_lock(&LockLeaderboard);
				for(int i=0; i<numOfPlayers; i++){
					board += leaderB[i].name;
					board += to_string(leaderB[i].turns);
				}
				pthread_mutex_unlock(&LockLeaderboard);
				strcpy(buffer, board.c_str());
				bytesSent = sizeof(int);
				int leadB;
				leadB = htonl(strlen(board.c_str())+1);
				char* sizeOfLB = (char*) &leadB;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) sizeOfLB, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					sizeOfLB += sendLength;
				}
				bytesSent = ntohl(leadB);
				int count = 0;
				while (bytesSent > 0){
					sendLength = send(clientSocket, (void*) &buffer[count], bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					count += sendLength;
				}
			}else{
				bytesSent = sizeof(int);
				int in = htonl(-1);
				char* incorrect = (char*) &in;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) incorrect, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					incorrect += sendLength;
				}
			}
		}else if(wOrL == 2){
			numGuesses++;
			int counter = 0;
			responseSize = 0;
			while(responseSize < (int)sizeof(char)){
				receiveLength = recv(clientSocket, (void*) &buffer[counter], sizeof(char), 0);
				if(receiveLength <= 0){
					cout << "Error: Could not Receive" << endl;
					pthread_detach(pthread_self());
					close(clientSocket); 
					return NULL;
				}
				responseSize += receiveLength;
				counter += receiveLength;
			}
			if(!isalpha(buffer[0])){
				cout << "Error: Data Invalid" << endl;
				pthread_detach(pthread_self());
				close(clientSocket);
				return NULL;
			}
			char userGuess = toupper(buffer[0]);
			int arr[MAX_MESSAGE_SIZE];
			int j = 0;
			bool found = false;
			for(int i=0; i<(int)word.size(); i++){
				if(userGuess == word[i]){
					found = true;
					arr[j] = htonl(i);
					j++;
				}
			}
			if(found){
				bytesSent = sizeof(int);
				responseType = htonl(2);
				iResponseType = (char*) &responseType;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) iResponseType, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					iResponseType += sendLength;
				}
				int inds = htonl(j);
				char* indexs = (char*) &inds;
				bytesSent = sizeof(int);
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) indexs, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					indexs += sendLength;
				}
				bytesSent = j*sizeof(int);
				int count = 0;
				while (bytesSent > 0){
					sendLength = send(clientSocket, (void*) &arr[count], bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					count += sendLength;
				}
			}else{
				bytesSent = sizeof(int);
				int in = htonl(-1);
				char* incorrect = (char*) &in;
				while(bytesSent > 0){
					sendLength = send(clientSocket, (void*) incorrect, bytesSent, 0);
					if(sendLength == -1){
						cout << "Error: Could not Send" << endl;
						pthread_detach(pthread_self());
						close(clientSocket);
						return NULL;
					}
					bytesSent -= sendLength;
					incorrect += sendLength;
				}
			}
		}else{
			cout << "Error: Could not Receive" << endl;
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
	}
	pthread_detach(pthread_self());
	close(clientSocket);
	return NULL;
}

string getWord(){
	string path = "/home/fac/lillethd/cpsc3500/projects/p4/words.txt";
	ifstream myfile(path.c_str());
	string word = "";
	int index = rand() % NUMOFWORDS;
	for(int i=0; i<index; i++){
		myfile >> word;
	}
	return word;
}

