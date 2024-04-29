#include "packet_statistics.h"

namespace safe_udp {
PacketStatistics::PacketStatistics() {
  slow_start_packet_sent_count_ = 0;
  cong_avd_packet_sent_count_ = 0;
  slow_start_packet_rx_count_ = 0;
  cong_avd_packet_rx_count_ = 0;
  retransmit_count_ = 0;
}

PacketStatistics::~PacketStatistics() {}
}  // namespace safe_udp