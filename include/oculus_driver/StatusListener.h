/******************************************************************************
 * oculus_driver driver library for Blueprint Subsea Oculus sonar.
 * Copyright (C) 2020 ENSTA-Bretagne
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/

#pragma once

#include <eventpp/callbacklist.h>
#include <eventpp/utilities/counterremover.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "oculus_driver/Clock.h"
#include "oculus_driver/Oculus.h"

namespace oculus {

class StatusListener
{
    public:

    using IoService    = boost::asio::io_service;
    using IoServicePtr = std::shared_ptr<IoService>;
    using Socket       = boost::asio::ip::udp::socket;
    using EndPoint     = boost::asio::ip::udp::endpoint;

    private:
    OculusStatusMsg prev_;

    protected:
    const std::shared_ptr<spdlog::logger> logger;

    Socket          socket_;
    EndPoint        remote_;
    OculusStatusMsg msg_;
    Clock           clock_;

    eventpp::CallbackList<void(const OculusStatusMsg&)> callbacks_;

    public:

    StatusListener(const IoServicePtr& service,
                   const std::shared_ptr<spdlog::logger>& logger,
                   uint16_t listeningPort = 52102);
    
    inline auto& callbacks() { return callbacks_; }

    template <typename T = float>
    T time_since_last_status() const { return clock_.now<T>(); }

    inline OculusStatusMsg get_latest() { return prev_; }
    
    private:
    
    void get_one_message();
    void message_callback(const boost::system::error_code& err, std::size_t bytesReceived);
};

}  // namespace oculus
