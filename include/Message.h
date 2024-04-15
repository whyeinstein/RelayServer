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

  // 将报文发送到指定的套接字
  void sendToSocket(int socket) {
    // 将报文头部和数据转换为 char 数组
    std::vector<char> buffer(sizeof(MessageHeader) + data_.size());
    std::memcpy(buffer.data(), &header_, sizeof(MessageHeader));
    std::memcpy(buffer.data() + sizeof(MessageHeader), data_.data(),
                data_.size());

    // 发送数据到套接字
    send(socket, buffer.data(), buffer.size(), 0);
  }

  // 从指定的套接字接收报文
  static Message receiveFromSocket(int socket) {
    // 从套接字接收数据
    std::vector<char> buffer(sizeof(MessageHeader));
    recv(socket, buffer.data(), sizeof(MessageHeader), 0);

    // 解析报文头部
    MessageHeader header;
    std::memcpy(&header, buffer.data(), sizeof(MessageHeader));

    // 从套接字接收报文数据
    buffer.resize(sizeof(MessageHeader) + header.messageLength);
    recv(socket, buffer.data() + sizeof(MessageHeader), header.messageLength,
         0);

    // 创建 Message 对象并返回
    return Message(header.messageType, buffer.data() + sizeof(MessageHeader),
                   header.messageLength);
  }

 private:
  MessageHeader header_;
  std::vector<char> data_;  //数组容器，方便动态调整大小
};
#endif