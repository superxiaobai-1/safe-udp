#pragma once

#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include "data_segment.h"

namespace safe_udp {
constexpr char CLIENT_FILE_PATH[] = "/work/files/client_files/";

class UdpClient {
 public:
  UdpClient();
  ~UdpClient() { close(sockfd_); }

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
  bool fin_flag_received_;

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
}  // namespace safe_udp