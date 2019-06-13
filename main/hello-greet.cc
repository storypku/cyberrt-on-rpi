#include "hello-greet.h"
#include <sys/stat.h>
#include <unistd.h>
#include <glog/logging.h>

DEFINE_string(npc, "BlahblahBloodMoon", "Non-Player Character");

std::string get_greet(const std::string& who) {
  LOG(WARNING) << "NPC time: " << FLAGS_npc;
  return "Hello " + who;
}

int device_view(const std::string& dev_name) {
    struct udev* udev = udev_new();
    if (udev == nullptr) {
        LOG(ERROR) << "udev_new() failed";
        return -1;
    }
    struct stat statbuf = {};
    if (stat(dev_name.c_str(), &statbuf) != 0) {
        LOG(ERROR) << "No such file or directory: " << dev_name;
        udev_unref(udev);
        return -1;
    }

    if (!S_ISCHR(statbuf.st_mode)) {
        LOG(ERROR) << dev_name << " is not a character device";
        udev_unref(udev);
        return -2;
    }

    char dev_id[64];
    snprintf(dev_id, sizeof(dev_id), "c%d:%d",
             major(statbuf.st_rdev), minor(statbuf.st_rdev));

    struct udev_device* device = udev_device_new_from_device_id(udev, dev_id);
    if (device == nullptr) {
        LOG(ERROR) << "No device named " << dev_name;
        udev_unref(udev);
        return -3;
    }

    LOG(INFO) << "Successful visit to " << dev_name;
    udev_device_unref(device);
    udev_unref(udev);
    return 0;
}
