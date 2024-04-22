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

#include <memory>

#include "oculus_driver/Oculus.h"
#include "oculus_driver/SonarClient.h"
#include "oculus_driver/print_utils.h"
#include "oculus_driver/utils.h"

namespace oculus {

class SonarDriver : public SonarClient {
 public:
  using Duration = SonarClient::Duration;

  using IoService = boost::asio::io_service;
  using IoServicePtr = std::shared_ptr<IoService>;

  using PingConfig = OculusSimpleFireMessage;
  using PingResult = OculusSimplePingResult;

  using MessageCallback = std::function<void(const Message::ConstPtr&)>;
  using StatusCallback = std::function<void(const OculusStatusMsg&)>;
  using PingCallback = std::function<void(const PingMessage::ConstPtr)>;
  using DummyCallback = std::function<void(const OculusMessageHeader&)>;
  using ConfigCallback =
      std::function<void(const PingConfig&, const PingConfig&)>;

  using TimeSource = SonarClient::TimeSource;
  using TimePoint =
      typename std::invoke_result<decltype(&TimeSource::now)>::type;

 protected:
  PingConfig lastConfig_;
  uint8_t lastPingRate_;

  // message callbacks will be called on every received message.
  // config callbacks will be called on (detectable) configuration changes.
  eventpp::CallbackList<void(const Message::ConstPtr&)> messageCallbacks;
  eventpp::CallbackList<void(const PingMessage::ConstPtr)> pingCallbacks;
  eventpp::CallbackList<void(const OculusMessageHeader&)> dummyCallbacks;
  eventpp::CallbackList<void(const PingConfig&, const PingConfig&)>
      configCallbacks;

 public:
  SonarDriver(const IoServicePtr& service,
              const std::shared_ptr<spdlog::logger>& logger,
              const Duration& checkerPeriod = boost::posix_time::seconds(1));

  bool send_ping_config(PingConfig config);
  PingConfig current_ping_config();
  PingConfig request_ping_config(PingConfig request);
  PingConfig last_ping_config() const;

  // Stanby mode (saves current ping rate and set it to 0 on the sonar
  void standby();
  void resume();

  virtual void on_connect();
  virtual void handle_message(const Message::ConstPtr& message);

  /////////////////////////////////////////////
  // All remaining member function are related to callbacks and are merely
  // helpers to add callbacks.

  auto& message_callbacks() { return messageCallbacks; }
  auto& ping_callbacks() { return pingCallbacks; }
  auto& dummy_callbacks() { return dummyCallbacks; }
  auto& config_callbacks() { return configCallbacks; }
};

}  // namespace oculus
