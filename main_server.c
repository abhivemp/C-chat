/* Abhi Vempati & Kevin Williams
   OS Project 4
   Client files for the server!!
   Professor DeGood
   CSC 345 -02
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_NUM 3000

char colors[8][9] = {
	"\x1B[31m", //RED 
	"\x1B[32m", //GRN 
	"\x1B[33m", //YEL
	"\x1B[34m", //BLU
	"\x1B[35m", //MAG
	"\x1B[36m", //CYN
	"\x1B[37m", //WHT 
	"\x1B[0m" //RESET 
};	

void error(const char *msg){
	perror(msg);
	exit(1);
}

/* Room Configurations for the project */

typedef struct _USR {
	int clisockfd;		// socket file descriptor
	char username[200];
	int userColor;      // Each client would have a color going in
	struct _USR* next;	// for linked list queue
} USR;

typedef struct _Room {
	USR* headPtr;
	int cliCount;
	int roomNumber;
} Room;

/* Maximium of 4 rooms */
Room rooms[4];

/* add tail allows client info to be stored in a linked list */
void add_tail(int newclisockfd, char usern[], int roomNo){
	
	int col = rand() % 7;
	
	if(rooms[roomNo].headPtr == NULL) {
		USR* temp = (USR*) malloc(sizeof(USR));
		temp->clisockfd = newclisockfd;
		strcpy(temp->username, usern);
		temp->userColor = col;
		temp->next = NULL;
		rooms[roomNo].headPtr = temp;
		rooms[roomNo].cliCount++;
	} else {
		USR* temp = (USR*) malloc(sizeof(USR));
		temp->clisockfd = newclisockfd;
		strcpy(temp->username, usern);
		temp->userColor = col;
		temp->next = rooms[roomNo].headPtr;
		rooms[roomNo].headPtr = temp;
		rooms[roomNo].cliCount++;
	}

}

/* this updates the rooms array and their list whenever a client disconnects
   Printed on server */
void disc(int remclisockfd, int roomNo){

	USR* temp = rooms[roomNo].headPtr;
	USR* tempPre = NULL;
	if(temp == NULL){return;}
	if(temp->clisockfd == remclisockfd){
		printf("\t%s%s%s\n", colors[temp->userColor], temp->username, colors[7]);
		rooms[roomNo].headPtr = temp->next;
		rooms[roomNo].cliCount--;
	}else{

		while(temp->next != NULL){
			if(temp->clisockfd == remclisockfd){
				break;
			}
			tempPre = temp;
			temp = temp->next;
		}

		if(temp->next == NULL){
			printf("\t%s%s%s\n", colors[temp->userColor], temp->username, colors[7]);
			tempPre->next = NULL;
			rooms[roomNo].cliCount--;
		}else{
			printf("\t%s%s%s\n", colors[temp->userColor], temp->username, colors[7]);
			tempPre->next = temp->next;
			rooms[roomNo].cliCount--;
		}
	
	}

}

/* Prints the room status. This method is sent as a
   char and sends to client page to print */
char* display() {

	USR* temp;
	char* text = malloc(1000 * sizeof(char));
	char buf[200];
	for(int i = 0; i < 4; i++) {
		temp = rooms[i].headPtr;
		snprintf(buf, sizeof(buf),"room %d with %d client(s)\n", rooms[i].roomNumber,rooms[i].cliCount);
		strcat(text,buf);
		memset(buf,0,200);
	}			
	return text;

}
/* Displays an up to date list of clients after someone 
   Connects or Disconnects
   It even adds their color too! */
void serveDisp(){

	USR* temp;
	printf("\n");
	for(int i = 0; i < 4; i++) {
		temp = rooms[i].headPtr;
		printf("Clients in room number %d:\n", i);
		if(temp != NULL){
			while(temp != NULL){
				printf("\t%s%s%s\n", colors[temp->userColor], temp->username, colors[7]);
				temp = temp->next;
			}
		}else{
			printf("\n");
		}
	}	
	printf("\n");

}
/* This method is supposed to send the message to all other clients */
void broadcast(int fromfd, char* message, int roomNo) {
	

	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");

	// traverse through all connected clients

	USR* cur = rooms[roomNo].headPtr;
	char buffer[512];
	int nmsg = 0;
	int nsen = 0;
	char* senderName;
	int sendCol;

	while(cur != NULL) {

		if(cur->clisockfd == fromfd) {
			senderName = cur->username;
			sendCol = cur->userColor;
		}
		cur = cur->next;
	}

	cur = rooms[roomNo].headPtr;

	while (cur != NULL) {
		/* Send the message to everyone besides the sender */
		
		if (cur->clisockfd != fromfd) {
			memset(buffer,0,512);
			/* Prepare and send message; formatting to the client as well */
			sprintf(buffer, "%s[%s]:%s %s", colors[sendCol], senderName, message, colors[7]);
			nmsg = strlen(buffer);
		
			nsen = send(cur->clisockfd, buffer, nmsg, 0);
			if(nsen == -1) error("ERROR send() failed\n");
			// if(nsen == 0) printf("disconnected: %s", cur->username);
			// if (nsen != nmsg) error("ERROR send() failed");
		}

		cur = cur->next;
	}
}

typedef struct _ThreadArgs {
	int clisockfd;
	int roomNumber;
	
} ThreadArgs;

void* thread_main(void* args){

	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	// get socket descriptor from argument
	int clisockfd = ((ThreadArgs*) args)->clisockfd;
	int roomNo = ((ThreadArgs*) args)->roomNumber;
	
	free(args);

	//-------------------------------
	// Now, we receive/send messages
	char buffer[256];
	int nsen, nrcv;
	memset(buffer,0,256);
	nrcv = recv(clisockfd, buffer, 255, 0);
	if (nrcv < 0) {
	 error("ERROR recv() failed");
	}
	
	/* this part of the code was initially buggy
	   buffer needs to be lear before receiving data. Not clearning the buffer 
	   will lead to printing out some of the bytes from teh previous message 
	*/
	while (nrcv > 0) {
		// we send the message to everyone except the sender
		broadcast(clisockfd, buffer, roomNo);

		memset(buffer,0,256);
		nrcv = recv(clisockfd, buffer, 255, 0);
		if (nrcv < 0) error("ERROR recv() failed");
		
	}
	/* Print the client info */
	printf("Client diconnected: ");
	disc(clisockfd, roomNo);
	serveDisp();
	close(clisockfd);
	//-------------------------------
	return NULL;

}

int main(int argc, char *argv[]){

	/* Configurations */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	//serv_addr.sin_addr.s_addr = inet_addr("192.168.1.171");	
	serv_addr.sin_port = htons(PORT_NUM);

	int status = bind(sockfd, 
			(struct sockaddr*) &serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5); // maximum number of connections = 5

	/* Configure all rooms */
	for(int i = 0; i < 4; i++) {
		rooms[i].headPtr = NULL;
		rooms[i].cliCount = 0;
		rooms[i].roomNumber = i;
	}

	/* Substringing the input to receive username and roomnum */
	char incoming[256];
	char* username;
	char* dlim = ":";
	char* roomQuo;
	char noneRoomNo[10];
	char returnMsg[10];

	int x;

	while(1) {
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, 
			(struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0) error("ERROR on accept");

		/* Recieved argument from client */
		memset(incoming,0,256);
		x = recv(newsockfd,incoming, 255, 0);
		if(x < 0) error("username not received from client");

		username = strtok(incoming,dlim);
		roomQuo = strtok(NULL, dlim);


		/* if new, then add them in the next available room */
		/* empty -- print out room status */
		/* if num, then add them in that room num */
		int i;
		int actualRoomNo = -1;
		memset(returnMsg, 0, 10);
		if(strcmp("new",roomQuo) == 0) {
			i = 0;
			while(i < 4) {
				if(rooms[i].cliCount == 0) {
					actualRoomNo = i;
					add_tail(newsockfd,username, actualRoomNo);
					sprintf(returnMsg, "%d", i);
					break;
				}
				i++;
			}

			if(i == 4) {
				/* if no room is available send that message to the client */
				strcpy(returnMsg, "Out");
			}

		/* If no 4th argument from client */
		} else if(strcmp("None", roomQuo) == 0) {
			
			int dx = send(newsockfd, display(), sizeof(char) * 1000,0 );
			if(dx < 0) {error("Failed to send display() info");}

			memset(noneRoomNo, 0, 10);
			dx = recv(newsockfd, noneRoomNo, 10, 0);
			if(dx < 0) {error("not recived from user");}

			/* After second prompt, if they typed in new */
			if(strncmp("new",noneRoomNo, 3) == 0){
				int index = 0;
				while(index < 4){
					if(rooms[index].cliCount == 0){
						actualRoomNo = index;
						add_tail(newsockfd, username, actualRoomNo);
						sprintf(returnMsg, "%d", index);
						break;
					}
					index++;
				}
				if(index == 4){
					strcpy(returnMsg, "Out");
				}
			}else{
				actualRoomNo = atoi(noneRoomNo);
				add_tail(newsockfd, username, actualRoomNo);
				strcpy(returnMsg, noneRoomNo);
			}
		} else {	
			actualRoomNo = atoi(roomQuo);
			add_tail(newsockfd,username, actualRoomNo); 
			strcpy(returnMsg, roomQuo);
		}
		/* Print the client list if someome connects */
		if(actualRoomNo != -1){
			printf("Connected: %s, %d\n", username, actualRoomNo);
			serveDisp();
		}

		int sx = send(newsockfd, returnMsg, strlen(returnMsg), 0);
		if(sx < 0) error("Error sending response to user");
	
		/* the if statement prevents a client to connect if they aren't in 
		   a room, makes it efficient */
		// prepare ThreadArgs structure to pass client socket
		if(actualRoomNo != -1){
	
			ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
			if (args == NULL) error("ERROR creating thread argument");
			
			args->clisockfd = newsockfd;
			args->roomNumber = actualRoomNo;
		

			pthread_t tid;
			if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
		}

	}
	return 0; 

}