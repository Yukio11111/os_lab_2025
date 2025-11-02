#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 10050
#define BUFSIZE 100
#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main() {
  int sockfd, cfd;
  int nread;
  char buf[BUFSIZE];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(sockfd, 5) < 0) {
    perror("listen");
    exit(1);
  }

  printf("Server listening on port %d\n", SERV_PORT);

  while (1) {
    unsigned int clilen = SLEN;
    if ((cfd = accept(sockfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      continue;
    }

    printf("Connection established\n");

    while ((nread = read(cfd, buf, BUFSIZE)) > 0) {
      printf("Received: ");
      write(1, buf, nread);
      // Добавлено: Эхо обратно клиенту
      if (write(cfd, buf, nread) < 0) {
        perror("write to client");
        break;
      }
    }

    close(cfd);
  }

  close(sockfd);
  return 0;
}