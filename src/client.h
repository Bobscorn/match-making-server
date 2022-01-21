#pragma once

#include <string>
#include <chrono>
#include <memory>

#include "loop.h"
#include "socket_wrapper.h"
#include "protocol.h"

void client_loop(std::string server_address_pair, protocol input_protocol);

class client_init_kit {
    public:
    std::unique_ptr<IDataSocketWrapper> conn_socket;
    std::chrono::system_clock::time_point last_send;
    int auth_key;
    EXECUTION_STATUS status;
    ::protocol protocol;
    bool is_leader;

    client_init_kit(std::string server_address_pair, ::protocol input_protocol);
    ~client_init_kit();
};
