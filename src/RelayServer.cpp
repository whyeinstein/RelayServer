#include <atomic>

#include "Message.h"
#include "Server.h"

bool Server::running = true;

int main() {
  int port = -1;
  std::cout << "请输入端口" << std::endl;
  std::cin >> port;
  Server myServer("127.0.0.1", port);
  myServer.run();
  return 0;
}
