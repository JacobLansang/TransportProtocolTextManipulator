///////////////////////////////////////////////////////////////////////////
//
//
//  Some of this code was taken from William Black (william.black@ucalgary.ca): 
//  https://uofc-my.sharepoint.com/personal/william_black_ucalgary_ca/_layouts/15/onedrive.aspx?id=%2Fpersonal%2Fwilliam%5Fblack%5Fucalgary%5Fca%2FDocuments%2FCPSC441%2FW2022%2FTutorials%2FTutorial%2D07%2DUDP%2FUDP%2DClient%2DServer%2Fudpdemo%2Dclient%2Ec&parent=%2Fpersonal%2Fwilliam%5Fblack%5Fucalgary%5Fca%2FDocuments%2FCPSC441%2FW2022%2FTutorials%2FTutorial%2D07%2DUDP%2FUDP%2DClient%2DServer
//
///////////////////////////////////////////////////////////////////////////

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>

int PORT_NUM;

// prints out available commands into terminal
void printCommands() {
  printf("Please choose from the following:\n");
  printf("  1 - Devowel a message\n");
  printf("  2 - Envowel a message\n");
  printf("  0 - Exit program\n");
  printf("Your desired menu selection? ");
}

// function to make the program wait at a specific line for a certain time
void wait(long seconds) {
	sleep(seconds);
}

int main(int argc,char *argv[])
{
	PORT_NUM = atoi(argv[1]); // user can enter a desired port number to be used

  printf("Welcome! I am a client for the vowelizer demo!\n\n");

  // construct TCP socket
  int connection_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(connection_fd == -1) { perror("Failed to create socket"); return 1;}

  // construct and initialize UDP socket
  ssize_t sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sockfd == -1) { perror("Failed to create socket file descriptor."); exit(EXIT_FAILURE); };

  // construct sockaddr_in and fill in PORT_NUM and IP address.
	struct sockaddr_in connection_sa;
	memset(&connection_sa,0,sizeof(struct sockaddr_in));
	connection_sa.sin_family = AF_INET;
	connection_sa.sin_port = htons(PORT_NUM);
	connection_sa.sin_addr.s_addr = inet_addr("136.159.5.27");

  // TCP handshake with server
  connect(connection_fd,(struct sockaddr*)&connection_sa,
			      sizeof(struct sockaddr_in));
	if(connection_fd == -1) { perror("Failed to connect"); return 1;}

  wait(0.5); // make sure to wait for server to recvfrom before we call sendto

  // send a null string to server to initialize a connection through UDP
  while ((sendto(sockfd, "", strlen(""), MSG_CONFIRM,
        (struct sockaddr*)&connection_sa, sizeof(struct sockaddr_in))) == -1) {
          printf("UDP send failed.");
        }
 

  printf("Connection established! \n");

  // program never leaves this loop until client exits
  while(1) {
    printCommands(); // prints available server commands

    int input = 0;
    scanf("%d", &input); // read integer input that corresponds to command

    // if input == 1, split word
    if (input == 1) {

      // sends the result of the input to the server to get ready to split word
      send(connection_fd, &input, sizeof(input), 0);
      input = 0;

      char str[1024] = {'\0'}; // stores string input
      char conson[1024] = {'\0'}; // stores split output of consonants
      char vowels[1024] = {'\0'}; // store split output of vowels

      while ((getchar()) != '\n'); // gets rid of '\n' from scanf

      printf("Enter your message to devowel: \n");
      fgets(str,sizeof(str),stdin); // receives sentence input to split
			str[strcspn(str, "\n")] = 0; // remove newline at end of str

      printf("You have entered: '%s'\n", str);

      // send the input sentence to be split to server
      send(connection_fd, str, sizeof(str), 0);
      //printf("MESSAGE SENT! \n");
      ssize_t bytes = 0;
      ssize_t bytes2 = 0;

      // timeval waits 500 us for non vowels to be received through TCP
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 500;
      setsockopt(connection_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
      bytes = recv(connection_fd, conson, 1024, 0);
      printf("Server sent %zd bytes of non-vowels on TCP: '%s'\n", bytes, conson);
      //wait(0.5);

      // tv2 waits for receiving vowels through UDP
      struct timeval tv2;
      tv2.tv_sec = 5;
      tv2.tv_usec = 500;
      setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv2,sizeof(struct timeval));

      socklen_t lengthOfServer = sizeof(connection_sa);      

      // try and receive vowels through UDP
      bytes2 = recvfrom(sockfd, vowels, 1024, MSG_WAITALL,
           (struct sockaddr*)&connection_sa, &lengthOfServer);

      printf("Server sent %zd bytes of vowels using  UDP: '%s'\n\n", bytes2, vowels);

    // if input == 2, merge vowels and non vowels
    } else if (input == 2) {

      // send result of input to server
      send(connection_fd, &input, sizeof(input), 0); 
      input = 0;

      char nonvowelStr[1024] = {'\0'}; // stores non vowel input
      char vowelStr[1024] = {'\0'}; // stores vowel input
      while ((getchar()) != '\n'); // gets rid of '\n' from scanf

      printf("Enter non-vowel part of message to envowel: ");
      fgets(nonvowelStr, sizeof(nonvowelStr), stdin); // receive input from terminal
      nonvowelStr[strcspn(nonvowelStr, "\n")] = 0;

      char* nonvowel = nonvowelStr;
      // printf("This is the size of non vowel: %ld\n\n", sizeof(nonvowel));
      // printf("This is the size of non vowel: %s\n\n", nonvowel);

      // receives vowel input and puts it in string
      printf("Enter the vowel part of message to envowel: ");
      fgets(vowelStr, sizeof(vowelStr), stdin);
      vowelStr[strcspn(vowelStr, "\n")] = 0;

      char* vowel = vowelStr;

      // send nonvowels through TCP
      send(connection_fd, nonvowel, strlen(nonvowel), 0);

      // wait for server to be able to recvfrom
      wait(0.5);

      // send vowels through UDP
      if ((sendto(sockfd, vowel, strlen(vowel), MSG_CONFIRM,
        (struct sockaddr*)&connection_sa, sizeof(struct sockaddr_in))) == -1) {
          printf("UDP Send failed.\n");
        }

      // wait 2 seconds to receive all bytes through UDP
      struct timeval tv;
      tv.tv_sec = 2;
      tv.tv_usec = 0;
      setsockopt(connection_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));        
      char buffer[1024] = {'\0'};
      ssize_t bytes = recv(connection_fd, buffer, 1024, 0);

      printf("Server sent %zd bytes of message using TCP: '%s'\n\n", bytes, buffer);

    // if input == 0, close connections and terminate client program
    } else if (input == 0) {
      printf("\nQuitting client program. Thank you! \n\n");
      close(connection_fd);
      close(sockfd);
      return 0;

    // if input != 1, 2, 0, ask for input again
    } else {
      printf("Invalid selection.\n\n");
    }
  }

    return 0;
}