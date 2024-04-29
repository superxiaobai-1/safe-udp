#pragma once

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
