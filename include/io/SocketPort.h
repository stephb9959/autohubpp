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

#ifndef SOCKETPORT_H
#define SOCKETPORT_H

#include "ioport.hpp"

#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

namespace ace {
    namespace io {
        typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_port_ptr;
        typedef std::function<void() > recv_handler;

        class SocketPort : public IOPort {
        public:
            typedef IOPort base;
            typedef SocketPort type;

            SocketPort(boost::asio::io_service& ios) : base(), socket_io_(ios),
            recv_buffer_has_data_(false) {
                socket_port_ = std::make_shared<boost::asio::ip::tcp::socket>
                        (socket_io_);
            }

            SocketPort() = delete;

            ~SocketPort() {
                close();
            }

            bool open(const std::string host, int) override;

            void async_read_some() override;

            void
            set_recv_handler(std::function<void() > fp) override {
                m_recv_handler = fp;
            }

            void close();

            std::size_t recv_with_timeout(std::vector<unsigned char>& buffer,
                    int msTimeout = 50) override;

            unsigned int recv_buffer(std::vector<unsigned char>& buffer) override;

            unsigned int send_buffer(std::vector<unsigned char>& buffer) override;
        protected:

            void on_async_receive_some(const boost::system::error_code& ec,
                    size_t bytes_transferred);

            void on_async_receive_more(const boost::system::error_code& ec,
                    size_t bytes_transferred);

        private:
            boost::asio::io_service& socket_io_;
            recv_handler m_recv_handler;
            socket_port_ptr socket_port_;
            std::vector<unsigned char> incoming_buffer_;
            std::vector<unsigned char> recv_buffer_;
            bool recv_buffer_has_data_;
            std::mutex recv_buffer_mutex_;
        };
    } // namespace io
} // namespace ace
#endif /* SOCKETPORT_H */

