#pragma once

#include <sys/time.h>
#include <time.h>

namespace safe_udp {
class SlidWinBuffer {
 public:
  SlidWinBuffer() {}
  ~SlidWinBuffer() {}

  int first_byte_;
  int data_length_;
  int seq_num_;
  struct timeval time_sent_;
};
}  // namespace safe_udp