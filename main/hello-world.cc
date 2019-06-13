#include "hello-greet.h"
#include <ctime>
#include <iostream>
#include <string>
#include <glog/logging.h>

void print_localtime() {
  std::time_t result = std::time(nullptr);
  std::cout << std::asctime(std::localtime(&result));
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  LOG(ERROR) << "Hello world from glog!";
  std::string who = "world";
  if (argc > 1) {
    who = argv[1];
  }
  auto msg = get_greet(who);
  std::cout << msg << std::endl;
  print_localtime();

  who = "/dev/tty10";
  if (argc > 2) {
      who = argv[2];
  }
  device_view(who);
  return 0;
}
