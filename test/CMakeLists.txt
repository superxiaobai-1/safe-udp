
add_executable(server server.cpp)
target_include_directories(server PUBLIC
  ../udp_transport
)

target_link_libraries(server udp_transport)


add_executable(client client.cpp)
target_include_directories(client PUBLIC
  ../udp_transport
)

target_link_libraries(client udp_transport)

install(TARGETS  server  client DESTINATION  ${PROJECT_BINARY_DIR}/bin)




