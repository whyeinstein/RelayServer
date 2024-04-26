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
  Server(const char *ip, int port);
  ~Server();

  // 修改监听事件
  void ModListen(int i, int current_fd);

  // 清理断开的套接字
  void DelCliSocket(int current_fd, int client_id);

  // 捕获信号
  static void signalHandler(int signum);

  // 服务器运行
  void run();

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