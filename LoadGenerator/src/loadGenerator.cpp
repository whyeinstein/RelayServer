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

bool Client::running = true;

void LoadGenerator::AddClient(Client &client, int loop_num) {
  // 创建一个新线程来处理客户端的事件
  client_threads.push_back(std::thread([&client, this, loop_num]() {
    client.EventLoop(loop_num);

    // 记录当前线程的数据
    std::lock_guard<std::mutex> lock(mtx);
    avr_time_per_msg_ += client.GetAvrTimePerMsg();
    client_num_++;
    all_msg_count += client.GetAllMsgCount();
  }));
}

void LoadGenerator::AddDynamicClient(Client &client, int clients_index,
                                     int client_id) {
  std::default_random_engine random_generator;
  std::uniform_int_distribution<int> distribution(100, 200);
  int randomNumber = distribution(random_generator);
  if (randomNumber % 2 == 1) {
    randomNumber += 1;  // 保证为偶数
  }

  // 创建一个 pair 并添加到数组中
  dynamic_client_threads.push_back(
      std::make_tuple(std::thread(), false, clients_index, client_id));

  // 启动线程
  std::get<0>(dynamic_client_threads.back()) =
      std::thread([&client, this, &randomNumber]() {
        client.EventLoop(randomNumber);

        // 记录当前线程的数据
        std::lock_guard<std::mutex> lock(mtx);
        avr_time_per_msg_ += client.GetAvrTimePerMsg();
        client_num_++;
        all_msg_count += client.GetAllMsgCount();

        // 线程结束，将结束标志设为 true
        std::get<1>(dynamic_client_threads.back()) = true;
      });
}

void LoadGenerator::WaitAll() {
  for (auto &t : client_threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  // 总用时
  end_time = std::chrono::high_resolution_clock::now();
  diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                    start_time);

  // 每秒平均转发报文数
  avr_msg =
      1000 * all_msg_count /
      (diff_time.count() +
       std::numeric_limits<
           double>::epsilon());  //乘1000将毫秒转化为秒，分母加极小值防止为0

  std::cout << "每秒平均转发报文数：" << avr_msg << "" << std::endl;

  // 计算总平均时间
  avr_time_per_msg_ = avr_time_per_msg_ / client_num_;

  std::cout << "每报文平均延迟：" << avr_time_per_msg_ << "ms" << std::endl;
}
