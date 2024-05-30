#ifndef REPLY_SERVER
#define REPLY_SERVER

#ifndef DEBUG
// #define DEBUG
#endif

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
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
  static void SignalHandler(int signum);

  // 服务器运行
  void Run();

  // 服务器处理新连接
  int NewConnect();

  // 接受数据
  bool RecvData(int current_fd);

  // 发送数据
  void SendData(int current_fd);

  // 新
  void ConnectionHandler();

 private:
  const char *server_ip;
  const int MAX_EVENTS = 20000;
  int server_port;
  int server_socket;
  int epoll_fd = -1;
  std::unordered_map<int, int> sessionMap;   // client_socket to session_id
  std::unordered_map<int, int> rSessionMap;  // session_id to client_socket
  std::unordered_map<int, std::vector<char>>
      recv_buffers;  //套接字到缓冲区的映射
  epoll_event event;
  size_t buf_size = 22 * 1024;  // 缓存区大小
  epoll_event listen_ev[2];     // 两种监听模式
  static bool running;
  std::queue<int> new_connections;   // 新连接队列
  std::mutex queue_mutex;            // 队列锁
  std::condition_variable queue_cv;  // 新连接条件变量
};
#endif