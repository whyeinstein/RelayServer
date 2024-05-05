#ifndef LOADGENERATOR
#define LOADGENERATOR

#ifndef DEBUG
// #define DEBUG
#endif

#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>

#include "Client.h"
class LoadGenerator {
 public:
  std::chrono::high_resolution_clock::time_point start_time;  // 记录开始时间点
  std::chrono::high_resolution_clock::time_point end_time;  // 记录结束时间点
  std::chrono::milliseconds diff_time;        // 计算耗费的时间
  bool dynamic_client_alive[2000] = {false};  // 动态客户端id是否可用
  std::vector<std::tuple<std::thread, bool, int, int>>
      dynamic_client_threads;  // 动态线程数组

  // 固定运行客户端
  void AddClient(Client &client, int loop_num = 10000);

  // 动态运行客户端
  void AddDynamicClient(Client &client, int clients_index, int client_id);

  // 等待固定线程结束
  void WaitAll();

 private:
  std::vector<std::thread> client_threads;  // 线程数组
  // std::vector<std::thread> dynamic_client_threads;

  double avr_time_per_msg_ = 0;  // 每报文平均延迟
  int client_num_ = 0;           // 线程数目
  double all_msg_count = 0;      // 转发报文总数
  double avr_msg = 0;            // 每秒平均转发报文数
  std::mutex mtx;
};
#endif