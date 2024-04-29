#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <glog/logging.h>

#include "udp_server.h"

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;
  FLAGS_minloglevel = google::GLOG_INFO;

  int sfd = 0;
  int port_num = 8080;
  int recv_window = 0;
  char *message_recv;
  if (argc < 3) {
    LOG(INFO) << "Please provide a port number and receive window";
    exit(1);
  }
  if (argv[1] != NULL) {
    port_num = atoi(argv[1]);
  }
  if (argv[2] != NULL) {
    recv_window = atoi(argv[2]);
  }

  UdpServer *udp_server = new UdpServer();
  udp_server->rwnd_ = recv_window;
  sfd = udp_server->StartServer(port_num);
  message_recv = udp_server->GetRequest(sfd);
  // char cwd[1024];
  // if (getcwd(cwd, sizeof(cwd)) != NULL) {
  //   LOG(INFO) << "Current working directory: " << cwd;
  // } else {
  //   perror("getcwd() error");
  //   return 1;
  // }

  std::string file_name =
      "/work/files/server_files/" + std::string(message_recv);
  if (udp_server->OpenFile(file_name)) {
    udp_server->StartFileTransfer();
  } else {
    udp_server->SendError();
  }

  free(udp_server);
  return 1;
}
