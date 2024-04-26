#ifndef REPLY_SERVER
#define REPLY_SERVER

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

class Server {
 public:
  Server(const char *ip, int port) : server_ip(ip), server_port(port) {
    // 初始化两种监听模式
    listen_ev[0].data.fd = epoll_fd;
    listen_ev[1].data.fd = epoll_fd;
    listen_ev[0].events = EPOLLIN;
    listen_ev[1].events = EPOLLIN | EPOLLOUT;

    //创建服务器套接子
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
      throw std::runtime_error("构造服务器失败1");
    }

    //绑定服务器地址和端口
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) == -1) {
      close(server_socket);
      throw std::runtime_error("构造服务器失败2");
    }

    //监听连接
    if (listen(server_socket, 5) == -1) {
      close(server_socket);
      throw std::runtime_error("构造服务器失败3");
    }

    //创建epoll实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      close(server_socket);
      throw std::runtime_error("构造服务器失败4");
    }

    //添加服务器套接字到epoll实例
    event.events = EPOLLIN;
    event.data.fd = server_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
      close(epoll_fd);
      close(server_socket);
      throw std::runtime_error("构造服务器失败5");
    }
  }

  // 修改监听事件
  void ModListen(int i, int current_fd) {
    listen_ev[0].data.fd = current_fd;
    listen_ev[1].data.fd = current_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &listen_ev[i]) == -1) {
      std::string err_s = "epoll_ctl: mod " + std::to_string(i);
      perror(err_s.c_str());
      exit(EXIT_FAILURE);
    }
  }

  // 清理断开的套接字
  void DelCliSocket(int current_fd, int client_id) {
    // 从epoll中删除
    int ecrn = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
    if (ecrn == -1) {
      std::cerr << "Failed to remove client socket from epoll." << std::endl;
      exit(-1);
    }

    //删除映射关系
    auto it1 = sessionMap.find(current_fd);
    if (it1 != sessionMap.end()) {
      std::cout << "客户端断开连接id: " << it1->second << std::endl;
      sessionMap.erase(it1);
    }

    //删除反向映射
    auto it2 = rSessionMap.find(client_id);
    if (it2 != sessionMap.end()) {
      std::cout << "对应套接字删除: " << it2->second << std::endl;
      rSessionMap.erase(it2);
    }
    close(current_fd);
  }

  static void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
  }

  void run() {
    running = true;
    // 注册信号SIGINT和处理函数
    signal(SIGINT, signalHandler);
    epoll_event events[MAX_EVENTS];
    while (running) {
      //使用epoll_wait函数等待事件的发生，将事件存储在events数组中
      int numEvents = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
      if (numEvents == -1) {
        perror("Error waiting for events");
        break;
      }
      std::cout << "事件" << numEvents << "发生" << std::endl;
      //处理事件
      for (int i = 0; i < numEvents; ++i) {
        int current_fd = events[i].data.fd;
        if (current_fd == server_socket) {
          //新连接事件
          sockaddr_in client_address;
          socklen_t client_addr_len = sizeof(client_address);
          int client_socket = accept(
              server_socket, reinterpret_cast<sockaddr *>(&client_address),
              &client_addr_len);
          std::cout << "Accepted connection from "
                    << inet_ntoa(client_address.sin_addr) << std::endl;

          // 接收客户端发送的会话 ID
          char session_id[64];
          ssize_t bytesRead =
              recv(client_socket, session_id, sizeof(session_id) - 1, 0);
          if (bytesRead <= 0) {
            // 处理连接断开的情况
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
            close(client_socket);
            continue;
          }
          session_id[bytesRead] = '\0';

          // 存储会话ID和客户端套接字的映射关系
          int session_id_int = std::atoi(session_id);
          sessionMap[client_socket] = session_id_int;
          rSessionMap[session_id_int] = client_socket;
          recv_buffers[client_socket] = std::vector<char>();
          std::cout << "注册会话id: " << session_id_int << std::endl;
          event.events = EPOLLIN;
          event.data.fd = client_socket;

          //将客户端套接字添加到 epoll 实例，关注事件为可读
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
            perror("Error adding client socket to epoll");
            close(client_socket);
            continue;
          }

          // 发送确认消息
          const char *ack_msg = "ACK";
          send(client_socket, ack_msg, strlen(ack_msg), 0);

          // 设置为非阻塞模式
          // int flags = fcntl(server_socket, F_GETFL, 0);
          // if (flags == -1) {
          //   throw std::runtime_error("获取文件状态标志失败");
          // }
          // flags |= O_NONBLOCK;
          // if (fcntl(server_socket, F_SETFL, flags) == -1) {
          //   throw std::runtime_error("设置文件状态标志失败");
          // }
          int flags = fcntl(client_socket, F_GETFL, 0);
          if (flags == -1) {
            throw std::runtime_error("获取文件状态标志失败");
          }
          flags |= O_NONBLOCK;
          if (fcntl(client_socket, F_SETFL, flags) == -1) {
            throw std::runtime_error("设置文件状态标志失败");
          }
        } else {
          if (events[i].events & EPOLLIN) {
            //数据可读事件
            int client_id = sessionMap[current_fd];

            // 处理来自客户端的数据
            char buffer[1024];
            ssize_t bytesRead = recv(current_fd, buffer, sizeof(buffer) - 1, 0);

            //错误执行相应的处理
            if (bytesRead < 0) {
              if (errno != EWOULDBLOCK) {
                std::cerr << "errno:" << errno << "  " << std::strerror(errno)
                          << std::endl;
              }
              if (errno == ECONNRESET) {
                std::cerr << "Connection reset by peer.\n";
                DelCliSocket(current_fd, client_id);
                continue;
              }
            } else if (bytesRead == 0) {
              // int ecrn =
              //     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
              // if (ecrn == -1) {
              //   std::cerr << "Failed to remove client socket from epoll."
              //             << std::endl;
              //   exit(-1);
              // }

              // //删除映射关系
              // auto it1 = sessionMap.find(current_fd);
              // if (it1 != sessionMap.end()) {
              //   std::cout << "客户端断开连接id: " << it1->second <<
              //   std::endl; sessionMap.erase(it1);
              // }

              // //删除反向映射
              // auto it2 = rSessionMap.find(client_id);
              // if (it2 != sessionMap.end()) {
              //   std::cout << "对应套接字删除: " << it2->second << std::endl;
              //   rSessionMap.erase(it2);
              // }
              // close(current_fd);
              DelCliSocket(current_fd, client_id);
              continue;
            }
            // 缓存接收到的数据
            recv_buffers[current_fd].insert(recv_buffers[current_fd].end(),
                                            buffer, buffer + bytesRead);
            // 缓冲区非空，监听写
            if (!recv_buffers[current_fd].empty()) {
              ModListen(1, current_fd);
            }
            buffer[bytesRead] = '\0';

            // 输出
            std::cout << "Received message from client" << client_id << ": "
                      << buffer << std::endl;
          }
          if (events[i].events & EPOLLOUT) {
            // 判断对方是否存在
            int client_id = -1, target_id = -1;
            client_id = sessionMap[current_fd];
            target_id =
                (client_id % 2 == 0)
                    ? (client_id + 1)
                    : (client_id - 1);  //偶数加一,奇数减一，获取目标客户端id

            // 键不存在
            auto it = rSessionMap.find(target_id);
            if (it == rSessionMap.end()) {
              target_id = client_id;
              std::string message = "The counterpart is disconnected";
              recv_buffers[current_fd].clear();
              recv_buffers[current_fd].insert(recv_buffers[current_fd].begin(),
                                              message.begin(), message.end());
            }

            // ？？？增加判断条件：目的client正常连接???
            if (!recv_buffers[current_fd].empty()) {
              ssize_t bytesSent =
                  send(rSessionMap[target_id], recv_buffers[current_fd].data(),
                       recv_buffers[current_fd].size(), MSG_NOSIGNAL);
              std::cout << client_id << "Sending message to "
                        << " to client " << target_id << std::endl;
              if ((bytesSent) < 0) {
                if (errno != EWOULDBLOCK) {
                  std::cerr << "发送出错，errno值为: " << errno
                            << ". 错误信息: " << strerror(errno) << std::endl;
                  if (errno == EPIPE) {
                    // 对方已经关闭了连接，关闭我们的套接字并进行清理
                    //...
                    std::cerr << "对方已经关闭了连接" << std::endl;
                  }
                }
              } else {
                // 删除已发送的数据
                recv_buffers[current_fd].erase(
                    recv_buffers[current_fd].begin(),
                    recv_buffers[current_fd].begin() + bytesSent);

                // 缓冲区空，取消监听写
                if (recv_buffers[current_fd].empty()) {
                  ModListen(0, current_fd);
                }
              }
            }
          }

          //数据发送给目标客户端
          // send(rSessionMap[target_id], buffer, bytesRead, 0);
        }
      }
    }
  }

  ~Server() {
    sessionMap.clear();
    rSessionMap.clear();
    recv_buffers.clear();
    close(epoll_fd);
    close(server_socket);
  }

 private:
  const char *server_ip;
  const int MAX_EVENTS = 10000;
  int server_port;
  int server_socket;
  int epoll_fd = -1;
  std::unordered_map<int, int> sessionMap;   // client_socket to session_id
  std::unordered_map<int, int> rSessionMap;  // session_id to client_socket
  std::unordered_map<int, std::vector<char>>
      recv_buffers;  //套接字到缓冲区的映射
  epoll_event event;
  size_t buf_size = 20 * 1024;  // 缓存区大小
  epoll_event listen_ev[2];     // 两种监听模式
  static bool running;
};
#endif