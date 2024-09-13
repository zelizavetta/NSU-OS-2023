#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

void handle_sigint(int sig) {
  const char *msg = "\nClient disconnected.\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
  _exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <proxy_ip> <proxy_port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *proxy_ip = argv[1];
  int proxy_port = atoi(argv[2]);
  int sock_fd;
  struct sockaddr_in proxy_addr;
  char buffer[BUFFER_SIZE];

  // signal(SIGINT, handle_sigint);

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&proxy_addr, 0, sizeof(proxy_addr));
  proxy_addr.sin_family = AF_INET;
  proxy_addr.sin_port = htons(proxy_port);
  if (inet_pton(AF_INET, proxy_ip, &proxy_addr.sin_addr) <= 0) {
    perror("inet_pton");
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  if (connect(sock_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) <
      0) {
    perror("connect");
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  printf("Connected to proxy server at %s:%d\n", proxy_ip, proxy_port);

  while (1) {
    printf("Enter message (Ctrl+D to quit): ");
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
      printf("\nClient disconnected.\n");
      break;
    }

    if (send(sock_fd, buffer, strlen(buffer), 0) < 0) {
      perror("send");
      break;
    }

    int bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
      perror("recv");
      break;
    } else if (bytes_received == 0) {
      printf("Server disconnected.\n");
      break;
    }

    buffer[bytes_received] = '\0';
    printf("Response from server: %s\n", buffer);
  }

  close(sock_fd);
  return 0;
}
