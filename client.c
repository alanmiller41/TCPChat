/*
Alan Miller CSCI 335
This program is a multithreaded client application that
can send messages back and forth with a server.  All 
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
#include <strings.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>




#define USERNAME_SIZE 30
#define BUFFER_SIZE 400

int n;
char globalPrompt [USERNAME_SIZE+2];

/*
Using a struct to allow us to pass multipler parameters into
a function while starting it using threads.
*/
typedef struct {
	int socketfd;
	//char *prompt;
}connectionInfo;

void* error (char *msg){
	//perror(msg);
	exit(0);
}

/*
This method is generally the same as the sendMsg method 
in server.c with the exception of not attempting to send
a message to every other client, just to the server.
*/
void sendMsg(char prompt[USERNAME_SIZE+2], int sockfd, struct sockaddr_in *address){

	int message_size = BUFFER_SIZE+USERNAME_SIZE+2;
	char cBuffer[BUFFER_SIZE];
	char message[BUFFER_SIZE+USERNAME_SIZE+2];
	while(1){

		//Clear the message buffer
		bzero(cBuffer,BUFFER_SIZE);
		bzero(message, message_size);
		//Concatinate the prompt and buffer to send
		printf("\n%s", globalPrompt);
		fgets(cBuffer, BUFFER_SIZE, stdin);
		strcat(message, prompt);
		strcat(message, cBuffer);

		//User may quit program using exit command
		if (strncmp(cBuffer, "exit", 4) == 0){
			//send exit message to server.
			send(sockfd, cBuffer, strlen(cBuffer), 0);
			exit(0);
		}

		n = send(sockfd, message, strlen(message), 0);

		if(n < 0){
			error("ERROR writing to socket");
		}
	}
}

/*
This method is generally the same as the getMsg method in
server.c
*/
void* getMsg(void * info){
	int sockfd, n;
	char sBuffer[BUFFER_SIZE];

	connectionInfo* threadInfo = (connectionInfo*)info;
	sockfd = threadInfo->socketfd;

	while (1){

		bzero(sBuffer,BUFFER_SIZE);
		//Writes from socket to sBuffer.  No flags and since
		//we don't want to save the sender address we can leave
		//last two arguments as null pointers.
		n = recvfrom(sockfd, sBuffer, BUFFER_SIZE, 0, NULL, NULL);

		if(n < 0){
			error("ERROR reading from socket");
		}

		if (strncmp(sBuffer, "exit", 4) == 0){
			exit(0);
		}

		printf("\n\n%s\n%s", sBuffer, globalPrompt);
		
		/*
		It seems that attempting to print anything after this buffer
		won't appear on the same line as the shell prompt.  This results
		in there being an extra new line while prompting a user
		after they have received a message from the server.

		printf("\n%s", globalPrompt);
		*/
		printf("\n");
	

	}
}

int main(int argc, char *argv[]){

	int portno;
	int sockfd, n;

	struct sockaddr_in serv_addr;
	struct hostent *server;

	char prompt[USERNAME_SIZE+2];
	char userName[USERNAME_SIZE];

	/*
	We need arguments for both portnumber and hostname in
	the client side program.
	*/
	if(argc < 3){
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0){
		error("ERROR opening socket");
	}

	server = gethostbyname(argv[1]);
	if(server == NULL){
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	//copy server info into serv_addr struct
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

	serv_addr.sin_port = htons(portno);

	if(connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0){
		error("ERROR connecting");
	}

	printf("Please enter Username: ");
	fgets(userName,USERNAME_SIZE,stdin);
	//Deleting new line from end of username
	userName[strlen(userName)-1] = 0;
	userName[strcspn(userName, "\n")] = 0;

	strcpy(prompt, userName);
	strcat(prompt, ": ");
	strcpy(globalPrompt, prompt);
	connectionInfo info;
	info.socketfd = sockfd;
 

	printf("\n\n");

	pthread_t getThread;

	/*
	Create thread to receive messages.  Casts the info struct
	as a void * which pthread_create expects.
	*/
	pthread_create(getThread, NULL, getMsg, (void *) &info);

	sendMsg(prompt, sockfd, &serv_addr);
	close(sockfd);
	pthread_exit(NULL);

	return 0;
}