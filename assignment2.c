///////////////////////////////////////////////////////////////////////////
//
//
//  Some of this code was taken from William Black (william.black@ucalgary.ca): 
//	https://uofc-my.sharepoint.com/personal/william_black_ucalgary_ca/_layouts/15/onedrive.aspx?id=%2Fpersonal%2Fwilliam%5Fblack%5Fucalgary%5Fca%2FDocuments%2FCPSC441%2FW2022%2FTutorials%2FTutorial%2D07%2DUDP%2FUDP%2DClient%2DServer%2Fudpdemo%2Dserver%2Ec&parent=%2Fpersonal%2Fwilliam%5Fblack%5Fucalgary%5Fca%2FDocuments%2FCPSC441%2FW2022%2FTutorials%2FTutorial%2D07%2DUDP%2FUDP%2DClient%2DServer
//
//
///////////////////////////////////////////////////////////////////////////

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<sys/time.h>
#include<stdbool.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

#define SIMPLE

//#define PORT_NUM 9002
int PORT_NUM;

char* split(char*, bool);
char* merge(char*, char*);
void wait(long);

int main(int argc,char *argv[])
{
	PORT_NUM = atoi(argv[1]); // user can enter a desired port number to be used

	printf("Hello there! I am the vowelizer server!\n");

	int tcp_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // init TCP socket
	if(tcp_sock_fd == -1) { perror("Failed to create socket"); return 1;}

  	// construct and initialize UDP socket
	ssize_t sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sockfd == -1) { perror("Failed."); exit(EXIT_FAILURE); }

  	// construct sockaddr_in and fill in PORT_NUM and IP address.
	struct sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_port = htons(PORT_NUM);
	sock.sin_addr.s_addr = INADDR_ANY;
	socklen_t len = sizeof(sock);
	inet_pton(AF_INET,"136.159.5.27",&sock.sin_addr);

	// blank sockaddr_in to be used for UDP connection
	struct sockaddr_in client;
	memset(&client, 0, sizeof(struct sockaddr_in));

	// bind TCP socket
	if (bind(tcp_sock_fd, (struct sockaddr*)&sock, sizeof(struct sockaddr_in)) == -1) {
		perror("Failed to bind socket\n");
		return EXIT_FAILURE;
	}

	// bind UDP socket
 	ssize_t bind_err = bind(sockfd,
		(struct sockaddr*) &sock, 
		sizeof(struct sockaddr_in));

	// check if bind was successful
	if (bind_err == -1) {
		perror("Failed to bind socket\n");
		return EXIT_FAILURE;
	}

	printf("I am running on TCP port number %d and UDP port number %d.\n", 
		PORT_NUM, PORT_NUM);


	//Mark socket as a passive socket i.e one that will accept connections
	//Set the queue length of incoming connections to be 5
	if(listen(tcp_sock_fd,5) == -1) { return EXIT_FAILURE; }

	printf("Connecting to client... \n");

	// temp sockaddr_in to be used when accepting TCP client connection
	struct sockaddr_in temp;
	socklen_t t = sizeof(struct sockaddr_in);
	int connection_fd;

	// ensure that info sent by sendto function in client is obtained in less than 5 seconds
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));

	// receive info through UDP from client in order to establish connection
	while (recvfrom(sockfd, NULL, 1024, MSG_WAITALL,
		(struct sockaddr*)&client, &len) == -1) {
			printf("Waiting...\n");
			//return -1;
		}

	//program will not leave this loop
	while((connection_fd = accept(tcp_sock_fd,(struct sockaddr*)&temp,&t)) != -1) {
		printf("Client connected! \n\n");

		int pid = fork();
		if(pid == 0) //Child
		{
			while(1) {
				int input = 0;

				// receive desired input from client to split, merge, or quit
				ssize_t size = recv(connection_fd, &input, 1024, 0);
				if (size == -1) {
					printf("Receive failed.\n");
					return -1;
				}

				printf("Input received: '%d'\n\n", input);
								
				// if i == 0, split word
				if (input == 1) {
					char buf[1024] = {'\0'};

					// receive sentence to be split from client, put in buf
					ssize_t check = recv(connection_fd,buf,1024,0);
					if (check == -1) {
						printf("Receive failed.\n");
						return -1;
					}

					printf("\nSentence to split: '%s'\n", buf);

					// split word in buf, and put the non vowels in a char[]
					char* splitConsons = split(buf, true);
					char splitTemp[strlen(splitConsons)];
					strcpy(splitTemp, splitConsons);

					// send all non vowels back to client through TCP
					if ((send(connection_fd, splitTemp, sizeof(splitTemp), 0) == -1)) {
						printf("TCP Send failed.\n");
						return -1;
					}

					printf("Sent %zd bytes '%s' to client using TCP\n", 
						sizeof(splitTemp), splitTemp);

					wait(0.1); // wait to make sure that client is ready to receive info

					// split word in buf, put vowels in char[]
					char* splitVowels = split(buf, false);
					char splitTemp2[strlen(splitVowels)];
					strcpy(splitTemp2, splitVowels);
					splitTemp2[strcspn(splitTemp2, "\n")] = 0;

					len = sizeof(client);

					// send vowels from split tp client through UDP
					if ((sendto(sockfd, splitTemp2, sizeof(splitTemp2), MSG_CONFIRM,
						(struct sockaddr*)&client, len)) == -1) {
							printf("UDP Send failed.\n");
						}

					printf("Sent %zd bytes '%s' to client using UDP\n\n", 
						sizeof(splitTemp2), splitTemp2);

				// if input == 2, merge words	
				} else if (input == 2) {
					char nonvowels[1024] = {'\0'};
					char vowels[1024] = {'\0'};

					// receive nonvowels of word through TCP
					ssize_t bytes = recv(connection_fd, nonvowels, 1024, 0);
					if (bytes == -1) {
						printf("Receiving through TCP failed.\n");
					}

					printf("\nGot %zd bytes '%s' from client using TCP\n", bytes, nonvowels);

					len = sizeof(client);

					// timeval to make sure that receiving info doesn't take too long
					struct timeval tv;
					tv.tv_sec = 5;
					tv.tv_usec = 0;
					setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));

					// receive vowels of word through UDP
					ssize_t bytes2 = recvfrom(sockfd, vowels, 1024, MSG_WAITALL,
						(struct sockaddr*)&client, &len);
					if (bytes2 == -1) {
							printf("UDP receive failed.");
							//return -1;
						}

					printf("Got %zd bytes '%s' from client using UDP\n", bytes2, vowels);

					wait(0.5); // make sure that client is ready to receive info 

					// merge nonvowels and vowels into a word, put in a char[]
					char* mergedWord = merge(nonvowels, vowels);
					char mergedTemp[strlen(mergedWord)];
					strcpy(mergedTemp, mergedWord);

					// send merged word back to client through TCP
					ssize_t sendSize = send(connection_fd, mergedTemp, sizeof(mergedTemp), 0);

					if (sendSize == -1) {
						printf("TCP Send failed.\n");
						return -1;
					}

					printf ("Sent %zd bytes '%s' to client using TCP.\n", 
						sendSize, mergedTemp);
					
				// if input == 0, close connections to client
				} else if (input == 0) {
					printf("\nClient has closed connection. Thank you!\n\n");
					close(connection_fd);
					close(sockfd);
					exit(EXIT_SUCCESS);
					return -1;
				}
			}
		}
		else if(pid > 0)//Parent
		{
			close(connection_fd);
		}
		else//Error
		{
			return EXIT_FAILURE;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//    Code that handles splitting and merging below
//

// if SIMPLE is defined, use simple splitting and merging methods
#ifdef SIMPLE
char* split(char* args, bool sendConsonants) {
    char* temp1 = (char*)malloc(strlen(args));
    char* temp2 = (char*)malloc(strlen(args));

	// for every vowel in args, put it in temp 2 string
	// put a space in temp1
    for (int i = 0; i < strlen(args); i++) {
        if (args[i] == 'a' || args[i] == 'e' || args[i] == 'i' || 
            args[i] == 'o' || args[i] == 'u' || args[i] == 'A' ||
			 args[i] == 'E' || args[i] == 'I' || args[i] == 'O' ||
			  args[i] == 'U' ) {

            temp1[i] = ' ';
            temp2[i] = args[i];

		// if a letter is not a vowel, put a space in temp2 and the 
		// char in temp1
        } else {
            temp1[i] = args[i];
            temp2[i] = ' ';
        }
    }

	// if sendConsonants is true, send back the nonvowels
	// otherwise, send the vowels back
	if (sendConsonants) {
    	return temp1;

	} else {
		return temp2;
	}
}

// function to merge vowels and non vowels together in one string
char* merge(char* cons, char* vows) {
    int size = strlen(cons);

    char* temp1 = (char*)malloc(size);

	// if cons[i] has a space, insert vowel into temp1
	// e;se, insert nonvowel into temp1
    for (int i = 0; i < size; i++) {
        if (cons[i] != ' ') {
            temp1[i] = cons[i];

        } else {
            temp1[i] = vows[i];
        }
    }

    return temp1;
}

// if SIMPLE is defined, use advanced splitting and merging methods
#else

// method to split sentences into vowels and non vowels in the advanced case
char* split(char* args, bool sendConsonants) {
    char* temp1 = (char*)malloc(strlen(args));
    char* temp2 = (char*)malloc(strlen(args));

    int temp1Counter = 0;
    int temp2Counter = 0;
    int spaceCounter = 0;

    for (int i = 0; i < strlen(args); i++) {

		// if a vowel is detected, add it to temp2 string
        if (args[i] == 'a' || args[i] == 'e' || args[i] == 'i' || 
            args[i] == 'o' || args[i] == 'u' || args[i] == 'A' || 
			args[i] == 'E' || args[i] == 'I' || args[i] == 'O' || 
			args[i] == 'U') {
                temp2[temp2Counter] = args[i];
                temp2Counter++;

		// if a vowel is not detected add the letter to temp1		
		} else {
            temp1[temp1Counter] = args[i];
            temp1Counter++;
            spaceCounter++;
            if (args[i + 1] == 'a' || args[i + 1] == 'e' || args[i + 1] == 'i' || 
                args[i + 1] == 'o' || args[i + 1] == 'u' || args[i + 1] == 'A' || 
				args[i + 1] == 'E' || args[i + 1] == 'I' || args[i + 1] == 'O' || 
				args[i + 1] == 'U') {
                    char str[2];
                    sprintf(str, "%d", spaceCounter);

                    temp2[temp2Counter] = str[0];
                    temp2Counter++;

                    spaceCounter = 0;
                }
        }
    }

	// if sendConsonants is true, send back the nonvowels
	// otherwise, send the vowels back
	if (sendConsonants)
    	return temp1;
	else 
		return temp2;
}

// method to merge vowels and non vowels into sentences in the advanced case
char* merge(char* cons, char* vows) {
    char* vowelsWithSpaces = (char*)malloc(1024);
    char* consWithSpaces = (char*)malloc(1024);
    int index;

    int size = sizeof(vows);

    // loop to put spaces back into vowels string
    for (int i = 0; i < strlen(vows); i++) {
        if (vows[i] == '1' || vows[i] == '2' || vows[i] == '3' || vows[i] == '4' || 
        vows[i] == '5' || vows[i] == '6' || vows[i] == '7' || vows[i] == '8' || 
        vows[i] == '9') {
            char c = vows[i];
            int x = c - '0';
            
			// if a number is found, add spaces according to number to string
            for (int n = 0; n < x; n++) {
                vowelsWithSpaces[index] = ' ';
                index++;
            }
        } else {
            vowelsWithSpaces[index] = vows[i];
            index++;
        } 
    }


    int index2 = 0;
    int consIndex;

    // loop to put spaces back into nonvowels string
    for (int i = 0; i < strlen(vows); i++) {
        if (vows[i] == '1' || vows[i] == '2' || vows[i] == '3' || vows[i] == '4' || 
        vows[i] == '5' || vows[i] == '6' || vows[i] == '7' || vows[i] == '8' || 
        vows[i] == '9') {
            char c = vows[i];
            int x = c - '0';
            
			// if a number is found, add spaces according to number to string
            for (int n = 0; n < x; n++) {
                consWithSpaces[index2] = cons[consIndex];
                consIndex++;
                index2++;
            }

        } else {
            consWithSpaces[index2] = ' ';
            index2++;
        } 
    }

	// if there are still any non-vowels left in cons, add to string
    for (int i = consIndex; i < strlen(cons); i++) {
        consWithSpaces[index2] = cons[i];
        index2++;
    }

    char vowels[1024];
    char consonants[1024];
    strcpy(consonants, consWithSpaces);

    size = MAX(strlen(consonants), strlen(vowels));

    for (int i = index; i < size; i++) {
        vowelsWithSpaces[i] = ' ';
    }

    strcpy(vowels, vowelsWithSpaces);
    char* temp1 = (char*)malloc(1024);

	// combine vowelsWithSpaces and consWithSpaces into one string.
    for (int i = 0; i < size; i++) {
        if (vowels[i] == ' ') {
            temp1[i] = consonants[i];

        } else {
            temp1[i] = vowels[i];
        }
    }

    return temp1;
}

#endif

// function to make the program wait at a specific line for a certain time
void wait(long seconds) {
	sleep(seconds);
}