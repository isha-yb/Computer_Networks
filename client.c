#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

char *pub_key;
char *priv_key;
int sockfd;
pthread_t tid1, tid2;


void *encryptf(void *param){

  char plain[1024];

  while(1){
    bzero(plain, sizeof(plain));
    int i=0;
    while((plain[i++] = getchar()) != '\n');
    plain[i-1] = 0;
    
    // USE PUBLIC KEY TO ENCRYPT PLAIN TEXT

    char encrypted[1024];

    // OPEN PUBLIC KEY FILE

    FILE* pub_fp=fopen(pub_key,"r");
    if(pub_fp==NULL){
      printf("failed to open pub_key file %s!\n", pub_key);
      break;
    }

    // READ PUBLIC KEY FROM FILE

    RSA* rsa1=PEM_read_RSA_PUBKEY(pub_fp, NULL, NULL, NULL);
    if(rsa1==NULL){
      printf("unable to read public key!\n");
      break;
    }

    if(strlen(plain)>=RSA_size(rsa1)){
      printf("failed to encrypt\n");
      break;
    }
    fclose(pub_fp);

    // USE PUBLIC KEY TO ENCRYPT

    int encrylen=RSA_public_encrypt(i-1, plain, encrypted, rsa1, RSA_PKCS1_PADDING);
    if(encrylen==-1 ){
      printf("failed to encrypt\n");
      break;
    }

    // SEND ENCRYPTED TEXT TO SERVER

    write(sockfd, encrypted, sizeof(encrypted));
    
    // CHECK IF CLIENT HAS EXITED

    if(strcmp(plain, "exit") == 0){
      break;
    }

    bzero(plain, sizeof(plain));

  }
  pthread_cancel(tid2);
  pthread_exit(0);
}

void *decryptf(void *param){

  while(1){
    
    // USE PRIVATE KEY TO DECRYPT
    // OPEN PRIVATE KEY FILE

    FILE* priv_fp=fopen(priv_key,"r");
    if(priv_fp==NULL){
      printf("failed to open priv_key file %s!\n", priv_key);
      break;
    }

    // READ PRIVATE KEY FROM FILE

    RSA *rsa2 = PEM_read_RSAPrivateKey(priv_fp, NULL, NULL, NULL);
    if(rsa2==NULL){
      printf("unable to read private key!\n");
      break;
    }

    // READ ENCRYPTED TEXT RECEIVED FROM SERVER

    int encrylen = RSA_size(rsa2);
    char decrypted[1024];
    char encrypted[1024];
    for(int i=0; i<1024; i++){
      decrypted[i] = '\0';
    }
    read(sockfd, encrypted, encrylen);
  
    // USE PRIVATE KEY TO DECRYPT

    int decrylen=RSA_private_decrypt(encrylen, encrypted, decrypted, rsa2, RSA_PKCS1_PADDING);
    if(decrylen==-1){
      printf("failed to decrypt!\n");
      break;
    }
    fclose(priv_fp);
    if(strcmp("exit", decrypted) == 0){
      break;
    }
    printf("THE ENCRYPTED STRING RECEIVED IS: %s\n", encrypted);
    printf("THE DECRYPTED STRING RECEIVED IS: %s\n", decrypted);
  }
  pthread_cancel(tid1);
  pthread_exit(0);
}


int main(int argc, char *argv[]){

  // COMMAND LINE INPUT: IP, PORT, filename priv key, filename pub key

  if(argc != 5){
  	printf("[-]Incorrect input\n");
  	return 0;
  }

  char *ip = (char *)malloc(strlen(argv[1])*sizeof(char));
  strcpy(ip, argv[1]);

  int port = atoi(argv[2]);

  priv_key = (char *)malloc(strlen(argv[3])*sizeof(char));
  strcpy(priv_key, argv[3]);
    
  pub_key = (char *)malloc(strlen(argv[4])*sizeof(char));
  strcpy(pub_key, argv[4]);

  // CREATING CLIENT SOCKET

  struct sockaddr_in client_addr;
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("[-]Error in creating socket\n");
    exit(1);
  }
  printf("[+]Client socket created successfully.\n");

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = inet_addr(ip);

  // CONNECT SOCKET

  int e = connect(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
  if(e == -1) {
    perror("[-]Server is unreachable. Client exiting ... \n");
    exit(1);
  }
  printf("[+]Connected to Server\n");

  // ENCRYPT, DECRYPT THREADS 
  
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&tid1, &attr, encryptf, NULL);
  pthread_create(&tid2, &attr, decryptf, NULL);
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  // CLOSE SOCKET

  close(sockfd);
  printf("[+]Closing the connection.\n");

  return 0;
}