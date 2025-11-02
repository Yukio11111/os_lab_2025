#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 100
#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int fd;
  int nread;
  char buf[BUFSIZE];
  struct sockaddr_in servaddr;

  if (argc != 3) {
    printf("Too few arguments. Usage: %s <IP> <PORT>\n", argv[0]);
    exit(1);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }

  printf("Input message to send\n");
  while ((nread = read(0, buf, BUFSIZE)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
    // Добавлено: Чтение ответа от сервера (эхо)
    nread = read(fd, buf, BUFSIZE);
    if (nread < 0) {
      perror("read from server");
      exit(1);
    } else if (nread == 0) {
      break;
    }
    printf("REPLY FROM SERVER: ");
    write(1, buf, nread);
  }

  close(fd);
  exit(0);
}