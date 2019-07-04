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

#include "cyber/logger/async_logger.h"

#include <stdlib.h>
#include <string>
#include <thread>

#include "cyber/base/macros.h"
#include "cyber/logger/logger_util.h"

namespace apollo {
namespace cyber {
namespace logger {

std::unordered_map<std::string, LogFileObjectPtr> AsyncLogger::moduleLoggerMap;

AsyncLogger::AsyncLogger(google::base::Logger* wrapped, int max_buffer_bytes)
    : max_buffer_bytes_(max_buffer_bytes),
      wrapped_(wrapped),
      active_buf_(std::make_unique<Buffer>()),
      flushing_buf_(std::make_unique<Buffer>()) {
  if (max_buffer_bytes_ <= 0) {
    max_buffer_bytes_ = 2 * 1024 * 1024;
  }
}

AsyncLogger::~AsyncLogger() {
  Stop();
}

void AsyncLogger::Start() {
  CHECK_EQ(state_, INITTED);
  state_ = RUNNING;
  thread_ = std::thread(&AsyncLogger::RunThread, this);
  // std::cout << "Async Logger Start!" << std::endl;
}

void AsyncLogger::Stop() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_EQ(state_, RUNNING);
    state_ = STOPPED;
  }
  wake_flusher_cv_.notify_one();
  thread_.join();
  CHECK(active_buf_->messages.empty());
  CHECK(flushing_buf_->messages.empty());
  // std::cout << "Async Logger Stop!" << std::endl;
}

void AsyncLogger::Write(bool force_flush, time_t timestamp, const char* message,
                        int message_len) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // drop message when active buffer full
    if (unlikely(BufferFull(*active_buf_))) {
      return;
    }

    if (state_ != RUNNING) {
      // std::cout << "Async Logger not running!" << std::endl;
      return;
    }

    switch (message[0]) {
      case 'F': {
        active_buf_->add(Msg(timestamp, std::string(message, message_len), 3),
                         force_flush);
        break;
      }
      case 'E': {
        active_buf_->add(Msg(timestamp, std::string(message, message_len), 2),
                         force_flush);
        break;
      }
      case 'W': {
        active_buf_->add(Msg(timestamp, std::string(message, message_len), 1),
                         force_flush);
        break;
      }
      case 'I': {
        active_buf_->add(Msg(timestamp, std::string(message, message_len), 0),
                         force_flush);
        break;
      }
      default: {
        active_buf_->add(Msg(timestamp, std::string(message, message_len), -1),
                         force_flush);
      }
    }
  }
  wake_flusher_cv_.notify_one();
}

void AsyncLogger::Flush() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (state_ != RUNNING) {
    // std::cout << "Async Logger not running!" << std::endl;
    return;
  }

  // Wake up the writer thread at least twice.
  // This ensures both buffers were completely flushed.
  uint64_t orig_flush_count = flush_count_;
  while (flush_count_ < (orig_flush_count + 2) && state_ == RUNNING) {
    active_buf_->flush = true;
    wake_flusher_cv_.notify_one();
    flush_complete_cv_.wait(lock);
  }
}

uint32_t AsyncLogger::LogSize() { return wrapped_->LogSize(); }

void AsyncLogger::RunThread() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (state_ == RUNNING || active_buf_->needs_flush_or_write()) {
    while (!active_buf_->needs_flush_or_write() && state_ == RUNNING) {
      if (wake_flusher_cv_.wait_for(lock, std::chrono::seconds(2)) ==
          std::cv_status::timeout) {
        active_buf_->flush = true;
      }
    }

    active_buf_.swap(flushing_buf_);
    lock.unlock();

    for (auto& msg : flushing_buf_->messages) {
      std::string module_name;
      FindModuleName(&msg.message, &module_name);

      LogFileObjectPtr fileobject;
      {
        auto iter = moduleLoggerMap.find(module_name);
        if (iter != moduleLoggerMap.end()) {
            fileobject = iter->second;
        } else {
            fileobject = std::make_shared<LogFileObject>(google::INFO, module_name.c_str());
            fileobject->SetSymlinkBasename(module_name.c_str());
            moduleLoggerMap.emplace(module_name, fileobject);
        }
      }
      const bool should_flush = msg.level > 0;
      fileobject->Write(should_flush, msg.ts, msg.message.data(),
                        static_cast<int>(msg.message.size()));
    }

    if (flushing_buf_->flush) {
      for (auto& module_logger : moduleLoggerMap) {
        module_logger.second->Flush();
      }
    }
    flushing_buf_->clear();

    lock.lock();
    flush_count_++;
    // lock.unlock(); ???
    flush_complete_cv_.notify_all();
    // lock.lock(); ???
  }
}

bool AsyncLogger::BufferFull(const Buffer& buf) const {
  // We evenly divide our total buffer space into two buffers.
  return buf.size > (max_buffer_bytes_ / 2);
}

}  // namespace logger
}  // namespace cyber
}  // namespace apollo