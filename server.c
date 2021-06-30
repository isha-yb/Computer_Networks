#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <asm/unistd.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

void *mythread(void *param);
int sockfd, sockfd1, sockfd2;
struct sockaddr_in server_addr, new_addr;
socklen_t addr_size;
int N=0;
int c1, c2;


// CLIENT 1

void func1(int fd){
  char buffer[1024];
  while(1){
    bzero(buffer, sizeof(buffer));
    read(fd, buffer, sizeof(buffer));
    write(c2, buffer, sizeof(buffer));  
  }
}


// CLIENT 2

void func2(int fd){
  char buffer[1024];
  while(1){
    bzero(buffer, sizeof(buffer));
    read(fd, buffer, sizeof(buffer));
    write(c1, buffer, sizeof(buffer));  
  }
}

void *mythread(void *param){
  int addr_size = sizeof(new_addr);
  int new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
  if(N==0){
    N++;
  	c1 = new_sock;
  	func1(new_sock);
  }
  else{
  	c2 = new_sock;
  	func2(new_sock);
  }
  pthread_exit(0);
}


int main(int argc, char *argv[]){

  // SERVER INPUT

  if(argc != 2){
    printf("[-]Incorrect input\n");
    return 0;
  }
  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("[-]Error in creating socket");
    exit(1);
  }
  printf("[+]Server socket created successfully\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  // BINDING SOCKET

  int e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if(e < 0) {
    perror("[-]Error in binding socket");
    exit(1);
  }
  printf("[+]Binding successfull\n");


  // LISTENING

  if(listen(sockfd, 10) == 0){
  printf("[+]Listening....\n");
  }else{
  perror("[-]Error in listening");
    exit(1);
  }
  
  // CREATING THREADS FOR EACH CLIENT

  pthread_t tid1, tid2;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&tid1, &attr, mythread, NULL);
  pthread_create(&tid2, &attr, mythread, NULL);
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  close(sockfd);
  return 0;
}

