/* Abhi Vempati & Kevin Williams
   OS Project 4
   Client files for the server!!
   Professor DeGood
   CSC 345 -02
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>

/* using a port number higher than 1024 so we don't have to be in root */
#define PORT_NUM 3000

void error(const char *msg){
	perror(msg);
	exit(0);
}

/* Struct for threads to create client thread socket descriptors */
typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

void* thread_main_recv(void* args){
	

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	char buffer[512];
	// keep receiving and displaying message from server

	/* This change here is important, we want the first message to be displayed
	   n was by default 0. Allowing it to be 1 at the start will receive messages
	   Also, clear buffer before receive was missing. Added that here
	*/ 

	int n = 1;

	// n = recv(sockfd, buffer, 512, 0);
	
	while (n > 0) {
		memset(buffer, 0, 512);
		n = recv(sockfd, buffer, 512, 0);
		
		if (n < 0) error("C: main_recv: ERROR recv() failed");
		printf("\n%s\n", buffer);
	}
	return NULL;

}

void* thread_main_send(void* args){

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep sending messages to the server
	char buffer[256];
	int n;

	while (1) {
		// You will need a bit of control on your terminal
		// console or GUI to have a nice input window.
		memset(buffer, 0, 256);
		fgets(buffer, 255, stdin);

		if (strlen(buffer) == 1) buffer[0] = '\0';

		n = send(sockfd, buffer, strlen(buffer), 0);
		if (n < 0) error("ERROR writing to socket");

		/* Disconnected statement goes here. Add in broadcast
		   Check bits that it is 0 for it to be empty */
		if (n == 0) {
			break; // we stop transmission when user type empty string
		}
	}
	return NULL;

}


int main(int argc, char *argv[]) {

	if (argc < 3) error("Please specify hostname. If not enter username to connect");
	
	/* Validation that the user entered in 4 arguments and to check
	if it's new or number */
	char roomQuo[10] = "None";
	if (argc == 4) {
		strcpy(roomQuo, argv[3]);
		if((atoi(argv[3]) >= 4) && (!(strcmp("new",roomQuo) == 0))) {
			error("Room number out of bounds");
		}
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");	

    /*Allowing clients to conect to local host and not through IP ;;)) */
	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT_NUM);


	printf("Try connecting to %s...\n", inet_ntoa(serv_addr.sin_addr));

	int status = connect(sockfd, 
			(struct sockaddr *) &serv_addr, slen);
	if (status < 0) error("ERROR connecting");

	/* Allow client to add username when connecting */
	/* Concat and parsing in one send. This is username and the room option */
	char* username = argv[2];
	strcat(username,":");
	strcat(username , roomQuo);

	int x = send(sockfd, username, strlen(username), 0);
	if (x < 0) error("ERROR in sending username");


	/* if not 4th arg */
	if(strcmp("None", roomQuo) == 0) {
		char* display = malloc(sizeof(char) * 1000);
		int dx = recv(sockfd,display, (sizeof(char) * 1000), 0);
		if(dx < 0) error("Reciving display");

		printf("The folowing rooms are available:\n");
		printf("%s\n",display);

		printf("\nEnter a room number or type \"new\" if there is an open one\n:");
		char buffer[20];
		memset(buffer, 0, 20);
		fgets(buffer, 19, stdin);
		dx = send(sockfd, buffer, strlen(buffer), 0);
		if(dx < 0) error("Sending new choice");
	}

 //----------------------------------------------------------
	
	/* return message if connected to room */
	char returnMsg[10];
	memset(returnMsg, 0,10);
	int rm = recv(sockfd,returnMsg,10,0);

	if( rm < 0) error("Failed to get return message from server");

	if(strcmp("Out", returnMsg) == 0) {
		printf("Rooms are full. Please wait or enter another room\n");
		exit(1);
		
	} else {
		printf("SUCCESS: Connected to server with room %s \n", returnMsg);
	}

    /* Respective threads for each client's send and recieve to server and to the other clients */
	pthread_t tid1;
	pthread_t tid2;

	ThreadArgs* args;
	
	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid1, NULL, thread_main_send, (void*) args);

	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid2, NULL, thread_main_recv, (void*) args);

	// parent will wait for sender to finish (= user stop sending message and disconnect from server)
	pthread_join(tid1, NULL);

	close(sockfd);

	return 0;
}