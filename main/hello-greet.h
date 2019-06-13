#ifndef LIB_HELLO_GREET_H_
#define LIB_HELLO_GREET_H_

#include <string>
#include <gflags/gflags.h>
#include <libudev.h>

DECLARE_string(npc);

std::string get_greet(const std::string &thing);
int device_view(const std::string& dev_name);

#endif
