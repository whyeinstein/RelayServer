#include "LoadGenerator.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "Client.h"
#include "LoadGenerator.h"
#include "Message.h"

const char *server_ip = "127.0.0.1";
const int server_port = 8849;

int main() {
  size_t max_msg_size = 10 * 1024;  // 10KB
  int num_clients = 10000;
  std::cout << "输入客户端数量" << std::endl;
  std::cin >> num_clients;

  int max_string_length = 50;
  char **pdata = new char *[num_clients];  // 分配指针数组

  for (int i = 0; i < num_clients; i++) {
    pdata[i] = new char[max_string_length];  // 为每个字符串分配内存
    snprintf(pdata[i], max_string_length, "Hello from Client %d",
             i);  // 使用 snprintf 以防止溢出
  }

  LoadGenerator load_generator;

  std::vector<Client> clients;

  for (int i = 0; i < num_clients; ++i) {
    clients.emplace_back(Client(std::to_string(i), 1, pdata[i],
                                std::strlen(pdata[i]), max_msg_size));
    clients.back().ConnectToServer(server_ip, server_port);
  }
  std::cout << "连接完成" << std::endl;
  sleep(5);

  for (int i = 0; i < (int)clients.size(); ++i) {
    load_generator.AddClient(clients[i]);
  }

  load_generator.WaitAll();

  for (int i = 0; i < num_clients; i++) {
    delete[] pdata[i];  // 释放每个字符串的内存
  }
  delete[] pdata;  // 释放指针数组的内存

  return 0;
}
// Message message1(1, data[0], std::strlen(data[0]));  // 1 表示数据报文
// Message message2(1, data[1], std::strlen(data[1]));  // 1 表示数据报文
// Message message3(1, data[2], std::strlen(data[2]));  // 1 表示数据报文
// Message message4(1, data[3], std::strlen(data[3]));  // 1 表示数据报文

// const char *data[] = {"Hello from Client 0", "Hello from Client 1",
//                       "Hello from Client 2", "Hello from Client 3"};
