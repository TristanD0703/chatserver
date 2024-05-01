#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

void* userThread(void* args);
void* serverThread(void* args);

void* userThread(void* args) {
	//continuously listens for input from the user, then sends string to server
	char* buff = (char*) malloc(513 * sizeof(char));
	size_t readCount;
	int connection_fd = (int)(long)args;

	while((readCount = (size_t)read(0, buff, sizeof(char)*512)) > 0){
		buff[(unsigned int)readCount] = '\0';
		//Checks for the quit command "\q". Closes the client if detected.
		if(buff[0] == '\\' && buff[1] == 'q'){
			close(connection_fd);
			exit(0);
		}
		write(connection_fd, buff, readCount + 1); 
	}
	return 0;
}

void* serverThread(void* args) {
	int connection_fd = (int)(long)args;
	char* buff = (char*) malloc(513 * sizeof(char));
	size_t readSize;

	//Listens for data from the server. If data is found, prints it to the terminal.
	while((readSize = (size_t) read(connection_fd, buff, sizeof(char) * 512)) > 0){
		write(0, buff, readSize);
	}
	free(buff);
	return 0;
}

int main(void){
	//Sets up a socket to listen for local connections on port 30212
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(30212);

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	//takes input from the user for their username and stores it in var
	char username[33];
	printf("Enter your username (max 32 characters)\n");
	size_t usrSize = (size_t)read(0, username, 32 * sizeof(char));
	username[32] = '\0';
	username[(unsigned int) usrSize - 1] = '\0';

	printf("logging in as %s\n", username);	

	//attempts to connect to the server. exits if unable to connect
	if(connect(serverSocket, (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "connection failed\n");
		return 2;
	}
	//sends username to server
	write(serverSocket, username, strlen(username));

	pthread_t user;
	pthread_t server;
	pthread_create(&user, NULL, userThread, (void*)(long)serverSocket);
	pthread_create(&server, NULL, serverThread, (void*)(long)serverSocket);

	void* ret;
	pthread_join(user, &ret);
	pthread_join(user, &ret);

	close(serverSocket);
}
