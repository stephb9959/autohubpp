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
#include "include/io/SocketPort.h"
#include "include/Logger.h"
#include "include/utils/utils.hpp"

#include <chrono>
#include <thread>
#include <array>
#include <future>
#include <boost/asio/use_future.hpp>
#include <future>

namespace ace
{
namespace io
{

using boost::asio::ip::tcp;

void
SocketPort::async_read_some() {
    if (socket_port_.get() == NULL || !socket_port_->is_open()) return;
    incoming_buffer_.resize(0);
    std::vector<uint8_t>().swap(incoming_buffer_);
    incoming_buffer_.resize(512);

    if (recv_buffer_has_data_ && m_recv_handler) {
        m_recv_handler();
    }
    socket_port_->async_read_some(boost::asio::buffer(incoming_buffer_),
            std::bind(&type::on_async_receive_some, this, std::placeholders::_1,
            std::placeholders::_2));
}

void
SocketPort::on_async_receive_some(const boost::system::error_code& ec,
        size_t bytes_transferred) {
    if (!ec) {
        utils::Logger::Instance().Debug(FUNCTION_NAME);
        if (bytes_transferred > 0) {
            std::lock_guard<std::mutex>lk(recv_buffer_mutex_);
            auto it = incoming_buffer_.begin();
            for (; it < incoming_buffer_.begin() + bytes_transferred; it++)
                recv_buffer_.push_back(*it);
            recv_buffer_has_data_ = recv_buffer_.size() > 0;
        }
        async_read_some();
    } else {
        utils::Logger::Instance().Debug("%s\t  - ERROR: %s",
                FUNCTION_NAME_CSTR, ec.message().c_str());
        if (bytes_transferred > 0) {
            std::lock_guard<std::mutex>lk(recv_buffer_mutex_);
            auto it = incoming_buffer_.begin();
            for (; it < incoming_buffer_.begin() + bytes_transferred; it++)
                recv_buffer_.push_back(*it);
            recv_buffer_has_data_ = recv_buffer_.size() > 0;
        }
    }
}

bool
SocketPort::open(const std::string com_port_name, uint32_t port = 9761) {
    boost::system::error_code ec;
    tcp::endpoint ep(boost::asio::ip::address::from_string(com_port_name),
            port);

    if (socket_port_.get() == NULL)
        return false;
    socket_port_->connect(ep, ec);
    if (ec)
        return false;

    async_read_some();
    return true;
}
/**
 * 
 * 
 * 
 * 
 */
void
SocketPort::close() {
    if (socket_port_) {
        if (socket_port_->is_open()) {
            socket_port_->cancel(); // cancel any existing async_read(s)
            socket_port_->close();
        }
        socket_port_.reset();
    }
}

/**
 * Complete an asynchronous read with a time out.
 * 
 * This function causes all outstanding asynchronous read or 
 * write operations to finish immediately, and the handlers for 
 * canceled operations will be passed the 
 * boost::asio::error::operation_aborted error.
 * 
 * Call will not return unless timeout has exceeded, data has been
 * been transferred to the buffer or operation has been canceled
 * 
 * @param buffer The buffer to which data will be placed
 * @param msTimeout The amount of time(milliseconds) to wait before canceling
 * @return Returns the number of bytes transferred to the buffer
 */
std::size_t
SocketPort::recv_with_timeout(std::vector<uint8_t>& buffer,
        uint32_t msTimeout) {
    utils::Logger::Instance().Trace(FUNCTION_NAME);
    uint16_t rVal = 0;
    std::vector<uint8_t> data;
    data.resize(512);

    boost::system::error_code ec;
    socket_port_->cancel(ec);
    if (recv_buffer_has_data_) recv_buffer(buffer);

    utils::Logger::Instance().Debug("ERROR %s", ec.message().c_str());
    
    std::future<std::size_t> read_result = socket_port_->async_read_some(
            boost::asio::buffer(data), boost::asio::use_future);
    std::future_status status;
    do {
        status = read_result.wait_for(std::chrono::milliseconds(msTimeout));
        if (status == std::future_status::timeout) {
            utils::Logger::Instance().Debug("%s\n\t  - timeout waiting for data, "
                    "%d ms timeout expired.\n"
                    "\t  - canceling async_read_some.", FUNCTION_NAME_CSTR, msTimeout);
            
            socket_port_->cancel(ec);
            if (recv_buffer_has_data_) recv_buffer(buffer);
            utils::Logger::Instance().Debug("ERROR %s", ec.message().c_str());
            
            break;
        } else if (status == std::future_status::ready) {
            rVal = read_result.get();
            data.resize(rVal);
            for (const auto& it : data)
                buffer.push_back(it);
            break;
        } else if (status == std::future_status::deferred) {
            utils::Logger::Instance().Debug("%s\n\t - deferred waiting",
                    FUNCTION_NAME_CSTR);
        } else {
            utils::Logger::Instance().Debug("%s\n\t - unknown state while waiting"
                    "for future",
                    FUNCTION_NAME_CSTR);
        }
    } while (status != std::future_status::ready);

    return rVal;
}

/**
 * Move the contents of the internal buffer to the end of supplied buffer
 * @param buffer 
 * @return returns the number of bytes read from the buffer
 */
uint16_t
SocketPort::recv_buffer(std::vector<uint8_t>& buffer) {
    uint16_t rVal = 0;
    std::lock_guard<std::mutex>_(recv_buffer_mutex_);
    for (const auto& it : recv_buffer_)
        buffer.push_back(it);

    rVal = recv_buffer_.size();
    recv_buffer_.resize(0);
    std::vector<uint8_t>().swap(recv_buffer_);

    recv_buffer_has_data_ = false;

    return rVal;
}

uint16_t
SocketPort::send_buffer(std::vector<uint8_t>& buffer) {
    utils::Logger::Instance().Trace(FUNCTION_NAME);
    uint16_t sent = 0;
    uint16_t to_send = buffer.size();
    std::vector<uint8_t> temp;
    while (sent < to_send) {
        std::vector<uint8_t>().swap(temp);
        std::copy(buffer.begin() + sent, buffer.end(),
                std::back_inserter(temp));
        sent += socket_port_->write_some(boost::asio::buffer(
                temp, temp.size()));
    }
}
} // namespace io
} // namespace ace