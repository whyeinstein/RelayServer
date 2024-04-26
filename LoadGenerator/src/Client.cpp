#include "Client.h"

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
  std::cout << id << " 连接到服务器" << std::endl;

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

void Client::EventLoop() {
  epoll_event events[10];
  while (true) {
    // 注意nfds为-1的情况，timeout==-1无限期等待事件发生  ？？？
    int nfds = epoll_wait(epoll_fd, events, 10, -1);
    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == client_socket) {
        if (events[i].events & EPOLLIN) {
          // 若缓冲区已满，停止读取数据
          if (recv_buffer.size() >= recv_size) {
            continue;
          }
          // 读取数据
          char buffer[1024];
          ssize_t bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
          if (bytesRead <= 0) {
            // 连接已关闭
            close(client_socket);
            close(epoll_fd);
            return;
          }

          // 判断是否已经接受完一条完整的消息
          count_one_msg += bytesRead;
          if (count_one_msg >= msg_size) {
            count_one_msg -= msg_size;
            recv_num++;
          }

          // 添加接收到的数据到recv_buffer
          recv_buffer.insert(recv_buffer.end(), buffer, buffer + bytesRead);
          std::string receivedData(buffer, bytesRead);

          // 缓冲区非空，监听写
          if (!recv_buffer.empty()) {
            ModListen(1);
          }
          std::cout << "Client " << id
                    << " received message size: " << bytesRead << std::endl;
        }
        if ((events[i].events & EPOLLOUT) && !recv_buffer.empty()) {
          ssize_t bytesSent =
              send(client_socket, recv_buffer.data(), recv_buffer.size(), 0);
          if (bytesSent <= 0) {
            // 发送失败
            close(client_socket);
            close(epoll_fd);
            return;
          }

          // for (std::vector<char>::iterator it = recv_buffer.begin();
          //      it != recv_buffer.begin() + bytesSent; it++) {
          //   std::cout << *it;
          // }
          // std::cout << std::endl;

          // 删除已发送的数据
          recv_buffer.erase(recv_buffer.begin(),
                            recv_buffer.begin() + bytesSent);
          std::cout << "Client " << id << " sent message size: " << bytesSent
                    << std::endl;

          // 缓冲区空，取消监听写
          if (recv_buffer.empty()) {
            ModListen(0);
          }
        }
      }
    }
  }
}

// 修改监听事件
void Client::ModListen(int i) {
  if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &listen_ev[i]) == -1) {
    std::string err_s = "epoll_ctl: mod " + std::to_string(i);
    perror(err_s.c_str());
    exit(EXIT_FAILURE);
  }
}