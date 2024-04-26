#ifndef LOADGENERATOR
#define LOADGENERATOR

#include <thread>
#include <vector>

#include "Client.h"
class LoadGenerator {
 private:
  std::vector<std::thread> client_threads;

 public:
  void AddClient(Client& client) {
    // 创建一个新线程来处理客户端的事件
    client_threads.push_back(std::thread([&client]() { client.EventLoop(); }));
  }

  void WaitAll() {
    for (auto& t : client_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

  // 其他方法...
};
#endif