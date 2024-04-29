#pragma once

#include <cstdint>
#include <cstdlib>

namespace safe_udp {
constexpr int MAX_PACKET_SIZE = 1472;
constexpr int MAX_DATA_SIZE = 1460;
constexpr int HEADER_LENGTH = 12;

class DataSegment {
 public:
  DataSegment();
  ~DataSegment() {
    if (final_packet_ != nullptr) {
      free(final_packet_);
    }
  }

  char *SerializeToCharArray();
  void DeserializeToDataSegment(unsigned char *data_segment, int length);

  int seq_number_;
  int ack_number_;
  bool ack_flag_;
  bool fin_flag_;
  uint16_t length_;
  char *data_ = nullptr;

 private:
  uint32_t convert_to_uint32(unsigned char *buffer, int start_index);
  bool convert_to_bool(unsigned char *buffer, int index);
  uint16_t convert_to_uint16(unsigned char *buffer, int start_index);
  char *final_packet_ = nullptr;
};
}  // namespace safe_udp