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

#include "cyber/io/poller.h"
#include <gtest/gtest.h>
#include "cyber/common/log.h"
#include <fcntl.h>
#include <unistd.h>
#include <thread>

// #include "cyber/init.h"

using apollo::cyber::io::Poller;
using apollo::cyber::io::PollRequest;
using apollo::cyber::io::PollResponse;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  auto poller = Poller::Instance();
  EXPECT_TRUE(poller != nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));


  int pipe_fd[2] = {-1, -1};
  EXPECT_TRUE(0 == pipe2(pipe_fd, O_NONBLOCK));
  AINFO << "Test pipe fd=" << pipe_fd[0];

  int ref_pipe[2] = {-1, -1};
  EXPECT_TRUE(0 == pipe2(ref_pipe, O_NONBLOCK));
  AINFO << "Reference pipe fd=" << ref_pipe[0];

  PollRequest ref_req;
  ref_req.fd = ref_pipe[0];
  ref_req.events = EPOLLIN | EPOLLET;
  ref_req.timeout_ms = 0;
  ref_req.callback = [](const PollResponse&) {};

  PollRequest request;
  request.fd = pipe_fd[0];
  request.events = EPOLLIN | EPOLLET;
  request.timeout_ms = 0;

  PollResponse response(123);
  request.callback = [&response](const PollResponse& rsp) { response = rsp; };

  EXPECT_TRUE(poller->Register(request));
  EXPECT_TRUE(poller->Register(ref_req));
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_EQ(response.events, 0);

  response.events = 123;
  request.timeout_ms = 1;
  EXPECT_TRUE(poller->Register(ref_req));
  EXPECT_TRUE(poller->Register(request));
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  EXPECT_EQ(response.events, 0);
  AINFO << "==================";
  // timeout_ms is 50
  response.events = 123;
  request.timeout_ms = 50;
  EXPECT_TRUE(poller->Register(request));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(response.events, 123);
  std::this_thread::sleep_for(std::chrono::milliseconds(35));
  EXPECT_EQ(response.events, 0);

  // timeout_ms is 200
  response.events = 123;
  request.timeout_ms = 200;
  EXPECT_TRUE(poller->Register(request));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(response.events, 123);
  char msg = 'C';
  ssize_t res = 0;
  int try_num = 3;
  do {
    --try_num;
    res = write(pipe_fd[1], &msg, 1);
  } while (res <= 0 && try_num >= 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_NE(response.events & EPOLLIN, 0);
  // EXPECT_TRUE(poller->Unregister(request));
  poller->Shutdown();

  close(pipe_fd[0]);
  close(pipe_fd[1]);

}
