#ifndef CLIENT
#define CLIENT

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "ClientMessage.h"

class Client {
 public:
  Client(const std::string& id, uint8_t messageType, const char* data,
         uint32_t dataLength, size_t msg_size);

  // 客户端连接服务器
  void ConnectToServer(const char* server_ip, int server_port);

  // 处理读写事件
  void EventLoop();

  /**
   * @brief 修改监听模式
   * @param i - 监听模式
   * 可选值：
   * - 0:读
   * - 1:读写
   */
  void ModListen(int i);

 private:
  int epoll_fd;
  int client_socket;
  std::string id;
  std::string client_name;
  ClientMessage client_message;    // 消息
  size_t msg_size;                 // 消息总长度
  size_t msg_data_size;            // 消息正文长度
  unsigned int count_one_msg = 0;  // 统计一条消息
  unsigned int recv_num = 0;       // 接收到的消息条数

  size_t recv_size;               // 缓冲区长度
  std::vector<char> recv_buffer;  // 需要加一个最大长度来做限制
  epoll_event listen_ev[2];
};

#endif