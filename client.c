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
#include <time.h>
#include <sys/time.h>
#include <errno.h>

void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{
  int sockfd, numbytes = 1;
  char buf[4096], fileName[256];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  FILE * fp;
  FILE * fpt;
  struct timespec start, end;
  time_t sec;
  //char portno[6];
  //char sServ[INET6_ADDRSTRLEN];

  if (argc < 2){
      printf("Please enter the hostname of the server you want to connect to.\n");
      exit(1);
  }

  if(argc < 3){
    printf("Please enter the port you would like to connect to.\n");
    exit(1);
  }

  if(argc < 4){
    printf("Please enter the name of the file you would like to retrieve\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo))!= 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  //loop through results and connect to the first possible.
  for(p = servinfo; p!= NULL; p = p->ai_next){
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
      perror("client: socket");
      continue;
    }
    if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      perror("client: connect");
      continue;
    }
    break;
  }
  if (p == NULL){
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  freeaddrinfo(servinfo); //done with server info struct.

  if((fp = fopen(argv[3], "wb")) == NULL){
    printf("There was an error opening the file.: %s\n", strerror(errno));
    close(sockfd);
    exit(1);
  }

  recv(sockfd, buf, sizeof buf, 0);
  printf("%s\n", buf);
  send(sockfd, "Get", 4, 0);
  if(clock_gettime(CLOCK_MONOTONIC, &start) !=0){
    printf("Start time failed\n");
    exit(1);
  };
  while(1){
    numbytes = recv(sockfd, buf, sizeof(buf), 0);
    if (numbytes == 0){
      printf("File received\n");
      if(clock_gettime(CLOCK_MONOTONIC, &end) !=0)
      {
        printf("end time failed\n");
        exit(1);
      }
      sec = end.tv_nsec - start.tv_nsec;
      fpt = fopen("progTime.txt", "a+");
      fprintf(fpt, "%i\n", sec);
      exit(0);
    }
    //buf[numbytes] = '\0';
    if (numbytes < 0){
      printf("Error receiving file");
      exit(1);
    }
    fwrite(buf, numbytes, 1, fp);
    printf("Recieved %d bytes\n", numbytes);
  }


  //printf("client: received '%s'\n", buf);
  fclose(fp);
  close(sockfd);


  return 0;
}
