#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNECTIONS 32

void* connectionThread(void* arg);
void sendMessage(const char* msg);

//Struct holding important metadata for each connection
typedef struct connection {
	int fileDescriptor;
	short active;
	char username[33];
} Connection;

static Connection* connectionFDs; //Array of connections
static int connectionCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Will be called when a message needs to be sent to all clients
void sendMessage(const char* msg){
	printf("received message!\n");
	//important to lock since this is shared data.
	pthread_mutex_lock(&mutex);
	puts(msg);

	//Loops through all the current connetions, writing to the socket if it is active.
	for(int i = 0; i < MAX_CONNECTIONS; i++){
		if(connectionFDs[i].active){
			write(connectionFDs[i].fileDescriptor, msg, strlen(msg));
		}
	}	
	pthread_mutex_unlock(&mutex);
}

void* connectionThread(void* arg) {
	char buff[513];				//To hold data recieved from client
	size_t numRead; 			//Byte count read from the connection
	int connectionIndex = (int)(long)arg; 	//Index of the current connection's data in the global connections array
	Connection* conn = &connectionFDs[connectionIndex]; //grabs a pointer to the current connection's data
	
	//Capping the buffers to ensure there are no errors/vulnerabilities from reading the string
	buff[512] = '\0';			
	conn->username[32] = '\0';
	
	//Reads the first data received by client as the username
	numRead = (size_t)read(conn->fileDescriptor, &conn->username, sizeof(char) * 32);
	conn->username[(unsigned int) numRead + 1] = '\0'; //Caps the string to only include the username
	
	//Checks other connections' data to ensure the username received is not already in use
	for(int i = 0; i < MAX_CONNECTIONS; i++){
		if(i != connectionIndex && connectionFDs[i].active && strcmp(connectionFDs[i].username, conn->username) == 0){
			//Cleanup
			char* bad ="Username is already in use! Please use a different username.\n";
			write(conn->fileDescriptor, bad, strlen(bad));
			goto cleanup;
		}
	}
	printf("User %s logged in!\n", conn->username);
	char welcome[60];
	snprintf(welcome, 60, "%s has joined the chatroom\n", conn->username); 
	sendMessage(welcome);

	
	//Infinitely reads the data from the connection until it detects a connection closure
	while((numRead = (size_t)read(conn->fileDescriptor, &buff, sizeof(char) * 512)) > 0){
		char message[550];
		snprintf(message, 550, "<%s> %s\n", conn->username, buff); 
		//sends username concat with the connection buffer to all clients
		sendMessage(message);	
	}
	
	//When connection closure is detected, sets the file descriptor in the array to default,
	//then closes the connection and destroys the thread
cleanup:
	pthread_mutex_lock(&mutex);
	conn->active = 0;
	close(conn->fileDescriptor);
	connectionCount--;
	pthread_mutex_unlock(&mutex);
	printf("connection ended\n");
	
	return 0;	
}



int main(void) {
	//Allocates an array to store connection file descriptors. Inits all to 0 to remove noise.
	connectionFDs = calloc(sizeof(Connection), MAX_CONNECTIONS * sizeof(Connection));
	
	//Defines data for server socket configuration
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(30212);

	//Initializes socket file descriptor
	int inputSocket = socket(AF_INET, SOCK_STREAM, 0);

	//Attaches socket to the IP and port defined in "addr". Exits the program if this fails
	if(bind(inputSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0){
	       fprintf(stderr, "binding failed\n");
	       return 1;
	}		

	//begins listening on port 30212. Exits if this fails.
	if(listen(inputSocket, MAX_CONNECTIONS) < 0) {
		fprintf(stderr, "listen start failed\n");
		return 2;
	}

	int running = 1;
	int connection_fd;
	pthread_t thread;
	
	//Starts an infinite loop to continuously accept connections
	while(running) {
		puts("waiting on connection\n");

		//Waits for a connection. Then, collects the file descriptor of the connection socket.
		//Tries again if this fails
		if((connection_fd = accept(inputSocket, NULL, NULL)) < 0) {
			fprintf(stderr, "error on accepting connection");
			continue;
		}

		printf("Connection received!\n");
		//Checks if the amount of total connections exceeds the maximum allowed. Refuses connection if so.
		if(connectionCount == MAX_CONNECTIONS){
			printf("Cannot accept, too many connections\n");
			close(connection_fd);
			continue;
		}
		connectionCount++;
		
		//Creates a new connection struct to track this connection
		Connection newConnection;
		newConnection.active = 1;
		newConnection.fileDescriptor = connection_fd;

		//Finds an available spot in the connection file descriptor array to record this connection.
		//This will allow us to send messages to this client in the future.
		for(int i = 0; i < MAX_CONNECTIONS; i++){
			if(connectionFDs[i].active == 0){
				//important to lock the thread since multiple threads are accessing this data
				pthread_mutex_lock(&mutex);
				connectionFDs[i] = newConnection;
				pthread_mutex_unlock(&mutex);

				
				//Starts a new thread to monitor this connection. 
				//send the thread the index of this connections metadata
				pthread_create(&thread, NULL, &connectionThread, (void*)(long)i);
				break;
			}
		}
		pthread_detach(thread);
	}
}
