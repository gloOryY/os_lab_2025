#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int fd;
  int nread;
  int bufsize;              // ИЗМЕНЕНО: размер буфера как переменная
  char *buf;                // ИЗМЕНЕНО: буфер теперь динамический
  struct sockaddr_in servaddr;

  // ИЗМЕНЕНО: теперь нужно 3 аргумента: IP, порт, bufsize
  if (argc < 4) {
    printf("Usage: %s <IP> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  bufsize = atoi(argv[3]);  // ИЗМЕНЕНО: читаем размер буфера из argv[3]
  if (bufsize <= 0) {
    fprintf(stderr, "Incorrect bufsize\n");
    exit(1);
  }

  buf = malloc(bufsize);    // ИЗМЕНЕНО: выделяем память под буфер
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    free(buf);              // ИЗМЕНЕНО: освобождаем память перед выходом
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("bad address");
    free(buf);
    exit(1);
  }

  servaddr.sin_port = htons(atoi(argv[2]));

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    free(buf);
    exit(1);
  }

  write(1, "Input message to send\n", 22);

  // ИЗМЕНЕНО: используем bufsize вместо BUFSIZE
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      free(buf);
      exit(1);
    }
  }

  close(fd);
  free(buf);                // ИЗМЕНЕНО: освобождаем буфер
  exit(0);
}
