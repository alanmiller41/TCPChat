/*
Alan Miller CSCI 335
This program is a multithreaded server application that
can send messages back and forth with a client.  All 
messages are stored in a serverside output chatLog.txt file.
It was my intention to make it a multiclient server but I had 
difficulty getting the program to send messages to every client.
Some of the multiclient functionality remains.  More than one
client may send messages to the server, but the server cannot
relay information back to any client other than the first.

Known Bugs Include: 
	When a message is received the prompt prints
	on the line above the shell input.

	Occasionally the exit command can cause 
	the program to loop and print its prompt repeatedly
	before exiting.  This is obnoxious because it ends
	up in the chatlog.txt file.
*/


#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 400


void* error(char *msg){
	perror(msg);
	exit(1);
}

/*
Made a struct containing the socket information
in order to pass it into a pthread_create call.ß
*/
typedef struct{
	int socketfd;

}socketInfo;

/*
Array of current clients and the number of clients.
Mutexes for exclusion when adding/accessing clients
as well as for accessing the file pointer.ß
*/
struct sockaddr_in clients[5];
int numberOfClients;
pthread_mutex_t numberClientsLock;
pthread_mutex_t clientArrayLock;
pthread_mutex_t fileLock;

FILE *chatLog;

/*
This method is called after threads are created and takes user input
and sends it to a socket to be read by clients.
*/
void sendMsg(int socketfd, struct sockaddr_in *client_address){
	char sBuffer[BUFFER_SIZE];
	char message[BUFFER_SIZE + 9];
	int n, i;
	while(1){
		
		
		bzero(sBuffer,BUFFER_SIZE);
		bzero(message,BUFFER_SIZE+9);
		
		printf("\nServer: ");

		fgets(sBuffer, BUFFER_SIZE, stdin);
		if(!strncmp(sBuffer, "exit", 4)){
			send(socketfd, sBuffer, strlen(sBuffer), 0);
			exit(0);
		}

		//Formatting for file output and mutex access.
		strcat(message, "Server: ");
		strcat(message, sBuffer);
		pthread_mutex_lock(&fileLock);
		fputs(message, chatLog);
		pthread_mutex_lock(&fileLock);

		/*
		This loop is supposed to cause the message string to be sent
		to every client, however it only seems to send to the first.
		*/
		for(i = 0; i< numberOfClients; i++){
			n = sendto(socketfd, message, strlen(message), 0,
				(struct sockaddr *) &clients[i], sizeof(clients[i]));
			if(n <0){
				error("ERROR writing to socket");
			
			}
		}

	}
}

/*
This method loops through a call to receive messages
from a socket so that another process may be dedicated to
sending messages.
Casts socket as a void * so that it can be used easily
with threads.
*/
void* getMsg(void * socket){
	struct sockaddr_in;
	int sockfd, n;
	char cBuffer[BUFFER_SIZE];
	//Casting socket as an int
	sockfd = (int) socket;

	while(1){

		bzero(cBuffer,256);
		//Receive message from client.
		n = recvfrom(sockfd, cBuffer, BUFFER_SIZE, 0 , NULL, NULL);

		if (strncmp(cBuffer, "exit", 4) == 0){
			exit(0);
		}

		//printf("getMsg Thread started, n2 = %d\n", n);
		if(n< 0){
			error("ERROR reading from socket");
			
		}
		printf("\n\n%s\nServer: \n", cBuffer);

		//Mutual exclusion for chatLog.
		pthread_mutex_lock(&fileLock);
		fputs("\n\n", chatLog);
		fputs(cBuffer, chatLog);
		pthread_mutex_unlock(&fileLock);
		


		/*
		This code was causing errors but was supposed to, like above,
		loop through the clients array and write messages to them.
		
		// for(i = 0; i< numberOfClients; i++){
		// 	n = sendto(sockfd, cBuffer, strlen(cBuffer), 0,
		// 		(struct sockaddr *) &clients[i], sizeof(clients[i]));
		// 	if(n <0){
		// 		error("ERROR writing to socket");
			
		// 	}
		// }
		*/
	}
}

/*
This method is invoked by a thread and waits while any clients
may attempt to join the server.  Once clients are joined another
getMessageThread is created to listen for that client.
*/
void* getClients(void * socket){
	struct sockaddr_in cli_addr;
	int sockfd, newsockfd, clilen;
	clilen = sizeof(cli_addr);
	sockfd = (int) socket;

	//5 is the number of clients the server can handle
	listen(sockfd,5);

	while(1){
		newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0){
			error("ERROR on accept");
			
		}
		printf("Sockfd accepted\n");

		//Global clients array member gets info about new client.
		pthread_mutex_lock(&clientArrayLock);
		clients[numberOfClients].sin_family = cli_addr.sin_family;
		clients[numberOfClients].sin_port = cli_addr.sin_port;
		clients[numberOfClients].sin_addr.s_addr = cli_addr.sin_addr.s_addr;
		pthread_mutex_unlock(&clientArrayLock);
		
		pthread_mutex_lock(&numberClientsLock);
		numberOfClients += 1;
		pthread_mutex_unlock(&numberClientsLock);

		pthread_t getMessageThread;
		pthread_create(&getMessageThread, NULL, getMsg, (void *) newsockfd);
		
	}
}

int main(int argc, char *argv[]){

	//Open file pointer.
	chatLog = fopen("chatLog.txt", "w");
	numberOfClients = 0;
	int newsockfd, sockfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;

	if(argc < 2){
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	//initialize the socket with the usual arguments.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0){
		error("ERROR opening socket");
	}

	//sets server address to zeros.
	bzero((char *) &serv_addr, sizeof(serv_addr));

	//convert argument string to integer
	portno = atoi(argv[1]);

	//set information for serv_addr struct
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	//Bind to the provided socket
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
		error("ERROR on binding\n");
	}

	//length of client address
	clilen = sizeof(cli_addr);
	//Listen for clients
	listen(sockfd,5);
	//Accept first client connection
	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
	printf("Sockfd accepted\n");
	 if (newsockfd < 0){
	 	error("ERROR on accept\n");
	}

	numberOfClients+=1;

	/*
	Create threads to run the getClients and getMessageThreads while
	the server is still able to send messages with the sendMsg function.
	*/
	pthread_t getMessageThread, getClientThread;
	pthread_create(&getClientThread, NULL, getClients, (void *) sockfd);
	pthread_create(&getMessageThread, NULL, getMsg, (void *) newsockfd);
	sendMsg(newsockfd, &cli_addr);

	/*
	Teardown
	*/
	fclose(chatLog);
	close(newsockfd);
	close(sockfd);
	pthread_exit(NULL);
	return 0;

}
