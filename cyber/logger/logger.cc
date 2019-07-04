/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "cyber/logger/logger.h"

#include <glog/logging.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <utility>

#include "cyber/base/macros.h"
#include "cyber/logger/log_file_object.h"
#include "cyber/logger/logger_util.h"

namespace apollo {
namespace cyber {
namespace logger {


std::unordered_map<std::string, LogFileObjectPtr> Logger::moduleLoggerMap;

Logger::Logger(google::base::Logger* wrapped) : wrapped_(wrapped) {}

Logger::~Logger() {
}

void Logger::Write(bool force_flush, time_t timestamp, const char* message,
                   int message_len) {
  std::string log_message = std::string(message, message_len);
  std::string module_name;
  // set the same bracket as the bracket in log.h
  FindModuleName(&log_message, &module_name);

  LogFileObjectPtr file_object;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = moduleLoggerMap.find(module_name);
    if (iter != moduleLoggerMap.end()) {
      file_object = iter->second;
    } else {
      file_object = std::make_shared<LogFileObject>(google::INFO, module_name.c_str());
      file_object->SetSymlinkBasename(module_name.c_str());
      moduleLoggerMap.emplace(module_name, file_object);
    }
  }
  file_object->Write(force_flush, timestamp, log_message.c_str(),
    static_cast<int>(log_message.size()));
}

void Logger::Flush() { wrapped_->Flush(); }

uint32_t Logger::LogSize() { return wrapped_->LogSize(); }

}  // namespace logger
}  // namespace cyber
}  // namespace apollo
