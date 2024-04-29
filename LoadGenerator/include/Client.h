#ifndef CLIENT
#define CLIENT

#ifndef DEBUG
#define DEBUG
#endif

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "ClientMessage.h"

class Client {
 public:
  Client(const std::string& id, uint8_t messageType, const char* data,
         uint32_t dataLength, size_t msg_size);

  ~Client() {
    // std::cout << "客户端" << id << " 自己：" << message_count << " 总数："
    //           << all_message_count << std::endl;
  }

  // 客户端连接服务器
  void ConnectToServer(const char* server_ip, int server_port);

  // 处理读写事件
  void EventLoop(int loop_round = 10000);

  /**
   * @brief 修改监听模式
   * @param i - 监听模式
   * 可选值：
   * - 0:读
   * - 1:读写
   */
  void ModListen(int i);

  // 信号处理函数
  static void SignalHandler(int signum);

  // 返回每报文平均延迟
  double GetAvrTimePerMsg() { return avr_time_per_message; }

  // 返回报文总数
  double GetAllMsgCount() { return all_message_count; }

  //  private:
  int epoll_fd;
  int client_socket;
  std::string id;
  std::string client_name;

  ClientMessage client_message;  // 消息
  size_t msg_size;               // 消息总长度
  size_t msg_data_size;          // 消息正文长度
  size_t recv_size;              // 缓冲区长度

  unsigned int count_one_msg = 0;  // 统计一条消息
  unsigned int recv_num = 0;       // 接收到的消息条数

  std::vector<char> recv_buffer;  // 需要加一个最大长度来做限制
  std::vector<char> send_buffer;  // 需要加一个最大长度来做限制

  epoll_event listen_ev[2];

  // 计时
  std::chrono::high_resolution_clock::time_point start_time;  // 记录开始时间点
  std::chrono::high_resolution_clock::time_point end_time;  // 记录结束时间点
  std::chrono::milliseconds diff_time;  // 计算耗费的时间
  double avr_time_per_message = 0;      // 每报文平均延迟
  double message_count = 0;      // 收到对方对自己报文的响应次数
  double all_message_count = 0;  // 收到报文总数

  static bool running;
  int run_num = 0;
};

#endif