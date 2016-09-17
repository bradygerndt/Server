/*Project 1 - Threaded server.

Please include input file and port number as arguments
The socket set up portion of this program was modeled after Beej's guide for network sockets

*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>


//figure out whether we need ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int checkGet(char buf[])
{
  char testChar[4] = "Get";

    for(int c = 0; c < 3; c++)
    {
      if(testChar[c] != buf[c])
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
}


int main(int argc, char const *argv[]) {
  int sockfd, new_fd, retVal, threadRet;
  char portno[6];
  char * cliMsg[10], s[INET6_ADDRSTRLEN];//fileName[100], buf[512] was included in the tread function.
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage cliAddr; //client's address info.
  socklen_t sockin_size;
  int optval = 1;
  pthread_t thread;
  pthread_attr_t det;
  //FILE * fp; in thread function.
  //size_t bytesRead; included in the thread function.

/* check for filename as command line argument*/
  if (argc < 2) {
    printf("Please provide the name of an input file!!\n");
    exit(1);
  }

  if (argc < 3){
    printf("Please specify a port number.\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //passive sets to my ip.

  if ((retVal = getaddrinfo(NULL, argv[2], &hints, &servinfo))!=0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
    return 1;
  }
  //loop through structs returned by get address info.
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    //create the socket
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
    {
      perror("server: socket");
      continue;
    }
    //configure socket to be reusable
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) //Other sockets can bind unless active listening socket. Deals with address in use errors resulting from a crashed server.
    {
      perror("setsockopt");
      exit(1);
    }
    //bind and check success
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }
      break;
  }

freeaddrinfo(servinfo); //free up memory used by servinfo structure

if (p == NULL) {
  fprintf(stderr, "server: failed to bind\n");
  exit(1);
}

if (listen(sockfd, 10) == -1)
{
  perror("listen");
  exit(1);
}

printf("server: waiting for connections...\n");

//function for thread to use.
void *clientThread(void *ptr){
  FILE * fp;
  size_t bytesRead;
  char buf[4096],fileName[100], msg[100];
  memset(buf, 0, sizeof buf);
  strcpy(fileName, argv[1]);
  int numbytes;

  strcpy(msg, "Please enter \"Get\" to retrieve file on server.\n");

  /*open file on server*/
  send(ptr, msg, strlen(msg), 0);
  memset(msg, 0, sizeof msg);
  numbytes = recv(ptr, msg, sizeof(msg) - 1, 0);
  while(1){
  if(checkGet(msg) == 1)
  {
  if((fp = fopen(fileName, "rb"))==NULL)
  {
    printf("There was an error with the file on the server: %s\n", strerror(errno));
    exit(1);
  }
  printf("File successfully opened.\n");
  /*send the file to the client*/
    while (1) {
    //fseek(fp, 0, SEEK_SET);
    bytesRead = fread(buf, 1, 4096, fp);
    printf("Bytes have been read\n");
    if (bytesRead == 0){
        send(ptr, buf, bytesRead, 0);
        printf("%i\n", bytesRead);
        close(ptr);
        break;
      }
    if (bytesRead < 0){
      printf("There was an error reading the file: %s\n", strerror(errno));
    }
    //printf("End of loop\n");
    printf("%i\n", bytesRead);
    //printf("%x\n", buf);
    send(ptr, buf, bytesRead, 0);
  }
  fclose(fp);
  pthread_exit(NULL);
  return 0;
}
else
{
    send(ptr, msg, strlen(msg), 0);
}
}
}

//accept incoming connections
while(1)
{
  sockin_size = sizeof cliAddr;
  new_fd = accept(sockfd, (struct sockaddr *) &cliAddr, &sockin_size);
  if (new_fd == -1)
    {
    perror("accept");
    continue;
    }
    //print out ip of connecting client
    inet_ntop(cliAddr.ss_family, get_in_addr((struct sockaddr *)&cliAddr),
      s, INET6_ADDRSTRLEN);

    printf("server: Got connection from %s\n", s);
    pthread_attr_init(&det);
    pthread_attr_setdetachstate(&det, PTHREAD_CREATE_DETACHED);
    threadRet = pthread_create(&thread, &det, clientThread, (void *)new_fd);
    if(threadRet)
    {
      printf("Error no. on thread creation is %d\n", threadRet);
      exit(-1);
    }
    pthread_attr_destroy(&det);

    //clientThread(new_fd);
    //send(new_fd, "Enter \"Get\" to receive a file from the server.\n", 50, 0);
    //int testStuff = recv(new_fd, buf, sizeof buf, 0);
    //buf[testStuff] = '\n';
    //strcpy(cliMsg, buf);
    //printf("%s",cliMsg);
}



  return 0;
}
