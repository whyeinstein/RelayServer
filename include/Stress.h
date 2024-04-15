#ifndef STRESS
#define STRESS
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

class StressGenerator {
public:
  StressGenerator(int num_clients) : num_clients(num_clients) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      perror("创建epoll失败");
      exit(EXIT_FAILURE);
    }
  }

  void run() {
    createClients();
    epollLoop0();
    epollLoop();
  }

private:
  int epoll_fd;
  int num_clients;
  std::vector<int> client_sockets;

  const int MAX_EVENTS = 10000;
  const char *server_ip = "192.168.31.141";
  const int SERVER_PORT = 12346;

  std::unordered_map<int, int> idMap; // client_socket to session_id

  void createClients() {
    //创建套接字
    for (int i = 0; i < num_clients; ++i) {
      int client_socket = socket(AF_INET, SOCK_STREAM, 0);
      if (client_socket == -1) {
        perror("Failed to create client socket");
        exit(EXIT_FAILURE);
      }

      //连接服务器
      sockaddr_in server_addr{};
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = inet_addr(server_ip);
      server_addr.sin_port = htons(SERVER_PORT);

      if (connect(client_socket,
                  reinterpret_cast<struct sockaddr *>(&server_addr),
                  sizeof(server_addr)) == -1) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
      }

      //存储id与套结字关系
      idMap[client_socket] = i;
      //发送id
      std::string s = std::to_string(i);
      send(client_socket, s.c_str(), strlen(s.c_str()), 0);

      // 非阻塞模式
      fcntl(client_socket, F_SETFL,
            fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);

      client_sockets.push_back(client_socket);

      // epoll
      epoll_event event{};
      event.events = EPOLLIN | EPOLLOUT | EPOLLET; // ET边缘触发
      event.data.fd = client_socket;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
        perror("Failed to add client socket to epoll");
        exit(EXIT_FAILURE);
      }

      std::cout << "Client " << i + 1 << " connected." << std::endl;
    }
  }

  void epollLoop0() {
    epoll_event events[MAX_EVENTS];

    int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < num_events; ++i) {
      if ((events[i].events & EPOLLOUT) &&
          (idMap[events[i].data.fd] % 2 == 0)) { //偶数id发送
        sendMessage(events[i].data.fd, "666");
        std::cout << "客户端" << idMap[events[i].data.fd] << "发送666"
                  << std::endl;
      }
    }
  }

  //   void sendMessage(int client_socket) {
  //     const char *message = "Hello, Server!";
  //     int bytes_sent = send(client_socket, message, strlen(message), 0);

  //     if (bytes_sent == -1) {
  //       perror("Failed to send message");
  //       exit(EXIT_FAILURE);
  //     }

  //     std::cout << "Client " << client_socket << " sent message: " << message
  //               << std::endl;

  //     // Remove the EPOLLOUT event to avoid continuous writing
  //     epoll_event event{};
  //     event.events = EPOLLET; // Edge-triggered mode
  //     event.data.fd = client_socket;

  //     if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event) == -1) {
  //       perror("Failed to modify epoll event for client socket");
  //       exit(EXIT_FAILURE);
  //     }
  //   }
  void epollLoop() {
    epoll_event events[MAX_EVENTS];
    int count = 0;

    while (true) {
      count++;
      std::cout << "============" << count << std::endl;
      if (count == 200000) {
        std::cout << count << std::endl;
        break;
      }
      int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
      std::cout << "epollLoop事件" << num_events << "发生" << std::endl;
      for (int i = 0; i < num_events; ++i) {
        if (events[i].events & EPOLLIN) {
          // Receive and process incoming message
          receiveAndReply(events[i].data.fd);
        }
        // else if (events[i].events & EPOLLOUT) {
        //   // Send a message to the server
        //   sendMessage(events[i].data.fd, "666");
        // }
      }
    }
  }

  void receiveAndReply(int client_socket) {
    char buffer[256];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
      buffer[bytes_received] = '\0';
      // std::cout << idMap[client_socket] << "收到消息 " << buffer <<
      // std::endl; Echo the received message back to the server
      sendMessage(client_socket, buffer);
    } else if (bytes_received == 0) {
      // Connection closed by the server
      std::cout << "连接关闭" << std::endl;
      close(client_socket);
      exit(EXIT_SUCCESS);
    }
  }

  void sendMessage(int client_socket, const char *message) {
    int bytes_sent = send(client_socket, message, strlen(message), 0);

    if (bytes_sent == -1) {
      perror("发送消息失败");
      exit(EXIT_FAILURE);
    }

    // std::cout << "客户端id" << idMap[client_socket] << " 发送消息: " <<
    // message
    //           << std::endl;

    // { // Modify epoll event to wait for incoming messages (EPOLLIN)
    //   epoll_event event{};
    //   event.events = EPOLLIN | EPOLLET; // Edge-triggered mode for input
    //   events event.data.fd = client_socket;

    //   if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event) == -1) {
    //     perror("Failed to modify epoll event for client socket");
    //     exit(EXIT_FAILURE);
    //   }
    // }
  }
};
#endif
