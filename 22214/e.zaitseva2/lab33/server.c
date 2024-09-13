#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 510

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int port = atoi(argv[1]);
  int server_fd, client_fd, max_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  fd_set read_fds, active_fds;
  char buffer[BUFFER_SIZE];

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, MAX_CONNECTIONS) < 0) {
    perror("listen");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  FD_ZERO(&active_fds);
  FD_SET(server_fd, &active_fds);
  max_fd = server_fd;

  printf("Server listening on port %d\n", port);

  while (1) {
    read_fds = active_fds;

    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
      perror("select");
      break;
    }

    if (FD_ISSET(server_fd, &read_fds)) {
      client_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_fd < 0) {
        perror("accept");
        continue;
      }

      FD_SET(client_fd, &active_fds);
      if (client_fd > max_fd) max_fd = client_fd;

      printf("New connection established. Client FD: %d\n", client_fd);
    }

    int i;
    for (i = server_fd + 1; i <= max_fd; i++) {
      if (FD_ISSET(i, &read_fds)) {
        int bytes_read = recv(i, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
          if (bytes_read == 0 || (bytes_read < 0 && errno == ECONNRESET)) {
            printf("Connection closed by client FD: %d\n", i);
          } else {
            perror("recv");
          }
          close(i);
          FD_CLR(i, &active_fds);
        } else {
          buffer[bytes_read] = '\0';
          printf("Received from client FD %d: %s", i, buffer);

          if (send(i, buffer, bytes_read, 0) < 0) {
            perror("send");
            close(i);
            FD_CLR(i, &active_fds);
          }
        }
      }
    }
  }

  close(server_fd);
  return 0;
}
