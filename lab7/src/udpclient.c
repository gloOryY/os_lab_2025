#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int sockfd, n;
  int port;                       // ИЗМЕНЕНО: порт как параметр
  int bufsize;                    // ИЗМЕНЕНО: размер буфера как параметр
  char *sendline;                 // ИЗМЕНЕНО: динамический буфер
  char *recvline;                 // ИЗМЕНЕНО: динамический буфер
  struct sockaddr_in servaddr;
  // struct sockaddr_in cliaddr;  // ИЗМЕНЕНО: УДАЛЕНО, переменная не используется

  // ИЗМЕНЕНО: IP, порт, bufsize
  if (argc != 4) {
    printf("usage: %s <IPaddress of server> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[2]);           // ИЗМЕНЕНО
  bufsize = atoi(argv[3]);        // ИЗМЕНЕНО

  if (port <= 0 || bufsize <= 0) {
    fprintf(stderr, "Incorrect port or bufsize\n");
    exit(1);
  }

  sendline = malloc(bufsize);     // ИЗМЕНЕНО
  recvline = malloc(bufsize + 1); // ИЗМЕНЕНО
  if (!sendline || !recvline) {
    perror("malloc");
    free(sendline);
    free(recvline);
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);     // ИЗМЕНЕНО

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    free(sendline);
    free(recvline);
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    free(sendline);
    free(recvline);
    exit(1);
  }

  write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {   // ИЗМЕНЕНО
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      free(sendline);
      free(recvline);
      close(sockfd);
      exit(1);
    }

    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      free(sendline);
      free(recvline);
      close(sockfd);
      exit(1);
    }

    recvline[bufsize] = 0;        // ИЗМЕНЕНО: чтобы была строка
    printf("REPLY FROM SERVER= %s\n", recvline);
  }

  free(sendline);
  free(recvline);
  close(sockfd);
}
