#include <arpa/inet.h>  // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <poll.h>
#include <stdio.h>   // perror, printf
#include <stdlib.h>  // exit, atoi
#include <string.h>  // memset
#include <unistd.h>  // read, write, close

struct thread_args {
  int fd;
  struct sockaddr_in addr;
};
int INIT_SIZE = 1024;

void *xiancheng(struct thread_args *targs) {
  int clientFd = targs->fd;
  struct sockaddr_in client = targs->addr;
  free(targs);

  while (1) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int size = read(clientFd, buffer, sizeof(buffer));
    if (size < 0) {
      perror("read error");
      exit(5);
    } else if (size == 0) {
      close(clientFd);
      // printf("断开连接啦 from a client \n");
      char *client_ip = inet_ntoa(client.sin_addr);
      printf("断开连接啦 from a client %s:%d\n", client_ip,
             ntohs(client.sin_port));
      break;
    }
    printf("received %s from client %d \n", buffer, size);
    if (write(clientFd, buffer, size) < 0) {
      close(clientFd);
      perror("write error");
      exit(6);
    }
  }
}

int main(int argc, char const *argv[]) {
  int serverFd, clientFd;
  int ready_number;
  struct sockaddr_in server, client;
  struct pollfd event_set[INIT_SIZE];
  int len, n;
  int port = 1234;
  char *recv_line = (char *)malloc(INIT_SIZE);

  if (argc == 2) {
    port = atoi(argv[1]);
  }
  printf("要创建socket啦 \n");
  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    perror("Cannot create socket");
    exit(1);
  }
  printf("这是serverfd %d \n", serverFd);
  int reuse = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse,
             sizeof(int));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if (-1 == bind(serverFd, (struct sockaddr *)&server, sizeof(server))) {
    perror("bind server addr failure.");
    exit(EXIT_FAILURE);
  }
  if (listen(serverFd, 10) < 0) {
    perror("Listen error");
    exit(3);
  }

  len = sizeof(server);

  event_set[0].fd = serverFd;
  event_set[0].events = POLLIN;

  for (int i = 1; i < INIT_SIZE; i++) {
    event_set[i].fd = -1;
  }
  while (1) {
    if ((ready_number = poll(event_set, INIT_SIZE, -1)) < 0) {
      perror("pollfailed");
      exit(4);
    }

    if (event_set[0].revents && POLLIN) {
      printf("有情况！%d \n", ready_number);
      socklen_t client_len = sizeof(client);
      clientFd = accept(serverFd, (struct sockaddr *)&client, &client_len);
      printf("这里有一个新朋友过来了：%d \n", clientFd);
      int i;
      for (i = 1; i < INIT_SIZE; i++) {
        if (event_set[i].fd < 0) {
          event_set[i].fd = clientFd;
          event_set[i].events = POLLIN;
          break;
        }
      }
      if (i == INIT_SIZE) {
        perror("连接太多啦！不够用啦");
        exit(5);
      }
      if (--ready_number < 0) {
        continue;
      }
    }
    for (int i = 1; i < INIT_SIZE; i++) {
      if (event_set[i].fd == -1) {
        continue;
      }
      if (event_set[i].revents & POLLIN) {
        bzero(recv_line, INIT_SIZE);
        if ((n = read(event_set[i].fd, recv_line, INIT_SIZE)) > 0) {
          if (write(event_set[i].fd, recv_line, n) < 0) {
            perror("回写失败啦");
            exit(6);
          } else if (n == 0) {
            printf("客户端关闭啦");
            close(event_set[i].fd);
            event_set[i].fd = -1;
          } else {
            printf("回写了这么多 %s \n", recv_line);
          }
          if (--ready_number < 0) {
            break;
          }
        }
      }
    }
  }

  //线程的方式
  // pthread_t thread;
  // struct thread_args* targs = malloc(sizeof(struct thread_args));
  // targs->fd = clientFd;
  // targs->addr = client;
  // pthread_create(&thread, NULL, xiancheng, targs);
  // pthread_detach(thread);
  close(serverFd);
}
