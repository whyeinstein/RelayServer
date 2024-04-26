#ifndef MESSAGE
#define MESSAGE

#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <vector>

// 报文头部结构体
struct MessageHeader {
  uint8_t messageType;     // 报文类型
  uint32_t messageLength;  // 报文长度
};

//报文类
class Message {
 public:
  // 构造函数，初始化报文头部和数据
  Message(uint8_t messageType, const char *data, uint32_t dataLength) {
    header_.messageType = messageType;
    header_.messageLength = dataLength;
    data_.insert(data_.end(), data, data + dataLength);
  }

  // 获取报文头部
  const MessageHeader &getHeader() const { return header_; }

  // 获取报文数据
  const std::vector<char> &getData() const { return data_; }

 private:
  MessageHeader header_;
  std::vector<char> data_;  //数组容器，方便动态调整大小
};
#endif