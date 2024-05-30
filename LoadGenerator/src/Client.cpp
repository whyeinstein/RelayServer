#include "Client.h"

Client::Client(const std::string& id, uint8_t messageType, const char* data,
               uint32_t dataLength, size_t msg_size)
    : epoll_fd(-1),
      client_socket(-1),
      id(id),
      client_name("client" + id),
      client_message(messageType, data, dataLength, id),
      msg_data_size(msg_size) {
  // 初始化两种监听模式
  listen_ev[0].data.fd = epoll_fd;
  listen_ev[1].data.fd = epoll_fd;
  listen_ev[0].events = EPOLLIN;
  listen_ev[1].events = EPOLLIN | EPOLLOUT;

  // 初始化消息，缓冲区大小
  msg_size =
      client_message.getData().size() + sizeof(client_message.getHeader());
  recv_size = 10 * msg_size;
  // recv_buffer.resize(msg_size);
  // send_buffer.resize(msg_size);

  std::vector<char> data_to_send = client_message.serialize();
  send_buffer.insert(send_buffer.end(), data_to_send.begin(),
                     data_to_send.end());
  // 初始化缓冲区
  // std::memcpy(send_buffer.data(), &(client_message.getHeader()),
  //             sizeof(ClientMessageHeader));
  // std::memcpy(send_buffer.data() + sizeof(ClientMessageHeader),
  //             client_message.getData().data(),
  //             client_message.getData().size());
}

void Client::ConnectToServer(const char* server_ip, int server_port) {
  // 创建客户端套接字
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    perror("创建客户端套接字发生错误");
    return;
  }

  // 初始化两种监听模式的套接字
  listen_ev[0].data.fd = client_socket;
  listen_ev[1].data.fd = client_socket;

  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(server_ip);
  server_address.sin_port = htons(server_port);

  // 连接到服务器
  if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address),
              sizeof(server_address)) == -1) {
    if (errno != EINPROGRESS) {
      perror("连接到服务器时发生错误");
      close(client_socket);
      return;
    }
  }

  // 建立连接后发送客户端id
  const char* session_id = id.c_str();
  send(client_socket, session_id, strlen(session_id), 0);

  // 确认id发送到位
  char ack[64];
  ssize_t bytesRead = recv(client_socket, ack, sizeof(ack) - 1, 0);
  if (bytesRead <= 0) {
    std::cerr << "bytesRead:" << bytesRead << std::endl;
    perror("从服务器接收确认信息出错");
    close(client_socket);
    return;
  }
  ack[bytesRead] = '\0';
  if (strcmp(ack, "ACK") != 0) {
    std::cerr << "没有从服务器接收到确认信息，收到的是：" << ack << std::endl;
    close(client_socket);
    return;
  }

#ifdef DEBUG
  std::cout << id << " 连接到服务器" << std::endl;
#endif

  // 创建 epoll 实例
  epoll_fd = epoll_create1(0);

  // 添加客户端套接字到 epoll
  epoll_event ev = {0};
  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = client_socket;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
    perror("压力发生器:将客户端套接字添加到epoll时出错");
    close(client_socket);
  }

  // 设置套接字为非阻塞模式
  int flags = fcntl(client_socket, F_GETFL, 0);
  if (flags == -1) {
    throw std::runtime_error("获取文件状态标志失败");
  }
  flags |= O_NONBLOCK;
  if (fcntl(client_socket, F_SETFL, flags) == -1) {
    throw std::runtime_error("设置文件状态标志失败");
  }
}

void Client::SignalHandler(int signum) {
#ifdef DEBUG
  std::cout << "Interrupt signal (" << signum << ") received.\n";
#endif
  running = false;
}

void Client::EventLoop(int loop_round) {
  // 初始化状态变量
  bool readingHeader = true;
  ClientMessageHeader header;
  epoll_event events[10];

  // 注册信号SIGINT和处理函数
  running = true;
  // signal(SIGINT, SignalHandler);

  // 第一次发送起始时间
  start_time = std::chrono::high_resolution_clock::now();

  while (run_num <= loop_round || loop_round == -1) {
    run_num++;
    size_t ssize = send_buffer.size();
#ifdef DEBUG
    std::cout << "客户端" << id << "运行" << std::endl;
#endif
    // 注意nfds为-1的情况，timeout==-1无限期等待事件发生  ？？？
    if (is_end) {
      break;
    }
    int nfds = epoll_wait(epoll_fd, events, 10, -1);
    if (nfds == -1) {
      perror("error when epoll wait!");
      break;
    }
    if (is_end) {
      break;
    }
    for (int i = 0; i < nfds; i++) {
      // if (id == "0")
#ifdef DEBUG
      std::cout << "客户端" << id << "发生" << nfds << "个事件" << std::endl;
#endif

      if (events[i].data.fd == client_socket) {
        if (events[i].events & EPOLLIN) {
          // 若缓冲区已满，停止读取数据
          if (recv_buffer.size() >= recv_size) {
            continue;
          }
          // 读取数据
          char buffer[1024 * 129];
          ssize_t bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
          if (bytesRead <= 0) {
            // 连接已关闭
            close(client_socket);
            close(epoll_fd);
            return;
          }

          // 添加接收到的数据到recv_buffer
          recv_buffer.insert(recv_buffer.end(), buffer, buffer + bytesRead);
          // std::string receivedData(buffer, bytesRead);
          // if (id == "0")
#ifdef DEBUG
          std::cout << "客户端" << id << "读取数据" << std::endl;
#endif

          // 循环处理缓冲区中的数据
          while (recv_buffer.size() > 0) {
            if (readingHeader &&
                recv_buffer.size() >= sizeof(ClientMessageHeader)) {
              // if (id == "0")
#ifdef DEBUG
              std::cout << "客户端" << id << "读取头部" << std::endl;
#endif
              // 读取头部
              memcpy(&header, recv_buffer.data(), sizeof(ClientMessageHeader));

              recv_buffer.erase(
                  recv_buffer.begin(),
                  recv_buffer.begin() + sizeof(ClientMessageHeader));

              readingHeader = false;
            } else {
              // 读取数据
              if (recv_buffer.size() >= header.messageLength) {
                // if (id == "0")

#ifdef DEBUG
                std::cout << "客户端" << id << "读取完一条消息" << std::endl;
#endif
                ClientMessage message(header.messageType, recv_buffer.data(),
                                      header.messageLength,
                                      std::to_string(header.id_));
                recv_buffer.erase(recv_buffer.begin(),
                                  recv_buffer.begin() + header.messageLength);
                all_message_count++;

                // 处理message内容,若是自己发送的信息则记录一次延迟时间
                if (header.id_ == std::stoi(id)) {
                  // if (id == "0")
#ifdef DEBUG
                  std::cout << "客户端" << id << "读取到自己发送的消息"
                            << std::endl;
#endif
                  end_time = std::chrono::high_resolution_clock::now();
                  diff_time =
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          end_time - start_time);

                  // 计算每报文平均延迟
                  avr_time_per_message = (avr_time_per_message * message_count +
                                          diff_time.count()) /
                                         (message_count + 1);
                  message_count++;

                  // 下一轮的开始时间
                  start_time = std::chrono::high_resolution_clock::now();
                } else {
                  // if (id == "0")
#ifdef DEBUG
                  std::cout << "客户端" << id << "读取到对方发送的消息"
                            << std::endl;
#endif
                }

                // 序列化准备发送
                std::vector<char> data_to_send = message.serialize();

                // 将接收到的消息添加到send_buffer
                send_buffer.insert(send_buffer.end(), data_to_send.begin(),
                                   data_to_send.end());

                readingHeader = true;
              } else {
                // 缓冲区中的数据还不够一个完整的消息，那么等待下一轮读取数据
                break;
              }
            }
          }

          // 缓冲区非空，监听写
          if (!send_buffer.empty()) {
            ModListen(1);
          }
        }
        if ((events[i].events & EPOLLOUT) && !send_buffer.empty()) {
          ssize_t bytesSent =
              send(client_socket, send_buffer.data(), send_buffer.size(), 0);
          if (bytesSent <= 0) {
            // 发送失败
            close(client_socket);
            close(epoll_fd);
            return;
          }

          // 删除已发送的数据
          send_buffer.erase(send_buffer.begin(),
                            send_buffer.begin() + bytesSent);
          // if (id == "0")

#ifdef DEBUG
          std::cout << "客户端" << id << "发送的消息长度" << bytesSent
                    << std::endl;
#endif
          // 缓冲区空，取消监听写
          if (send_buffer.empty()) {
            ModListen(0);
          }
        }
      }
    }
  }
#ifdef DEBUG
  std::cout << id << "终结" << std::endl;
#endif
  // // 关闭套接字
  if (close(client_socket) == -1) {
    // 打印错误信息
    std::cerr << "Failed to close the client socket: " << strerror(errno)
              << std::endl;
  }
  is_end = true;
}

// 修改监听事件
void Client::ModListen(int i) {
  if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &listen_ev[i]) == -1) {
    std::string err_s = "epoll_ctl: mod " + std::to_string(i);
    perror(err_s.c_str());
    exit(EXIT_FAILURE);
  }
}