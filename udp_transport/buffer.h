#pragma once

#include <sys/time.h>
#include <time.h>

class SlidWinBuffer {
 public:
  SlidWinBuffer() {}
  ~SlidWinBuffer() {}

  int first_byte_;
  int data_length_;
  int seq_num_;
  struct timeval time_sent_;
};
