#ifndef MESSAGE
#define MESSAGE

#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 报文头部结构体
struct MessageHeader {
  uint8_t messageType;     // 报文类型
  uint32_t messageLength;  // 报文长度
  int age_ = 0;            // 报文转发次数
  int id_ = -1;            // 报文发送者
};

//报文类
class Message {
 public:
  // 构造函数，初始化报文头部和数据
  Message(uint8_t messageType, const char *data, uint32_t dataLength,
          const std::string &id) {
    header_.messageType = messageType;
    header_.messageLength = dataLength;
    header_.id_ = std::stoi(id);

    data_.insert(data_.end(), data, data + dataLength);
  }

  // 获取报文头部
  const MessageHeader &getHeader() const { return header_; }

  // 获取报文数据
  const std::vector<char> &getData() const { return data_; }

  // 序列化
  // 在  Message 类中添加一个序列化方法
  std::vector<char> serialize() const {
    std::vector<char> result;

    // 将头部数据添加到结果中
    const char *headerData = reinterpret_cast<const char *>(&header_);
    result.insert(result.end(), headerData, headerData + sizeof(header_));

    // 将数据添加到结果中
    result.insert(result.end(), data_.begin(), data_.end());

    return result;
  }

 private:
  MessageHeader header_;
  std::vector<char> data_;  //数组容器，方便动态调整大小
};
#endif