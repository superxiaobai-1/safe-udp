set(file
  data_segment.cpp
  packet_statistics.cpp
  sliding_window.cpp
  udp_server.cpp
  udp_client.cpp
  )

add_library(udp_transport SHARED ${file})
target_link_libraries(udp_transport  glog)

install(TARGETS  udp_transport DESTINATION  ${PROJECT_BINARY_DIR}/lib)




