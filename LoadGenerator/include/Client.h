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

 public:
  Client(const std::string& id, uint8_t messageType, const char* data,
         uint32_t dataLength, size_t msg_size)
      : epoll_fd(-1),
        client_socket(-1),
        id(id),
        client_name("client" + id),
        client_message(messageType, data, dataLength),
        msg_data_size(msg_size) {
    // 初始化两种监听模式
    listen_ev[0].data.fd = epoll_fd;
    listen_ev[1].data.fd = epoll_fd;
    listen_ev[0].events = EPOLLIN;
    listen_ev[1].events = EPOLLIN | EPOLLOUT;

    // 初始化消息，缓冲区大小
    msg_size =
        client_message.getData().size() + sizeof(client_message.getHeader());
    recv_size = 100 * msg_size;
    recv_buffer.resize(msg_size);

    // 初始化缓冲区
    std::memcpy(recv_buffer.data(), &(client_message.getHeader()),
                sizeof(ClientMessageHeader));
    std::memcpy(recv_buffer.data() + sizeof(ClientMessageHeader),
                client_message.getData().data(),
                client_message.getData().size());
  }

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
};

#endif