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

#include "oculus_driver/SonarDriver.h"

namespace oculus
{

SonarDriver::SonarDriver(const IoServicePtr &service,
                         const std::shared_ptr<spdlog::logger> &logger,
                         const Duration &checkerPeriod)
    : SonarClient(service, logger, checkerPeriod),
      lastConfig_(default_ping_config()),
      lastPingRate_(PingRateNormal) {}

bool SonarDriver::send_ping_config(PingConfig config)
{
    config.head.oculusId = OCULUS_CHECK_ID;
    config.head.msgId = MsgSimpleFire;
    config.head.srcDeviceId = 0;
    config.head.dstDeviceId = sonarId_;
    config.head.payloadSize = sizeof(PingConfig) - sizeof(OculusMessageHeader);
    config.head.msgVersion = 2;  // Requesting SimpleFireResponse V2

    // Other non runtime-configurable parameters (TODO : make then launch parameters)
    config.networkSpeed = 0xff;

    boost::asio::streambuf buf;
    buf.sputn(reinterpret_cast<const char*>(&config), sizeof(config));

    auto bytesSent = this->send(buf);
    if(bytesSent != sizeof(config))
    {
        logger->error("Could not send whole fire message({}/{})", bytesSent, sizeof(config));
        return false;
    }

    // BUG IN THE SONAR FIRMWARE : the sonar never sets the
    // config.pingRate field in the SimplePing message -> there is no
    // feedback saying if this parameter is effectively set by the sonar. The
    // line below allows to keep a trace of the requested ping rate but there
    // is no clean way to check.
    lastConfig_.pingRate = config.pingRate;

    // Also saving the last pingRate which is not standby to be able to resume
    // the sonar to the last ping rate in the resume() method.
    if(lastConfig_.pingRate != PingRateStandby) {
      lastPingRate_ = lastConfig_.pingRate;
    }
    return true;
}

SonarDriver::PingConfig SonarDriver::last_ping_config() const
{
    return lastConfig_;
}

SonarDriver::PingConfig SonarDriver::current_ping_config()
{
    PingConfig config;

    auto configSetter = [&](const Message::ConstPtr &message) {
        // lastConfig_ is ALWAYS updated before the callbacks are called.
        // We only need to wait for the next message to get the current ping
        // configuration.
        config = lastConfig_;
        config.head = message->header();
    };

    if(!timedCallback(messageCallbacks_, configSetter))
    {
        throw TimeoutReached();
    }
    return config;
}

SonarDriver::PingConfig SonarDriver::request_ping_config(PingConfig request)
{
    request.flags |= 0x4;  // forcing sonar sending gains to true

    // Waiting for a ping or a dummy message to have a feedback on the config changes.
    PingConfig feedback;
    int count = 0;
    const int maxCount = 100;  // TODO(jp-pino) make a parameter out of this
    do
    {
        if(this->send_ping_config(request))
        {
            try
            {
                feedback = this->current_ping_config();
                if(check_config_feedback(request, feedback)) break;
            }
            catch(const TimeoutReached& e)
            {
                logger->error("Timeout reached while requesting config");
                continue;
            }
        }
        count++;
    } while(count < maxCount);

    if(count >= maxCount)
    {
        logger->error(
            "Could not get a proper feedback from the sonar. "
            "Assuming the configuration is ok (fix this)");
        feedback = request;
        feedback.head.msgId = 0;  // invalid, will be checkable.
    }

    return feedback;
}

void SonarDriver::standby()
{
    auto request = lastConfig_;

    request.pingRate = PingRateStandby;

    this->send_ping_config(request);
}

void SonarDriver::resume()
{
    auto request = lastConfig_;

    request.pingRate = lastPingRate_;

    this->send_ping_config(request);
}

/**
 * Called when the driver first connection with the sonar.
 *
 * Will be called again if a new connection append.
 */
void SonarDriver::on_connect()
{
    // This makes the oculus fire right away.
    // On first connection lastConfig_ is equal to default_ping_config().
    status_callbacks()(statusListener_.get_latest());
}

/**
 * Called when a new complete message is received (any type).
 */
void SonarDriver::handle_message(const Message::ConstPtr& message)
{
    const auto& header = message->header();
    const auto& data = message->data();
    auto newConfig = lastConfig_;
    switch(header.msgId)
    {
    case MsgSimplePingResult:
        newConfig = reinterpret_cast<const PingResult*>(data.data())->fireMessage;
        newConfig.pingRate = lastConfig_.pingRate;  // feedback is broken on pingRate
        // When masterMode = 2, the sonar force gain between 40& and
        // 100%, BUT still needs resquested gain to be between 0%
        // and 100%. (If you request a gain=0 in masterMode=2, the
        // fireMessage in the ping results will be 40%). The gain is
        // rescaled here to ensure consistent parameter handling on client
        // side).
        if(newConfig.masterMode == 2) { newConfig.gain = (newConfig.gain - 40.0) * 100.0 / 60.0; }
        break;
    case MsgDummy:
        logger->trace("Dummy message received. Changing ping rate to standby");
        newConfig.pingRate = PingRateStandby;
        break;
    default:
        break;
    };

    if(config_changed(lastConfig_, newConfig)) {
        configCallbacks_(lastConfig_, newConfig);
    }
    lastConfig_ = newConfig;

    // Calling generic message callbacks first (in case we want to do something
    // before calling the specialized callbacks).
    messageCallbacks_(message);
    switch(header.msgId)
    {
    case MsgSimplePingResult:
        pingCallbacks_(PingMessage::Create(message));
        break;
    case MsgDummy:
        dummyCallbacks_(header);
        break;
    case MsgSimpleFire:
        logger->error("messageSimpleFire parsing not implemented.");
        break;
    case MsgPingResult:
        logger->error("messagePingResult parsing not implemented.");
        break;
    case MsgUserConfig:
        logger->error("messageUserConfig parsing not implemented.");
        break;
    default:
        break;
    }
}

}  // namespace oculus
