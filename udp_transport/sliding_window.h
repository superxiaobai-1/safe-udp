#pragma once

#include <vector>

#include "buffer.h"

class SlidingWindow {
 public:
  SlidingWindow();
  ~SlidingWindow();

  int AddToBuffer(const SlidWinBuffer&  buffer);

  std::vector<SlidWinBuffer> sliding_window_buffers_;
  int last_packet_sent_;
  int last_acked_packet_;
  // 第一个未确认的数据包的指针
  int send_base_;
  int dup_ack_;
};
