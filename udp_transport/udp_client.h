#pragma once

#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "data_segment.h"

class UdpClient {
 public:
  UdpClient();

  void SendFileRequest(const std::string& file_name);

  void CreateSocketAndServerConnection(const std::string& server_address,
                                       const std::string& port);

  int initial_seq_number_;
  bool is_packet_drop_;
  bool is_delay_;
  int prob_value_;

  int last_in_order_packet_;
  int last_packet_received_;
  int receiver_window_;

 private:
  void send_ack(int ackNumber);
  void insert(int index, const DataSegment& data_segment);
  int add_to_data_segment_vector(const DataSegment& data_segment);

  int sockfd_;
  int seq_number_;
  int ack_number_;
  int16_t length_;
  struct sockaddr_in server_address_;
  std::vector<DataSegment> data_segments_;
};
