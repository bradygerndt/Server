/*Project 1 - Event driven server.

Please include input file and port number as arguments
The socket set up portion of this program was modeled after Beej's guide for network sockets

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/*linked list for managing client file transfer*/
struct node
{
   size_t bytes;//amount read to client.
   int key;
   struct node *next;
};
struct node *head = NULL;
struct node *current = NULL;

void insertFirst(int key, size_t bytes)
{
  struct node *link = (struct node*) malloc(sizeof(struct node));

  link->key = key;
  link->bytes = bytes;

  link->next = head;

  head = link;
}

struct node* delete(int key){
  struct node* current = head;
  struct node* previous = NULL;

  if(head == NULL)
  {
    return NULL;
  }

  while(current->key != key)
  {
    if(current->next == NULL){
      return NULL;
    }
  else
  {
    previous = current;
    current = current->next;
  }

  }
  if(current == head)
  {
    head = head->next;
  }
  else
  {
    previous->next = current->next;
  }
  return current;
}

struct node* find(int key){

   //start from the first link
   struct node* current = head;

   //if list is empty
   if(head == NULL)
	{
      return NULL;
   }

   //navigate through list
   while(current->key != key){

      //if it is last node
      if(current->next == NULL){
         return NULL;
      }else {
         //go to next link
         current = current->next;
      }
   }

   //if data found, return the current Link
   return current;
}
/*linked list for managing client file transfer*/
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

int checkConnection(int fd, fd_set *master_read)
{
  char buf[512];
  size_t bytesRecv;


  if ((bytesRecv = recv(fd, buf, sizeof buf, 0))<=0)
  {
    //error or connection refused
    if(bytesRecv == 0)
    {
      printf("server: socket %d hung up\n", fd);
    }
    else
    {
      perror("recv");
    }
    delete(fd);
    close(fd);
    FD_CLR(fd, master_read);
  }
}

//figure out whether we need ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[]) {
  int sockfd, new_fd, retVal, big_fd, i, sr;
  int listExist = 0;
  char portno[6];
  char buf[512], fileName[100], cliMsg[512], s[INET6_ADDRSTRLEN];
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage cliAddr; //client's address info.
  struct node *clientLink;
  socklen_t sockin_size;
  int optval = 1;
  FILE * fp;
  size_t bytesRead, bytesSent, bytesRecv;
  long byterec[200];
  fd_set master_read;
  fd_set read_fds;
  struct timeval t;
//set time value
  t.tv_sec = 2;


/* check for filename as command line argument*/
  if (argc < 2) {
    printf("Please provide the name of an input file!!\n");
    exit(1);
  }

  if (argc < 3) {
    printf("Please specify a port number\n");
    exit(1);
  }

  strcpy(fileName, argv[1]);
  strcpy(portno, argv[2]);

  FD_ZERO(&master_read);
  FD_ZERO(&read_fds);


  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //passive sets to my ip.

  if ((retVal = getaddrinfo(NULL, portno, &hints, &servinfo))!=0)
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



  //adding listener to master_read set
  FD_SET(sockfd, &master_read);
  //set max fd in list
  big_fd = sockfd;

  while (1)
  {
    read_fds = master_read;
    if(select(big_fd+1, &read_fds, NULL, NULL, &t) == -1)
    {
      printf("in the place\n");
      perror("select");
      exit(4);
    }
    //look through existing connections
    for(i = 0; i<= big_fd; i++)
    {

      if(FD_ISSET(i, &read_fds))
      {
        if(i == sockfd)
        {
          sockin_size = sizeof cliAddr;
          new_fd = accept(sockfd, (struct sockaddr *) &cliAddr, &sockin_size);
          if (new_fd == -1)
            {
            perror("accept");
            }
          else
            {
              FD_SET(new_fd, &master_read);
              if(new_fd > big_fd)
              {
                big_fd = new_fd;//add new big value.
              }
              //announce new connection
              inet_ntop(cliAddr.ss_family, get_in_addr((struct sockaddr *)&cliAddr),
                s, INET6_ADDRSTRLEN);
              printf("server: Got connection from %s " "socket: %d\n", s, new_fd);
              strcpy(buf, "Please enter \"Get\" to retrieve file on server.\n");
              send(new_fd, buf, strlen(buf), 0);
            }
          }
          else
          {
            memset(buf, 0, sizeof buf);
            //get data from the client
            checkConnection(i, &master_read);
            if(checkGet(buf) == 1)
            {
              memset(buf, 0, sizeof buf);
              // strcpy(buf, "The file is on the way!\n");
              // send(i, buf, strlen(buf), 0);
              insertFirst(i, 0);
              listExist++;
              //open file for reading.
              if((fp = fopen(fileName, "rb"))==NULL)
              {
                printf("There was an error with the file on the server: %s\n", strerror(errno));
                exit(1);
              }
              memset(buf, 0, sizeof buf);
              bytesRead = fread(buf, 1, 4096, fp);
              if (bytesRead < 0)
              {
                printf("There was an error reading the file: %s\n", strerror(errno));
              }
              send(i, buf, bytesRead, 0);

              printf("There were %i bytes read to %s\n", bytesRead, s);
              clientLink = find(i);
              clientLink->bytes = bytesRead;
              if(bytesRead <= sizeof buf)
              {
                //printf("Smaller than buffer\n");
                listExist--;
                delete(i);
                close(i);
                FD_CLR(i, &master_read);
              }
              //close the file
              fclose(fp);
            }
            else
            {
              memset(buf, 0, sizeof buf);
              strcpy(buf, "Please enter \"Get\" to retrieve file on server.\n");
              checkConnection(i, &master_read);
              bytesSent = send(i, buf, strlen(buf), 0);
              continue;
            }



          }
        }
            if(listExist > 0 && i > 3)
            {
              if((fp = fopen(fileName, "rb"))==NULL)
              {
                printf("There was an error with the file on the server: %s\n", strerror(errno));
                exit(1);
              }
              fseek(fp, find(i)->bytes, SEEK_SET);
              memset(buf, 0, sizeof buf);
              bytesRead = fread(buf, 1, 4096, fp);
              if (bytesRead < 0)
              {
                printf("There was an error reading the file: %s\n", strerror(errno));
              }
              if(bytesRead == 0)
              {
                //printf("Deleting from linked list\n");
                send(i, buf, bytesRead, 0);
                listExist--;
                delete(i);
                close(i);
                FD_CLR(i, &master_read);
                fclose(fp);
              }
              else
              {
              //printf("Still sending\n");
              send(i, buf, bytesRead, 0);
              printf("There were %i bytes read to %s\n", bytesRead, s);
              find(i)->bytes += bytesRead;
              fclose(fp);
              }
            }

      }
    }

  return 0;
}
