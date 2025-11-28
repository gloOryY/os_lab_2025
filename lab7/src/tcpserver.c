#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  const size_t kSize = sizeof(struct sockaddr_in);

  int lfd, cfd;
  int nread;
  int bufsize;                // ИЗМЕНЕНО: буфер как параметр
  int port;                   // ИЗМЕНЕНО: порт как параметр
  char *buf;                  // ИЗМЕНЕНО: динамический буфер
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  // ИЗМЕНЕНО: порт и размер буфера из аргументов
  if (argc < 3) {
    printf("Usage: %s <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);       // ИЗМЕНЕНО
  bufsize = atoi(argv[2]);    // ИЗМЕНЕНО

  if (port <= 0 || bufsize <= 0) {
    fprintf(stderr, "Incorrect port or bufsize\n");
    exit(1);
  }

  buf = malloc(bufsize);      // ИЗМЕНЕНО
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    free(buf);                // ИЗМЕНЕНО
    exit(1);
  }

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);   // ИЗМЕНЕНО: порт из аргумента

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    free(buf);
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    free(buf);
    exit(1);
  }

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      free(buf);
      exit(1);
    }
    printf("connection established\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {  // ИЗМЕНЕНО
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
      close(cfd);
      free(buf);
      exit(1);
    }
    close(cfd);
  }

  free(buf);                  // теоретически недостижимо, но правильно
}
