#include <atomic>

#include "Message.h"
#include "Server.h"
#include "Stress.h"

bool Server::running = true;

int main() {
  Server myServer("127.0.0.1", 8849);
  myServer.run();
  return 0;
}
