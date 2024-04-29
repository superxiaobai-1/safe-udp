#pragma once

namespace safe_udp {
class PacketStatistics {
 public:
  PacketStatistics();
  virtual ~PacketStatistics();

  int slow_start_packet_sent_count_;
  int cong_avd_packet_sent_count_;
  int slow_start_packet_rx_count_;
  int cong_avd_packet_rx_count_;
  int retransmit_count_;
};
}  // namespace safe_udp
