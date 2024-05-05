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

const char *server_ip = "192.168.31.214";
// const char *server_ip = "127.0.0.1";
int server_port = 8849;

/**
 * @brief 压力发生器
 * @param argv[0] - 会话数量
 * @param argv[1] - 报文长度
 * 1 - 10B
 * 2 - 128B
 * 3 - 1K
 * 4 - 16K
 * 5 - 32K
 * 6 - 64K
 * 7 - 128K
 */
int main(int argc, char *argv[]) {
  int is_test = 0;
  int loop_num = 10000;

  std::cout << "请输入端口" << std::endl;
  std::cin >> server_port;
  size_t max_msg_size = 16 * 1024;  // 10B, 128B, 1K, 16K, 32K, 64K,128K
  int num_clients = 10000;
  std::cout << "输入客户端数量" << std::endl;
  std::cin >> num_clients;
  std::cout << "输入是否压测" << std::endl;
  std::cin >> is_test;
  std::cout << "输入循环次数" << std::endl;
  std::cin >> loop_num;

  if (argc > 1) {
    num_clients = std::atoi(argv[1]);
  }
  if (argc > 2) {
    switch (std::atoi(argv[2])) {
      case 1:
        max_msg_size = 10;
        break;
      case 2:
        max_msg_size = 128;
        break;
      case 3:
        max_msg_size = 1024;
        break;
      case 4:
        max_msg_size = 16 * 1024;
        break;
      case 5:
        max_msg_size = 32 * 1024;
        break;
      case 6:
        max_msg_size = 64 * 1024;
        break;
      case 7:
        max_msg_size = 128 * 1024;
        break;
      default:
        max_msg_size = 1024;
        break;
    }
  }
  if (argc > 3) {
    loop_num = std::atoi(argv[3]);
  }

  char **pdata = new char *[num_clients];  // 分配指针数组
  // std::fill_n(dynamic_client_alive, 2000, false);

  int min_cli_num = 100;  // 动态客户端最低数量

  for (int i = 0; i < num_clients; i++) {
    pdata[i] = new char[max_msg_size];  // 为每个字符串分配内存
    snprintf(pdata[i], max_msg_size, "Hello from Client %d",
             i);  // 使用 snprintf 以防止溢出

    // 填充剩余长度
    int len = strlen(pdata[i]);
    for (int j = len; j < max_msg_size - 1; j++) {
      pdata[i][j] = ' ';  // 使用空格填充
    }
    pdata[i][max_msg_size - 1] = '\0';  // 确保字符串结束
  }

  LoadGenerator load_generator;
  std::vector<Client> clients;
  std::vector<Client> dynamic_clients;
  auto program_start_time = std::chrono::steady_clock::now();
  auto program_end_time = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds;

  // 固定数目客户端
  for (int i = 0; i < num_clients; ++i) {
    clients.emplace_back(Client(std::to_string(i), 1, pdata[i],
                                std::strlen(pdata[i]), max_msg_size));
    clients.back().ConnectToServer(server_ip, server_port);
  }
  std::cout << "连接完成" << std::endl;
  sleep(2);

  // 若进行压测则置loop_num为-1，event_loop不主动结束
  if (is_test == 1) {
    loop_num = -1;
  }
  load_generator.start_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < (int)clients.size(); ++i) {
    load_generator.AddClient(clients[i], loop_num);
  }

  // 动态客户端
  while (is_test == 1) {
    if (dynamic_clients.size() < min_cli_num) {
      // 统计压测时间
      program_end_time = std::chrono::steady_clock::now();
      elapsed_seconds = program_end_time - program_start_time;
      if (elapsed_seconds.count() >= 12.0 * 60.0 * 60.0) {  // 12小时
        break;
      }

      // 清理执行完的线程
      for (auto &tuple : load_generator.dynamic_client_threads) {
        // 检查结束标志
        if (std::get<1>(tuple)) {
          if (std::get<0>(tuple).joinable()) {
            // 回收线程
            std::get<0>(tuple).join();

            // 销毁client对象
            dynamic_clients.erase(dynamic_clients.begin() + std::get<2>(tuple));

            // 对应id置为false，代表可以创建该id的客户端
            load_generator.dynamic_client_alive[std::get<3>(tuple)] = false;
          }
        }
      }

      // 随机新生成客户端
      std::default_random_engine random_generator;
      std::uniform_int_distribution<int> distribution(400, 800);
      int randomNumber = distribution(random_generator);

      // 保证为偶数
      if (randomNumber % 2 == 1) {
        randomNumber += 1;
      }
      int count = 0;

      // 生成新的客户端并加入线程数组
      for (int i = num_clients;
           i < (num_clients + 2000) || count < randomNumber; i += 2) {
        if (!load_generator.dynamic_client_alive[i] &&
            !load_generator.dynamic_client_alive[i + 1]) {
          dynamic_clients.emplace_back(Client(std::to_string(i), 1, pdata[i],
                                              std::strlen(pdata[i]),
                                              max_msg_size));
          dynamic_clients.back().ConnectToServer(server_ip, server_port);

          dynamic_clients.emplace_back(
              Client(std::to_string(i + 1), 1, pdata[i + 1],
                     std::strlen(pdata[i + 1]), max_msg_size));
          dynamic_clients.back().ConnectToServer(server_ip, server_port);

          load_generator.AddDynamicClient(
              dynamic_clients[dynamic_clients.size() - 2],
              dynamic_clients.size() - 2, i);
          load_generator.AddDynamicClient(
              dynamic_clients[dynamic_clients.size() - 1],
              dynamic_clients.size() - 1, i + 1);

          count += 2;
        }
      }
    }

    // 一秒钟检查一次
    sleep(1);
  }

  // 程序执行完毕清理残余动态客户端线程
  if (is_test == 1) {
    for (auto &tuple : load_generator.dynamic_client_threads) {
      if (std::get<0>(tuple).joinable()) {
        std::get<0>(tuple).join();
      }
    }
  }

  std::cout << "开始结束线程" << std::endl;
  load_generator.WaitAll();
  std::cout << "线程结束完毕" << std::endl;

  if (is_test == 1) {
    std::cout << "压力发生器运行时间："
              << elapsed_seconds.count() / (60.0 * 60.0 * 12) << std::endl;
  }

  for (int i = 0; i < num_clients; i++) {
    delete[] pdata[i];  // 释放每个字符串的内存
  }
  delete[] pdata;  // 释放指针数组的内存

  return 0;
}