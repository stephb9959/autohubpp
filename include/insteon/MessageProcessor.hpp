/*
 * Copyright (c) 2012, Aaron Coombs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Autohub++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL AARON COOMBS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MESSENGER_HPP
#define MESSENGER_HPP

#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <list>
#include <cstdint>


#include <boost/asio.hpp>

#include <yaml-cpp/yaml.h>

#include "../config.hpp"
#include "EchoStatus.hpp"
#include "InsteonProtocol.hpp"
#include "../io/ioport.hpp"
#include "../io/SerialPort.h"
#include "../io/SocketPort.h"
#include "../system/AutoResetEvent.hpp"

namespace ace
{
namespace insteon
{
class InsteonMessage;

struct WaitItem {

    WaitItem(uint8_t message_id) :
    message_id_(message_id), message_received_(false) {
    }
    uint8_t message_id_;
    bool message_received_;
    system::AutoResetEvent wait_event_;
    std::shared_ptr<InsteonMessage> insteon_message_;
};

typedef std::shared_ptr<InsteonMessage> msg_ptr;
typedef std::function<void(msg_ptr) > msg_handler;

/*
 * MessageProcessor class is responsible for coordinating
 * Insteon Messages between the IO and InsteonDevice objects
 */
class MessageProcessor : private boost::noncopyable {
public:
    typedef MessageProcessor type;

    explicit MessageProcessor(boost::asio::io_service& io_service,
                              YAML::Node config);
    ~MessageProcessor();

    bool connect();
    void onReceive();
    PlmEcho trySend(const std::vector<uint8_t>& send_buffer,
                       bool retry_on_nak = true);
    PlmEcho trySend(const std::vector<uint8_t>& send_buffer,
                       bool retry_on_nak, uint32_t echo_length);
    PlmEcho trySendReceive(const std::vector<uint8_t>&
                              send_buffer, int8_t triesLeft, uint8_t receive_message_id,
                              PropertyKeys& properties);

    void set_message_handler(msg_handler handler);
protected:
private:
    void processData();
    std::string byteArrayToStringStream(const std::vector<uint8_t>&
                                        data,
                                        uint32_t offset, uint32_t count);

    PlmEcho processEcho(uint32_t echo_length);
    bool processMessage(const std::vector<uint8_t>& read_buffer,
                        uint32_t offset, uint32_t& count, bool is_echo = false);

    void readData(std::vector<uint8_t>& return_buffer,
                  uint32_t bytes_expected,
                  bool is_echo);

    PlmEcho send(std::vector<uint8_t> send_buffer,
                    bool retry_on_nak, uint32_t echo_length);
    void updateWaitItems(const std::shared_ptr<InsteonMessage>& iMsg);

    std::unique_ptr<io::IOPort> io_port_;
    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand io_strand_;
    msg_handler msg_handler_;
    InsteonProtocol insteon_protocol_;

    std::list<std::shared_ptr<WaitItem>> wait_list_;
    std::mutex mutex_wait_list_;
    std::mutex lock_buffer_;
    std::vector<uint8_t> buffer_;
    std::mutex lock_data_processor_;

    std::chrono::system_clock::time_point time_of_last_command_;

    YAML::Node config_;
};
} // namespace insteon
} // namespace ace
#endif /* MESSENGER_HPP */

