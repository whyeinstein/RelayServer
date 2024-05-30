#include "Server.h"

#include <thread>

#include "Message.h"

Server::Server(const char *ip, int port) : server_ip(ip), server_port(port) {
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

Server::~Server() {
  sessionMap.clear();
  rSessionMap.clear();
  recv_buffers.clear();
  close(epoll_fd);
  close(server_socket);
}

// 修改监听事件
void Server::ModListen(int i, int current_fd) {
  listen_ev[0].data.fd = current_fd;
  listen_ev[1].data.fd = current_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &listen_ev[i]) == -1) {
    std::string err_s = "epoll_ctl: mod " + std::to_string(i);
    perror(err_s.c_str());
    exit(EXIT_FAILURE);
  }
}

// 清理断开的套接字
void Server::DelCliSocket(int current_fd, int client_id) {
  // 临走前告诉对方自己已经下线
  // 判断对方是否存在
  int target_id = -1;

  target_id = (client_id % 2 == 0)
                  ? (client_id + 1)
                  : (client_id - 1);  //偶数加一,奇数减一，获取目标客户端id

  // 对方还在线
  auto it = rSessionMap.find(target_id);
  if (it != rSessionMap.end()) {
    std::string message = "The counterpart is disconnected";
    Message msg(1, message.c_str(), message.size(), std::to_string(target_id));

    // 序列化并加入缓冲区
    std::vector<char> data_to_send = msg.serialize();
    recv_buffers[current_fd].clear();
    recv_buffers[current_fd].insert(recv_buffers[current_fd].begin(),
                                    data_to_send.begin(), data_to_send.end());

    // 因为自己即将下线，因此必须将消息send出去，否则epoll不会监听当前套接字，缓冲区中内容无法发送

    ssize_t bytesSent =
        send(rSessionMap[target_id], recv_buffers[current_fd].data(),
             recv_buffers[current_fd].size(), MSG_NOSIGNAL);
#ifdef DEBUG
    std::cout << client_id << " Sending offline message to "
              << " client " << target_id << std::endl;
#endif
    // 差错处理
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
    }
  }

  // 从epoll中删除
  int ecrn = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
  if (ecrn == -1) {
    std::cerr << "Failed to remove client socket from epoll." << std::endl;
    exit(-1);
  }

  //删除映射关系
  auto it1 = sessionMap.find(current_fd);
  if (it1 != sessionMap.end()) {
#ifdef DEBUG
    std::cout << "客户端断开连接id: " << it1->second << std::endl;
#endif
    sessionMap.erase(it1);
  }

  //删除反向映射
  auto it2 = rSessionMap.find(client_id);
  if (it2 != sessionMap.end()) {
#ifdef DEBUG
    std::cout << "对应套接字删除: " << it2->second << std::endl;
#endif
    rSessionMap.erase(it2);
  }
  close(current_fd);
}

void Server::SignalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  running = false;
}

// 新连接事件
int Server::NewConnect() {
  // bool continue_flag = false;
  sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  int client_socket =
      accept(server_socket, reinterpret_cast<sockaddr *>(&client_address),
             &client_addr_len);
  if (client_socket == -1) {
    perror("Error accepting new connection");
  }
  // #ifdef DEBUG
  std::cout << "Accepted connection from " << inet_ntoa(client_address.sin_addr)
            << std::endl;
  // #endif

  return client_socket;

  // // 接收客户端发送的会话 ID
  // char session_id[64];
  // ssize_t bytesRead =
  //     recv(client_socket, session_id, sizeof(session_id) - 1, 0);
  // if (bytesRead <= 0) {
  //   // 处理连接断开的情况
  //   epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
  //   close(client_socket);
  //   // continue;
  //   continue_flag = true;
  //   return continue_flag;
  // }
  // session_id[bytesRead] = '\0';

  // // 存储会话ID和客户端套接字的映射关系
  // int session_id_int = std::atoi(session_id);
  // sessionMap[client_socket] = session_id_int;
  // rSessionMap[session_id_int] = client_socket;
  // recv_buffers[client_socket] = std::vector<char>();
  // // #ifdef DEBUG
  // std::cout << "注册会话id: " << session_id_int << std::endl;
  // // #endif
  // event.events = EPOLLIN;
  // event.data.fd = client_socket;

  // //将客户端套接字添加到 epoll 实例，关注事件为可读
  // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
  //   perror("Error adding client socket to epoll");
  //   close(client_socket);
  //   // continue;
  //   continue_flag = true;
  //   return continue_flag;
  // }

  // // 发送确认消息
  // const char *ack_msg = "ACK";
  // send(client_socket, ack_msg, strlen(ack_msg), 0);

  // int flags = fcntl(client_socket, F_GETFL, 0);
  // if (flags == -1) {
  //   throw std::runtime_error("获取文件状态标志失败");
  // }
  // flags |= O_NONBLOCK;
  // if (fcntl(client_socket, F_SETFL, flags) == -1) {
  //   throw std::runtime_error("设置文件状态标志失败");
  // }
  // return continue_flag;
}

bool Server::RecvData(int current_fd) {
  //数据可读事件
  int client_id = sessionMap[current_fd];

  // 处理来自客户端的数据
  char buffer[128 * 1024 * 3];
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
      // continue;
      return true;
    }
  } else if (bytesRead == 0) {
    DelCliSocket(current_fd, client_id);
    // continue;
    return true;
  }
  // 缓存接收到的数据
  recv_buffers[current_fd].insert(recv_buffers[current_fd].end(), buffer,
                                  buffer + bytesRead);
  // 缓冲区非空，监听写
  if (!recv_buffers[current_fd].empty()) {
    ModListen(1, current_fd);
  }
  buffer[bytesRead] = '\0';

// 输出
#ifdef DEBUG
  std::cout << " Received message from client " << client_id << ": "
            << bytesRead << std::endl;
#endif
  return false;
}

void Server::SendData(int current_fd) {
  // 判断对方是否存在
  int client_id = -1, target_id = -1;
  client_id = sessionMap[current_fd];
  target_id = (client_id % 2 == 0)
                  ? (client_id + 1)
                  : (client_id - 1);  //偶数加一,奇数减一，获取目标客户端id

  // debug
  // target_id = 0;

  // 键不存在
  auto it = rSessionMap.find(target_id);
  if (it == rSessionMap.end()) {
    std::string message = "The counterpart is disconnected";
    Message msg(1, message.c_str(), message.size(), std::to_string(target_id));

    // 序列化并加入缓冲区
    std::vector<char> data_to_send = msg.serialize();
    recv_buffers[current_fd].clear();
    recv_buffers[current_fd].insert(recv_buffers[current_fd].begin(),
                                    data_to_send.begin(), data_to_send.end());
    // 将目的id置为当前套接字对应的id
    target_id = client_id;
  }

  // ？？？增加判断条件：目的client正常连接???
  if (!recv_buffers[current_fd].empty()) {
    ssize_t bytesSent =
        send(rSessionMap[target_id], recv_buffers[current_fd].data(),
             recv_buffers[current_fd].size(), MSG_NOSIGNAL);
#ifdef DEBUG
    std::cout << client_id << "Sending message to "
              << " to client " << target_id << std::endl;
#endif
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

void Server::Run() {
  running = true;

  // 注册信号SIGINT和处理函数
  signal(SIGINT, SignalHandler);
  epoll_event events[MAX_EVENTS];

  // 注册副线程处理新连接
  std::thread connection_thread(&Server::ConnectionHandler, this);

  while (running) {
    //使用epoll_wait函数等待事件的发生，将事件存储在events数组中
    int numEvents = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (numEvents == -1) {
      perror("Error waiting for events");
      break;
    }
#ifdef DEBUG
    std::cout << "事件" << numEvents << "发生" << std::endl;
#endif
    //处理事件
    for (int i = 0; i < numEvents; ++i) {
      int current_fd = events[i].data.fd;
      if (current_fd == server_socket) {
        int new_fd = NewConnect();
        if (new_fd != -1) {
          std::unique_lock<std::mutex> lock(queue_mutex);
          new_connections.push(new_fd);
          lock.unlock();
          queue_cv.notify_one();
        }
        continue;
        // if (NewConnect()) {
        //   continue;
        // }
      } else {
        if (events[i].events & EPOLLIN) {
          if (RecvData(current_fd)) {
            continue;
          }
        }
        if (events[i].events & EPOLLOUT) {
          SendData(current_fd);
        }
      }
    }
  }
}

void Server::ConnectionHandler() {
  while (running) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock,
                  [this] { return !new_connections.empty() || !running; });
#ifdef DEBUG
    std::cout << "副线程唤醒" << std::endl;
#endif

    while (!new_connections.empty()) {
#ifdef DEBUG
      std::cout << "处理新连接" << std::endl;
#endif
      int client_socket = new_connections.front();
      new_connections.pop();
      lock.unlock();

      // 处理新连接
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
      // #ifdef DEBUG
      std::cout << "注册会话id: " << session_id_int << std::endl;
      // #endif
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

      int flags = fcntl(client_socket, F_GETFL, 0);
      if (flags == -1) {
        throw std::runtime_error("获取文件状态标志失败");
      }
      flags |= O_NONBLOCK;
      if (fcntl(client_socket, F_SETFL, flags) == -1) {
        throw std::runtime_error("设置文件状态标志失败");
      }
      lock.lock();
    }
  }
}
