#ifndef CLIENTMESSAGE
#define CLIENTMESSAGE

#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <vector>

// 报文头部结构体
struct ClientMessageHeader {
  uint8_t messageType;     // 报文类型
  uint32_t messageLength;  // 报文长度
};

//报文类
class ClientMessage {
 public:
  // 构造函数，初始化报文头部和数据
  ClientMessage(uint8_t messageType, const char *data, uint32_t dataLength) {
    header_.messageType = messageType;
    header_.messageLength = dataLength;
    data_.insert(data_.end(), data, data + dataLength);
  }

  // 获取报文头部
  const ClientMessageHeader &getHeader() const { return header_; }

  // 获取报文数据
  const std::vector<char> &getData() const { return data_; }

 private:
  ClientMessageHeader header_;
  std::vector<char> data_;  //数组容器，方便动态调整大小
};
#endif
