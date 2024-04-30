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

#include <atomic>
#include <boost/asio.hpp>
#include <iostream>

#include "oculus_driver/Oculus.h"
#include "print_utils.h"

namespace oculus
{
    
struct TimeoutReached : public std::exception {
  const char* what() const throw() {
    return "Timeout reached before callback call.";
  }
};

template <typename EndPointT> inline EndPointT remote_from_status(const OculusStatusMsg& status)
{
    // going through string conversion allows to not care about
    // endianess. (fix this)
    return EndPointT(boost::asio::ip::address_v4::from_string(ip_to_string(status.ipAddr)), 52100);
}

inline bool header_valid(const OculusMessageHeader& header)
{
    return header.oculusId == 0x4f53;  // Fixed for Oculus Sonar
}

inline bool is_ping_message(const OculusMessageHeader& header)
{
    return header_valid(header) && header.msgId == OculusMessageType::MsgSimplePingResult;
}

inline OculusSimpleFireMessage2 default_ping_config()
{
    OculusSimpleFireMessage2 msg;
    std::memset(&msg, 0, sizeof(msg));

    msg.head.oculusId = OCULUS_CHECK_ID;
    msg.head.msgId = OculusMessageType::MsgSimpleFire;
    msg.head.srcDeviceId = 0;
    msg.head.dstDeviceId = 0;
    msg.head.payloadSize = sizeof(OculusSimpleFireMessage2) - sizeof(OculusMessageHeader);

    msg.masterMode = 2;
    msg.networkSpeed = 0xff;
    msg.gammaCorrection = 127;
    msg.pingRate = PingRateType::PingRateNormal;
    msg.range = 2.54;
    msg.gain = 50;
    msg.flags = 0b00011101;
    // bit 0: [RangeInMetres]  1: Metres.
    // bit 1: [16BitImg]       0: 8-bit.
    // bit 2: [GainSend]       1: Return the gain at the start of each line.
    // bit 3: [SimpleReturn]   1: Ouput simple fire returns.
    // bit 4: [GainAssist]     1: Gain assist disabled.
    // bit 5: [LowPower]       0: Disabled.
    // bit 6: [FullBeams]      0: Use 256 beams.
    // bit 7: [NetworkTrigger] 0: Fires automatically according to PingRate.
    msg.speedOfSound = 0.0;
    msg.salinity = 0.0;

    return msg;
}

inline bool check_config_feedback(const OculusSimpleFireMessage2& requested, const OculusSimpleFireMessage2& feedback)
{
    // returns true if feedback coherent with requested config.
    if (requested.pingRate == PingRateType::PingRateStandby)
    {
        // If in standby, expecting a dummy message
        if (feedback.head.msgId == OculusMessageType::MsgDummy) return true;
    }
    else
    {
        // If got a simple ping result, checking relevant parameters
        if (feedback.head.msgId == OculusMessageType::MsgSimplePingResult &&
           requested.masterMode == feedback.masterMode
           // feedback is broken on pingRate field
           // && requested.pingRate         == feedback.pingRate
           && requested.gammaCorrection == feedback.gammaCorrection && requested.flags == feedback.flags &&
           requested.range == feedback.range && std::abs(requested.gain - feedback.gain) < 1.0e-1)
        {
            // return true;  // bypassing checks on sound speed
            //  changing soundspeed is very slow (up to 6 seconds, maybe more)

            // For now simple ping is ok. Checking sound speed / salinity
            // parameters If speed of sound is 0.0, the sonar is using salinity
            // to calculate speed of sound.
            if (requested.speedOfSound != 0.0)
            {
                if (std::abs(requested.speedOfSound - feedback.speedOfSound) < 1.0e-1) return true;
            }
            else
            {
                if (std::abs(requested.salinity - feedback.salinity) < 1.0e-1) return true;
            }
        }
    }
    return false;
}

inline bool config_changed(const OculusSimpleFireMessage2& previous, const OculusSimpleFireMessage2& next)
{

    if (previous.masterMode != next.masterMode) return true;
    if (previous.pingRate != next.pingRate) return true;
    if (previous.networkSpeed != next.networkSpeed) return true;
    if (previous.gammaCorrection != next.gammaCorrection) return true;
    if (previous.flags != next.flags) return true;

    if (abs(previous.range - next.range) > 0.001) return true;
    if (abs(previous.gain - next.gain) > 0.1) return true;
    if (abs(previous.speedOfSound - next.speedOfSound) > 0.1) return true;
    if (abs(previous.salinity - next.salinity) > 0.1) return true;

    return false;
}

template <typename Prototype, typename Policies>
bool timedCallback(eventpp::CallbackList<Prototype, Policies>& callbacks,
                   auto& callback, int timeout_ms = 5000) {
    std::atomic_flag called;
    auto start = std::chrono::steady_clock::now();
    auto handle = eventpp::counterRemover(callbacks).append(
        [start, timeout_ms, &callback, &called](auto... args) {
        if (std::chrono::steady_clock::now() - start <
            std::chrono::milliseconds(timeout_ms)) {
            callback(args...);
            called.test_and_set();
        }
    });

    while (!called.test() || std::chrono::steady_clock::now() - start <
                                std::chrono::milliseconds(timeout_ms)) {
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
    }
    callbacks.remove(handle);
    return called.test();
}

}  // namespace oculus
