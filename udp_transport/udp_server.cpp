#include "udp_server.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <glog/logging.h>

namespace safe_udp {
UdpServer::UdpServer() {
  sliding_window_ = std::make_unique<SlidingWindow>();
  packet_statistics_ = std::make_unique<PacketStatistics>();

  sockfd_ = 0;
  smoothed_rtt_ = 20000;
  smoothed_timeout_ = 30000;
  dev_rtt_ = 0;

  initial_seq_number_ = 67;
  start_byte_ = 0;

  ssthresh_ = 128;
  cwnd_ = 1;

  is_slow_start_ = true;
  is_cong_avd_ = false;
  is_fast_recovery_ = false;
}

int UdpServer::StartServer(int port) {
  int sfd;
  struct sockaddr_in server_addr;
  LOG(INFO) << "Starting the webserver... port: " << port;
  sfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sfd < 0) {
    LOG(ERROR) << " Failed to socket !!!";
    exit(0);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(port);

  if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    LOG(ERROR) << "binding error !!!";
    exit(0);
  }
  LOG(INFO) << "**Server Bind set to addr: " << server_addr.sin_addr.s_addr;
  LOG(INFO) << "**Server Bind set to port: " << server_addr.sin_port;
  LOG(INFO) << "**Server Bind set to family: " << server_addr.sin_family;
  LOG(INFO) << "Started successfully";
  sockfd_ = sfd;
  return sfd;
}

bool UdpServer::OpenFile(const std::string &file_name) {
  LOG(INFO) << "Opening the file " << file_name;

  file_.open(file_name.c_str(), std::ios::in);
  if (!this->file_.is_open()) {
    LOG(INFO) << "File: " << file_name << " opening failed";
    return false;
  } else {
    LOG(INFO) << "File: " << file_name << " opening success";
    return true;
  }
}

void UdpServer::StartFileTransfer() {
  LOG(INFO) << "Starting the file_ transfer ";

  file_.seekg(0, std::ios::end);
  file_length_ = file_.tellg();
  file_.seekg(0, std::ios::beg);

  send();
}

void UdpServer::SendError() {
  std::string error("FILE NOT FOUND");
  sendto(sockfd_, error.c_str(), error.size(), 0,
         (struct sockaddr *)&cli_address_, sizeof(cli_address_));
}

void UdpServer::send() {
  LOG(INFO) << "Entering Send()";

  int sent_count = 1;
  int sent_count_limit = 1;

  struct timeval process_start_time;
  struct timeval process_end_time;
  gettimeofday(&process_start_time, NULL);

  if (sliding_window_->last_packet_sent_ == -1) {
    start_byte_ = 0;
  }

  while (start_byte_ <= file_length_) {
    fd_set rfds;
    struct timeval tv;
    int res;

    sent_count = 1;
    sent_count_limit = std::min(rwnd_, cwnd_);

    LOG(INFO) << "SEND START  !!!!";
    LOG(INFO) << "Before the window rwnd_: " << rwnd_ << " cwnd_: " << cwnd_
              << " window used: "
              << sliding_window_->last_packet_sent_ -
                     sliding_window_->last_acked_packet_;
    while (sliding_window_->last_packet_sent_ -
                   sliding_window_->last_acked_packet_ <=
               std::min(rwnd_, cwnd_) &&
           sent_count <= sent_count_limit) {
      send_packet(start_byte_ + initial_seq_number_, start_byte_);

      if (is_slow_start_) {
        // LOG(INFO) << "Window In SLOW START Window";
        packet_statistics_->slow_start_packet_sent_count_++;
      } else if (is_cong_avd_) {
        // LOG(INFO) << "Window In CONG AVD Window";
        packet_statistics_->cong_avd_packet_sent_count_++;
      }

      // break
      start_byte_ = start_byte_ + MAX_DATA_SIZE;
      if (start_byte_ > file_length_) {
        LOG(INFO) << "No more data left to be sent";
        break;
      }
      sent_count++;
    }

    LOG(INFO) << "SEND END !!!!!";

    // socket listen whith timeout
    FD_ZERO(&rfds);
    FD_SET(sockfd_, &rfds);

    tv.tv_sec = (int64_t)smoothed_timeout_ / 1000000;
    tv.tv_usec = smoothed_timeout_ - tv.tv_sec;

    LOG(INFO) << "SELECT START:" << smoothed_timeout_ << "!!!";
    while (true) {
      res = select(sockfd_ + 1, &rfds, NULL, NULL, &tv);
      if (res == -1) {
        LOG(ERROR) << "Error in select";
      } else if (res > 0) {  // ACK available event
        wait_for_ack();

        if (cwnd_ >= ssthresh_) {
          //慢启动---->拥塞避免
          LOG(INFO) << "CHANGE TO CONG AVD";
          is_cong_avd_ = true;
          is_slow_start_ = false;

          cwnd_ = 1;
          ssthresh_ = 64;
        }

        if (sliding_window_->last_acked_packet_ ==
            sliding_window_->last_packet_sent_) {
          if (is_slow_start_) {
            cwnd_ = cwnd_ * 2;
          } else {
            cwnd_ = cwnd_ + 1;
          }
          break;
        }
      } else {
        // 拥塞发生--超时重传
        LOG(INFO) << "Timeout occurred SELECT::" << smoothed_timeout_;
        // LOG(INFO) << "CHANGE TO SLOW START";
        ssthresh_ = cwnd_ / 2;
        if (ssthresh_ < 1) {
          ssthresh_ = 1;
        }
        cwnd_ = 1;

        // 重新开始慢启动
        if (is_fast_recovery_) {
          // LOG(INFO) << "CHANGE TO SLOW START from FastRecovery start_byte_:"
          //           << start_byte_;
          is_fast_recovery_ = false;
        }
        is_slow_start_ = true;
        is_cong_avd_ = false;

        // retransmit all unacked segments
        for (int i = sliding_window_->last_acked_packet_ + 1;
             i <= sliding_window_->last_packet_sent_; i++) {
          int retransmit_start_byte = 0;
          if (sliding_window_->last_acked_packet_ != -1) {
            retransmit_start_byte =
                sliding_window_
                    ->sliding_window_buffers_[sliding_window_
                                                  ->last_acked_packet_]
                    .first_byte_ +
                MAX_DATA_SIZE;
          }
          LOG(INFO) << "Timeout Retransmit seq number"
                    << retransmit_start_byte + initial_seq_number_;
          retransmit_segment(retransmit_start_byte);
          packet_statistics_->retransmit_count_++;
          LOG(INFO) << "Timeout: retransmission at " << retransmit_start_byte;
        }

        break;
      }
    }
    LOG(INFO) << "SELECT END !!!";
    LOG(INFO) << "current byte ::" << start_byte_ << " file_length_ "
              << file_length_;
  }

  gettimeofday(&process_end_time, NULL);

  int64_t total_time =
      (process_end_time.tv_sec * 1000000 + process_end_time.tv_usec) -
      (process_start_time.tv_sec * 1000000 + process_start_time.tv_usec);

  int total_packet_sent = packet_statistics_->slow_start_packet_sent_count_ +
                          packet_statistics_->cong_avd_packet_sent_count_;
  LOG(INFO) << "\n";
  LOG(INFO) << "========================================";
  LOG(INFO) << "Total Time: " << (float)total_time / pow(10, 6) << " secs";
  LOG(INFO) << "Statistics: 拥塞控制--慢启动: "
            << packet_statistics_->slow_start_packet_sent_count_
            << " 拥塞控制--拥塞避免: "
            << packet_statistics_->cong_avd_packet_sent_count_;
  LOG(INFO) << "Statistics: Slow start: "
            << ((float)packet_statistics_->slow_start_packet_sent_count_ /
                total_packet_sent) *
                   100
            << "% CongAvd: "
            << ((float)packet_statistics_->cong_avd_packet_sent_count_ /
                total_packet_sent) *
                   100
            << "%";
  LOG(INFO) << "Statistics: Retransmissions: "
            << packet_statistics_->retransmit_count_;
  LOG(INFO) << "========================================";
}

void UdpServer::send_packet(int seq_number, int start_byte) {
  bool lastPacket = false;
  int dataLength = 0;
  if (file_length_ <= start_byte + MAX_DATA_SIZE) {
    LOG(INFO) << "Last packet to be sent !!!";
    dataLength = file_length_ - start_byte;
    lastPacket = true;
  } else {
    dataLength = MAX_DATA_SIZE;
  }

  struct timeval time;

  gettimeofday(&time, NULL);

  if (sliding_window_->last_packet_sent_ != -1 &&
      start_byte <
          sliding_window_
              ->sliding_window_buffers_[sliding_window_->last_packet_sent_]
              .first_byte_) {
    for (int i = sliding_window_->last_acked_packet_ + 1;
         i < sliding_window_->last_packet_sent_; i++) {
      if (sliding_window_->sliding_window_buffers_[i].first_byte_ ==
          start_byte) {
        sliding_window_->sliding_window_buffers_[i].time_sent_ = time;
        break;
      }
    }

  } else {
    SlidWinBuffer slidingWindowBuffer;
    slidingWindowBuffer.first_byte_ = start_byte;
    slidingWindowBuffer.data_length_ = dataLength;
    slidingWindowBuffer.seq_num_ = initial_seq_number_ + start_byte;
    struct timeval time;
    gettimeofday(&time, NULL);
    slidingWindowBuffer.time_sent_ = time;
    sliding_window_->last_packet_sent_ =
        sliding_window_->AddToBuffer(slidingWindowBuffer);
  }
  read_file_and_send(lastPacket, start_byte, start_byte + dataLength);
}

void UdpServer::wait_for_ack() {
  unsigned char buffer[MAX_PACKET_SIZE];
  memset(buffer, 0, MAX_PACKET_SIZE);
  socklen_t addr_size;
  struct sockaddr_in client_address;
  addr_size = sizeof(client_address);
  int n = 0;
  int ack_number;
  while ((n = recvfrom(sockfd_, buffer, MAX_PACKET_SIZE, 0,
                       (struct sockaddr *)&client_address, &addr_size)) <= 0) {
  };
  DataSegment ack_segment;
  ack_segment.DeserializeToDataSegment(buffer, n);

  // LOG(INFO) << "ACK Received: ack_number " << ack_segment->ack_number_;
  SlidWinBuffer last_packet_acked_buffer =
      sliding_window_
          ->sliding_window_buffers_[sliding_window_->last_acked_packet_];
  if (ack_segment.ack_flag_) {
    if (ack_segment.ack_number_ == sliding_window_->send_base_) {
      LOG(INFO) << "DUP ACK Received: ack_number: " << ack_segment.ack_number_;
      sliding_window_->dup_ack_++;
      // 快速重传
      if (sliding_window_->dup_ack_ == 3) {
        packet_statistics_->retransmit_count_++;
        LOG(INFO) << "Fast Retransmit seq_number: " << ack_segment.ack_number_;
        retransmit_segment(ack_segment.ack_number_ - initial_seq_number_);
        sliding_window_->dup_ack_ = 0;
        if (cwnd_ > 1) {
          cwnd_ = cwnd_ / 2;
        }
        ssthresh_ = cwnd_;
        is_fast_recovery_ = true;
        // LOG(INFO) << "Change to fast Recovery ack_segment->ack_number:"
        //           << ack_segment->ack_number_;
      }

    } else if (ack_segment.ack_number_ > sliding_window_->send_base_) {
      if (is_fast_recovery_) {
        // LOG(INFO) << "Change to Cong Avoidance from fast recovery recv ack:"
        //           << ack_segment->ack_number_;
        cwnd_++;
        is_fast_recovery_ = false;
        is_cong_avd_ = true;
        is_slow_start_ = false;
      }

      sliding_window_->dup_ack_ = 0;
      sliding_window_->send_base_ = ack_segment.ack_number_;
      if (sliding_window_->last_acked_packet_ == -1) {
        sliding_window_->last_acked_packet_ = 0;
        last_packet_acked_buffer =
            sliding_window_
                ->sliding_window_buffers_[sliding_window_->last_acked_packet_];
      }
      ack_number = last_packet_acked_buffer.seq_num_ +
                   last_packet_acked_buffer.data_length_;

      while (ack_number < ack_segment.ack_number_) {
        sliding_window_->last_acked_packet_++;
        last_packet_acked_buffer =
            sliding_window_
                ->sliding_window_buffers_[sliding_window_->last_acked_packet_];
        ack_number = last_packet_acked_buffer.seq_num_ +
                     last_packet_acked_buffer.data_length_;
      }

      struct timeval startTime = last_packet_acked_buffer.time_sent_;
      struct timeval endTime;
      gettimeofday(&endTime, NULL);

      // LOG(INFO) << "seq num of last_acked "
      //           << last_packet_acked_buffer.seq_num_;
      calculate_rtt_and_time(startTime, endTime);
    }
    // LOG(INFO) << "sliding_window_->lastAckedPacket"
    //           << sliding_window_->last_acked_packet_;
  }
}

void UdpServer::calculate_rtt_and_time(struct timeval start_time,
                                       struct timeval end_time) {
  if (start_time.tv_sec == 0 && start_time.tv_usec == 0) {
    return;
  }
  long sample_rtt = (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                    (start_time.tv_sec * 1000000 + start_time.tv_usec);

  smoothed_rtt_ = smoothed_rtt_ + 0.125 * (sample_rtt - smoothed_rtt_);

  dev_rtt_ = 0.75 * dev_rtt_ + 0.25 * (abs(smoothed_rtt_ - sample_rtt));
  smoothed_timeout_ = smoothed_rtt_ + 4 * dev_rtt_;

  if (smoothed_timeout_ > 1000000) {
    smoothed_timeout_ = rand() % 30000;
  }
}

void UdpServer::retransmit_segment(int index_number) {
  for (int i = sliding_window_->last_acked_packet_ + 1;
       i < sliding_window_->last_packet_sent_; i++) {
    if (sliding_window_->sliding_window_buffers_[i].first_byte_ ==
        index_number) {
      struct timeval time;
      gettimeofday(&time, NULL);
      sliding_window_->sliding_window_buffers_[i].time_sent_ = time;
      break;
    }
  }

  read_file_and_send(false, index_number, index_number + MAX_DATA_SIZE);
}

void UdpServer::read_file_and_send(bool fin_flag, int start_byte,
                                   int end_byte) {
  int datalength = end_byte - start_byte;
  if (file_length_ - start_byte < datalength) {
    datalength = file_length_ - start_byte;
    fin_flag = true;
  }
  char *fileData = reinterpret_cast<char *>(calloc(datalength, sizeof(char)));
  if (!file_.is_open()) {
    LOG(ERROR) << "File open failed !!!";
    return;
  }
  file_.seekg(start_byte);
  file_.read(fileData, datalength);

  DataSegment *data_segment = new DataSegment();
  data_segment->seq_number_ = start_byte + initial_seq_number_;
  data_segment->ack_number_ = 0;
  data_segment->ack_flag_ = false;
  data_segment->fin_flag_ = fin_flag;
  data_segment->length_ = datalength;
  data_segment->data_ = fileData;

  send_data_segment(data_segment);
  LOG(INFO) << "Packet sent:seq number: " << data_segment->seq_number_;

  free(fileData);
  free(data_segment);
}

char *UdpServer::GetRequest(int client_sockfd) {
  char *buffer =
      reinterpret_cast<char *>(calloc(MAX_PACKET_SIZE, sizeof(char)));
  struct sockaddr_in client_address;
  socklen_t addr_size;
  memset(buffer, 0, MAX_PACKET_SIZE);
  addr_size = sizeof(client_address);
  recvfrom(client_sockfd, buffer, MAX_PACKET_SIZE, 0,
           (struct sockaddr *)&client_address, &addr_size);

  LOG(INFO) << "***Request received is: " << buffer;
  cli_address_ = client_address;
  return buffer;
}

void UdpServer::send_data_segment(DataSegment *data_segment) {
  char *datagramChars = data_segment->SerializeToCharArray();
  sendto(sockfd_, datagramChars, MAX_PACKET_SIZE, 0,
         (struct sockaddr *)&cli_address_, sizeof(cli_address_));
  free(datagramChars);
}
}  // namespace safe_udp
