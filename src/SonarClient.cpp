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

#include <oculus_driver/SonarClient.h>

#include <spdlog/spdlog.h>

#include <spdlog/spdlog.h>

namespace oculus {

using namespace std::placeholders;

SonarClient::SonarClient(const IoServicePtr& service,
                         const Duration& checkerPeriod) :
    ioService_(service),
    socket_(nullptr),
    remote_(),
    sonarId_(0),
    connectionState_(Initializing),
    checkerPeriod_(checkerPeriod),
    checkerTimer_(*service, checkerPeriod_),
    statusListener_(service),
    //statusCallbackId_(0),
    //data_(0)
    statusCallbackId_(0),
    message_(Message::Create())
{
    this->checkerTimer_.async_wait(
        std::bind(&SonarClient::checker_callback, this, std::placeholders::_1));
    this->reset_connection();
}

bool SonarClient::is_valid(const OculusMessageHeader& header)
{
    return header.oculusId == OCULUS_CHECK_ID && header.srcDeviceId == sonarId_;
}

bool SonarClient::connected() const
{
    return connectionState_ == Connected;
}

size_t SonarClient::send(const boost::asio::streambuf& buffer) const
{
    std::unique_lock<std::mutex> lock(socketMutex_); // auto lock

    if(!socket_ || !this->connected())
        return 0;
    return socket_->send(buffer.data());
}

/**
 * Connection Watchdog.
 *
 * This is an asynchronous loop independent from the connection status (it is
 * looping even if there is no connection). It checks the status of the
 * connection with the sonar and restarts it if necessary.
 */
void SonarClient::checker_callback(const boost::system::error_code& err)
{
    // Programming now the next check 
    this->checkerTimer_.expires_from_now(checkerPeriod_);
    this->checkerTimer_.async_wait(
        std::bind(&SonarClient::checker_callback, this, std::placeholders::_1));

    if(connectionState_ == Initializing || connectionState_ == Attempt) {
        // Nothing more to be done. Waiting.
        return;
    }
    
    auto lastStatusTime = statusListener_.time_since_last_status();
    if(lastStatusTime > 5) {
        // The status is retrieved through broadcasted UDP packets. No status
        // means no sonar on the network -> no chance to connect.
        // Still doing nothing because it might be a recoverable connection
        // loss.
        connectionState_ = Lost;
        logger.error("Connection lost for {}s", lastStatusTime);
        return;
    }

    if(this->time_since_last_message() > 10) {
        // Here last status was received less than 5 seconds ago but the last
        // message is more than 10s old. The connection is probably broken and
        // needs a reset.
        logger.error("Broken connection. Resetting.");
        this->reset_connection();
        return;
    }
}

void SonarClient::check_reception(const boost::system::error_code& err)
{
    // no real handling for now
    if(err) {
        std::ostringstream oss;
        oss << "reception error : " << err;
        logger.error(oss.str());
        //throw oss.str();
    }
}

void SonarClient::reset_connection()
{
    connectionState_ = Attempt;
    this->close_connection(); // closing previous connection
    statusCallbackId_ = statusListener_.add_callback(
        std::bind(&SonarClient::on_first_status, this, _1));
}

void SonarClient::close_connection()
{
    if(socket_) {
        std::unique_lock<std::mutex> lock(socketMutex_);
        logger.info("Closing connection");
        try {
            boost::system::error_code err;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
            if(err) {
                std::ostringstream oss;
                oss << err;
                logger.error("Error closing socket : '{}'", oss.str());
                return;
            }
            socket_->close();
        }
        catch(const std::exception& e) {
            logger.error("Error closing connection : '{}'", e.what());
        }
        socket_ = nullptr;
    }
}

void SonarClient::on_first_status(const OculusStatusMsg& msg)
{
    // got a status message. No need to keep listening.
    statusListener_.remove_callback(statusCallbackId_);
    
    // device id and ip fetched from status message
    sonarId_ = msg.hdr.srcDeviceId;
    remote_ = remote_from_status<EndPoint>(msg);
    
    logger.info("Got Oculus status : netip = {}, netmask = {}",
                 ip_to_string(msg.ipAddr), ip_to_string(msg.ipMask));

    // attempting connection
    socket_ = std::make_unique<Socket>(*ioService_);
    socket_->async_connect(remote_, std::bind(&SonarClient::connect_callback, this, _1));
}

void SonarClient::connect_callback(const boost::system::error_code& err)
{
    std::ostringstream oss;
    if(err) {
        oss << "oculus::SonarClient : connection failure. ( " << remote_ << ")";
        throw std::runtime_error(oss.str());
    }
    oss.clear();
    oss << remote_;
    logger.info("Connection successful ({})", oss.str());
    
    clock_.reset();

    connectionState_ = Connected;
    // this enters the ping data reception loop
    this->initiate_receive();

    this->on_connect();
}

void SonarClient::on_connect()
{
    // To be reimplemented in a subclass
}

void SonarClient::initiate_receive()
{
    if(!socket_) return;
    // asynchronously scan input until finding a valid header.
    // /!\ To be checked : This function and its callback handle the data
    // synchronization with the start of the ping message. It is relying on the
    // assumption that if the data left in the socket is less than the size of
    // the header, a short read will happen, so that the next read will be
    // exactly aligned on the next header.
    static unsigned int count = 0;
    //logger.info("Initiate receive : {}", count);
    count++;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(reinterpret_cast<uint8_t*>(&message_->header_), 
                            sizeof(message_->header_)),
        std::bind(&SonarClient::header_received_callback, this, _1, _2));
}

void SonarClient::header_received_callback(const boost::system::error_code err,
                                           std::size_t receivedByteCount)
{
    static unsigned int count = 0;
    //logger.info("Receive callback : {}", count);
    count++;
    // This function received only enough bytes for an OculusMessageHeader.  If
    // the header is valid, the control is dispatched to the next state
    // depending on the header content (message type). For now only simple ping
    // is implemented, but it seems to be the only message sent by the Oculus.
    // (TODO : check this last statement. Checked : wrong. Other message types
    // seem to be sent but are not documented by Oculus).
    this->check_reception(err);
    if(receivedByteCount != sizeof(message_->header_) || !this->is_valid(message_->header_)) {
        // Either we got data in the middle of a ping or did not get enougth
        // bytes (end of message). Continue listening to get a valid header.
        logger.error("Header reception error. Bytes received : {}, expected : {}",
                      receivedByteCount, sizeof(message_->header_));
        this->initiate_receive();
        return;
    }

    // Messsage header is valid. Now getting the remaining part of the message.
    // (The header contains the payload size, we can receive everything and
    // parse afterwards).
    message_->update_from_header();
    boost::asio::async_read(*socket_,
        boost::asio::buffer(message_->payload_handle(), message_->payload_size()),
        std::bind(&SonarClient::data_received_callback, this, _1, _2));
}

void SonarClient::data_received_callback(const boost::system::error_code err,
                                         std::size_t receivedByteCount)
{
    if(receivedByteCount != message_->header_.payloadSize) {
        // We did not get enough bytes. Reinitiating reception.
        logger.error("Data reception error. Bytes received : {}, expected : {}",
                      receivedByteCount, message_->header_.payloadSize);
        this->initiate_receive();
        return;
    }
    
    clock_.reset();
    // handle message is to be reimplemented in a subclass
    this->handle_message(message_);

    // Continuing the reception loop.
    this->initiate_receive();
}

void SonarClient::handle_message(const Message::ConstPtr& msg)
{
    // To be reimplemented in a subclass
}

} //namespace oculus
