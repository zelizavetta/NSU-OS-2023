#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 510

typedef struct {
  int client_fd;
  int target_fd;
} Connection;

int connect_to_target(const char *host, int port) {
  int target_fd;
  struct sockaddr_in target_addr;

  if ((target_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  memset(&target_addr, 0, sizeof(target_addr));
  target_addr.sin_family = AF_INET;
  target_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host, &target_addr.sin_addr) <= 0) {
    perror("inet_pton");
    close(target_fd);
    return -1;
  }

  if (connect(target_fd, (struct sockaddr *)&target_addr, sizeof(target_addr)) <
      0) {
    perror("connect");
    close(target_fd);
    return -1;
  }

  return target_fd;
}

void handle_data_transfer(int source_fd, int dest_fd, fd_set *read_fds,
                          fd_set *write_fds) {
  char buffer[BUFFER_SIZE];
  int bytes_read = recv(source_fd, buffer, sizeof(buffer), 0);
  if (bytes_read <= 0) {
    if (bytes_read == 0 || (bytes_read < 0 && errno == ECONNRESET)) {
      printf("Connection closed by client\n");
    } else {
      perror("recv");
    }
    close(source_fd);
    close(dest_fd);
    FD_CLR(source_fd, read_fds);
    FD_CLR(dest_fd, read_fds);
    FD_CLR(source_fd, write_fds);
    FD_CLR(dest_fd, write_fds);
  } else {
    if (send(dest_fd, buffer, bytes_read, 0) < 0) {
      perror("send");
      close(source_fd);
      close(dest_fd);
      FD_CLR(source_fd, read_fds);
      FD_CLR(dest_fd, read_fds);
      FD_CLR(source_fd, write_fds);
      FD_CLR(dest_fd, write_fds);
    }
  }
}

void remove_connection(Connection connections[], int *conn_count, int index) {
  close(connections[index].client_fd);
  close(connections[index].target_fd);
  int i;
  for (i = index; i < *conn_count - 1; i++) {
    connections[i] = connections[i + 1];
  }

  (*conn_count)--;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <proxy_port> <server_ip> <server_port>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  int proxy_port = atoi(argv[1]);
  char *server_ip = argv[2];
  int server_port = atoi(argv[3]);
  int proxy_fd, max_fd;
  struct sockaddr_in proxy_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  fd_set read_fds, write_fds, active_fds;
  Connection connections[MAX_CONNECTIONS];
  int conn_count = 0;

  if ((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&proxy_addr, 0, sizeof(proxy_addr));
  proxy_addr.sin_family = AF_INET;
  proxy_addr.sin_addr.s_addr = INADDR_ANY;
  proxy_addr.sin_port = htons(proxy_port);

  if (bind(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
    perror("bind");
    close(proxy_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(proxy_fd, MAX_CONNECTIONS) < 0) {
    perror("listen");
    close(proxy_fd);
    exit(EXIT_FAILURE);
  }

  FD_ZERO(&active_fds);
  FD_SET(proxy_fd, &active_fds);
  max_fd = proxy_fd;

  printf("Proxy listening on port %d\n", proxy_port);

  while (1) {
    read_fds = active_fds;
    write_fds = active_fds;

    if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
      perror("select");
      break;
    }

    if (FD_ISSET(proxy_fd, &read_fds)) {
      int client_fd =
          accept(proxy_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_fd >= 0) {
        if (conn_count < MAX_CONNECTIONS) {
          int target_fd = connect_to_target(server_ip, server_port);
          if (target_fd >= 0) {
            connections[conn_count].client_fd = client_fd;
            connections[conn_count].target_fd = target_fd;
            conn_count++;

            FD_SET(client_fd, &active_fds);
            FD_SET(target_fd, &active_fds);
            if (client_fd > max_fd) max_fd = client_fd;
            if (target_fd > max_fd) max_fd = target_fd;

            printf("New connection established. Client FD: %d, Target FD: %d\n",
                   client_fd, target_fd);
          } else {
            close(client_fd);
          }
        } else {
          close(client_fd);
        }
      } else {
        perror("accept");
      }
    }
    int i;
    for (i = 0; i < conn_count; i++) {
      int client_fd = connections[i].client_fd;
      int target_fd = connections[i].target_fd;

      if (FD_ISSET(client_fd, &read_fds)) {
        handle_data_transfer(client_fd, target_fd, &active_fds, &write_fds);
      }

      if (FD_ISSET(target_fd, &read_fds)) {
        handle_data_transfer(target_fd, client_fd, &active_fds, &write_fds);
      }
    }
    int j;
    for (j = 0; j < conn_count;) {
      if (connections[j].client_fd == -1 || connections[j].target_fd == -1) {
        remove_connection(connections, &conn_count, j);
      } else {
        j++;
      }
    }
  }

  close(proxy_fd);
  return 0;
}
