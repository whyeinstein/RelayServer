#include "Server.h"
int main() {
  // Usage example
  // Server myServer("192.168.31.141", 12346);
  Server myServer("127.0.0.1", 8848);

  myServer.run();

  return 0;
}
// {const char *server_ip = "127.0.0.1";
// const int server_port = 12346;
// const int MAX_EVENTS = 10;

// std::unordered_map<int, int> sessionMap;  // client_socket to Session_id
// std::unordered_map<int, int> rSessionMap; // 反向映射

// int main() {
//   //创建服务器套接子
//   int server_socket = socket(AF_INET, SOCK_STREAM, 0);
//   if (server_socket == -1) {
//     perror("Error creating server socket");
//     return 1;
//   }

//   //绑定服务器地址和端口
//   sockaddr_in server_address;
//   server_address.sin_family = AF_INET;
//   server_address.sin_addr.s_addr = inet_addr(server_ip);
//   server_address.sin_port = htons(server_port);

//   if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_address),
//            sizeof(server_address)) == -1) {
//     perror("Error binding server socket");
//     close(server_socket);
//     return 1;
//   }

//   //监听连接
//   if (listen(server_socket, 5) == -1) {
//     perror("Error listening on server socket");
//     close(server_socket);
//     return 1;
//   }

//   //创建epoll实例
//   int epoll_fd = epoll_create1(0);
//   if (epoll_fd == -1) {
//     perror("Error creating epoll instance");
//     close(server_socket);
//     return 1;
//   }

//   //添加服务器套接字到epoll实例
//   epoll_event event;
//   event.events = EPOLLIN;
//   event.data.fd = server_socket;

//   if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
//     perror("Error adding server socket to epoll");
//     close(epoll_fd);
//     close(server_socket);
//     return 1;
//   }

//   epoll_event events[MAX_EVENTS];

//   while (true) {
//     //使用epoll_wait函数等待事件的发生，将事件存储在events数组中
//     int numEvents = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
//     if (numEvents == -1) {
//       perror("Error waiting for events");
//       break;
//     }

//     //处理事件
//     for (int i = 0; i < numEvents; ++i) {
//       std::cout << "事件" << numEvents << "发生" << std::endl;
//       int current_fd = events[i].data.fd;

//       if (current_fd == server_socket) {
//         //新连接事件
//         sockaddr_in client_address;
//         socklen_t client_addr_len = sizeof(client_address);
//         int client_socket =
//             accept(server_socket, reinterpret_cast<sockaddr
//             *>(&client_address),
//                    &client_addr_len);
//         std::cout << "Accepted connection from "
//                   << inet_ntoa(client_address.sin_addr) << std::endl;

//         // 接收客户端发送的会话 ID
//         char session_id[64];
//         ssize_t bytesRead =
//             recv(client_socket, session_id, sizeof(session_id) - 1, 0);
//         if (bytesRead <= 0) {
//           // 处理连接断开的情况
//           epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
//           close(client_socket);
//           continue;
//         }
//         session_id[bytesRead] = '\0';

//         // 存储会话ID和客户端套接字的映射关系
//         int session_id_int = std::atoi(session_id);
//         sessionMap[client_socket] = session_id_int;
//         rSessionMap[session_id_int] = client_socket;
//         std::cout << "Assigned session ID: " << session_id_int << std::endl;
//         event.events = EPOLLIN;
//         event.data.fd = client_socket;

//         //将客户端套接字添加到 epoll 实例，关注事件为可读
//         if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
//         {
//           perror("Error adding client socket to epoll");
//           close(client_socket);
//           continue;
//         }
//       } else {
//         //数据可读事件
//         int client_id = sessionMap[current_fd];
//         int target_id =
//             (client_id % 2 == 0)
//                 ? (client_id + 1)
//                 : (client_id - 1); //偶数加一,奇数减一，获取目标客户端id

//         // 处理来自客户端的数据
//         char buffer[1024];
//         ssize_t bytesRead = recv(current_fd, buffer, sizeof(buffer) - 1, 0);

//         //客户端断开连接，执行相应的处理
//         if (bytesRead <= 0) {
//           epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
//           close(current_fd);

//           //删除映射关系
//           auto it1 = sessionMap.find(current_fd);
//           if (it1 != sessionMap.end()) {
//             std::cout << "客户端断开连接: " << it1->second << std::endl;
//             sessionMap.erase(it1);
//           }

//           //删除反向映射
//           auto it2 = rSessionMap.find(client_id);
//           if (it2 != sessionMap.end()) {
//             std::cout << "对应服务器套接字: " << it2->second << std::endl;
//             rSessionMap.erase(it2);
//           }
//           continue;
//         }
//         buffer[bytesRead] = '\0';
//         std::cout << "Received message from client" << client_id << ": "
//                   << buffer << std::endl;

//         //数据发送给目标客户端
//         send(rSessionMap[target_id], buffer, bytesRead, 0);
//         std::cout << "Send message: " << buffer << " to client " << target_id
//                   << std::endl;
//       }
//     }
//   }

//   close(epoll_fd);
//   close(server_socket);

//   return 0;
// }
//}
