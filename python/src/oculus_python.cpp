#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <oculus_driver/Oculus.h>
#include <oculus_driver/OculusMessage.h>
#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>
#include <oculus_driver/Recorder.h>
#include <spdlog/spdlog.h>

#include "oculus_message.h"
#include "oculus_files.h"

inline void message_callback_wrapper(py::object callback, const oculus::Message::ConstPtr& msg)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    callback(py::cast(msg));

    PyGILState_Release(gstate);
}

inline void ping_callback_wrapper(py::object callback,
                                  const oculus::PingMessage::ConstPtr& msg)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    callback(py::cast(msg));

    PyGILState_Release(gstate);
}

inline void status_callback_wrapper(py::object callback,
                                    const oculus::OculusStatusMsg& status)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    callback(py::cast(&status));

    PyGILState_Release(gstate);
}

inline void config_callback_wrapper(py::object callback,
                                    const oculus::SonarDriver::PingConfig& lastConfig,
                                    const oculus::SonarDriver::PingConfig& newConfig)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    callback(py::cast(&lastConfig), py::cast(&newConfig));

    PyGILState_Release(gstate);
}

struct OculusPythonHandle
{
    oculus::AsyncService service_;
    oculus::SonarDriver  sonar_;
    oculus::Recorder     recorder_;
    oculus::SonarDriver::MessageCallbacksType::Handle recorderCallbackId_;

    OculusPythonHandle() :
        sonar_(service_.io_service(), spdlog::get("console"))
    {}
    ~OculusPythonHandle() { this->stop(); }

    void start() { service_.start(); }
    void stop()  { service_.stop();  }

    bool send_config(py::object obj) {
        return sonar_.send_ping_config(*obj.cast<const oculus::SonarDriver::PingConfig*>());
    }

    py::object current_config() const {
        return py::cast(sonar_.last_ping_config());
    }

    py::object request_config(py::object obj) {
        return py::cast(sonar_.request_ping_config(*obj.cast<const oculus::SonarDriver::PingConfig*>()));
    }

    void add_message_callback(py::object obj) {
        sonar_.message_callbacks().append(std::bind(message_callback_wrapper, obj,
                                              std::placeholders::_1));
    }
    void add_ping_callback(py::object obj) {
        sonar_.ping_callbacks().append(std::bind(ping_callback_wrapper, obj,
                                           std::placeholders::_1));
    }
    void add_status_callback(py::object obj) {
        sonar_.status_callbacks().append(std::bind(status_callback_wrapper, obj,
                                             std::placeholders::_1));
    }
    void add_config_callback(py::object obj) {
        sonar_.config_callbacks().append(std::bind(config_callback_wrapper, obj,
                                             std::placeholders::_1,
                                             std::placeholders::_2));
        // sonar_.config_callbacks().append([this, obj](const auto last_config, const auto new_config) {
        //     config_callback_wrapper(obj, last_config, new_config);
        // });
    }

    void recorder_start(const std::string& filename, bool overwrite) {
        if (recorder_.is_open()) {
            return;
        }
        recorder_.open(filename, overwrite);
        recorderCallbackId_ = sonar_.message_callbacks().append(
            std::bind(&OculusPythonHandle::recorder_callback, this,
                      std::placeholders::_1));
    }
    void recorder_stop() {
        recorder_.close();
        if (recorderCallbackId_) {
            sonar_.message_callbacks().remove(recorderCallbackId_);
        }
    }
    void recorder_callback(const oculus::Message::ConstPtr& msg) const {
        recorder_.write(msg);
    }
    bool is_recording() const {
        return recorder_.is_open();
    }
};

PYBIND11_MODULE(_oculus_python, m_)
{
    py::class_<oculus::OculusMessageHeader>(m_, "OculusMessageHeader")
        .def(py::init<>())
        .def_readwrite("oculusId",    &oculus::OculusMessageHeader::oculusId)
        .def_readwrite("srcDeviceId", &oculus::OculusMessageHeader::srcDeviceId)
        .def_readwrite("dstDeviceId", &oculus::OculusMessageHeader::dstDeviceId)
        .def_readwrite("msgId",       &oculus::OculusMessageHeader::msgId)
        .def_readwrite("msgVersion",  &oculus::OculusMessageHeader::msgVersion)
        .def_readwrite("payloadSize", &oculus::OculusMessageHeader::payloadSize)
        .def_readwrite("partNumber",      &oculus::OculusMessageHeader::partNumber)
        .def("__str__", [](const oculus::OculusMessageHeader& header) {
            std::ostringstream oss;
            oss << header;
            return oss.str();
        });

    py::class_<oculus::OculusSimpleFireMessage>(m_, "OculusSimpleFireMessage")
        .def(py::init<>())
        .def_readwrite("head",            &oculus::OculusSimpleFireMessage::head)
        .def_readwrite("masterMode",      &oculus::OculusSimpleFireMessage::masterMode)
        .def_readwrite("pingRate",        &oculus::OculusSimpleFireMessage::pingRate)
        .def_readwrite("networkSpeed",    &oculus::OculusSimpleFireMessage::networkSpeed)
        .def_readwrite("gammaCorrection", &oculus::OculusSimpleFireMessage::gammaCorrection)
        .def_readwrite("flags",           &oculus::OculusSimpleFireMessage::flags)
        .def_readwrite("range",           &oculus::OculusSimpleFireMessage::range)
        .def_readwrite("gain",     &oculus::OculusSimpleFireMessage::gain)
        .def_readwrite("speedOfSound",    &oculus::OculusSimpleFireMessage::speedOfSound)
        .def_readwrite("salinity",        &oculus::OculusSimpleFireMessage::salinity)
        .def("__str__", [](const oculus::OculusSimpleFireMessage& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<oculus::OculusSimplePingResult>(m_, "OculusSimplePingResult")
        .def(py::init<>())
        .def_readonly("fireMessage",       &oculus::OculusSimplePingResult::fireMessage)
        .def_readonly("pingId",            &oculus::OculusSimplePingResult::pingId)
        .def_readonly("status",            &oculus::OculusSimplePingResult::status)
        .def_readonly("frequency",         &oculus::OculusSimplePingResult::frequency)
        .def_readonly("temperature",       &oculus::OculusSimplePingResult::temperature)
        .def_readonly("pressure",          &oculus::OculusSimplePingResult::pressure)
        .def_readonly("speeedOfSoundUsed", &oculus::OculusSimplePingResult::speeedOfSoundUsed)
        .def_readonly("pingStartTime",     &oculus::OculusSimplePingResult::pingStartTime)
        .def_readonly("dataSize",          &oculus::OculusSimplePingResult::dataSize)
        .def_readonly("rangeResolution",   &oculus::OculusSimplePingResult::rangeResolution)
        .def_readonly("nRanges",           &oculus::OculusSimplePingResult::nRanges)
        .def_readonly("nBeams",            &oculus::OculusSimplePingResult::nBeams)
        .def_readonly("imageOffset",       &oculus::OculusSimplePingResult::imageOffset)
        .def_readonly("imageSize",         &oculus::OculusSimplePingResult::imageSize)
        .def_readonly("messageSize",       &oculus::OculusSimplePingResult::messageSize)
        .def("__str__", [](const oculus::OculusSimplePingResult& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<oculus::OculusSimpleFireMessage2>(m_, "OculusSimpleFireMessage2")
        .def(py::init<>())
        .def_readwrite("head",            &oculus::OculusSimpleFireMessage2::head)
        .def_readwrite("masterMode",      &oculus::OculusSimpleFireMessage2::masterMode)
        .def_readwrite("pingRate",        &oculus::OculusSimpleFireMessage2::pingRate)
        .def_readwrite("networkSpeed",    &oculus::OculusSimpleFireMessage2::networkSpeed)
        .def_readwrite("gammaCorrection", &oculus::OculusSimpleFireMessage2::gammaCorrection)
        .def_readwrite("flags",           &oculus::OculusSimpleFireMessage2::flags)
        .def_readwrite("range",           &oculus::OculusSimpleFireMessage2::range)
        .def_readwrite("gain",     &oculus::OculusSimpleFireMessage2::gain)
        .def_readwrite("speedOfSound",    &oculus::OculusSimpleFireMessage2::speedOfSound)
        .def_readwrite("salinity",        &oculus::OculusSimpleFireMessage2::salinity)
        .def_readwrite("extFlags",        &oculus::OculusSimpleFireMessage2::extFlags)
        .def("__str__", [](const oculus::OculusSimpleFireMessage2& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<oculus::OculusSimplePingResult2>(m_, "OculusSimplePingResult2")
        .def(py::init<>())
        .def_readonly("fireMessage",       &oculus::OculusSimplePingResult2::fireMessage)
        .def_readonly("pingId",            &oculus::OculusSimplePingResult2::pingId)
        .def_readonly("status",            &oculus::OculusSimplePingResult2::status)
        .def_readonly("frequency",         &oculus::OculusSimplePingResult2::frequency)
        .def_readonly("temperature",       &oculus::OculusSimplePingResult2::temperature)
        .def_readonly("pressure",          &oculus::OculusSimplePingResult2::pressure)
        .def_readonly("heading",           &oculus::OculusSimplePingResult2::heading)
        .def_readonly("pitch",             &oculus::OculusSimplePingResult2::pitch)
        .def_readonly("roll",              &oculus::OculusSimplePingResult2::roll)
        .def_readonly("speeedOfSoundUsed", &oculus::OculusSimplePingResult2::speeedOfSoundUsed)
        .def_readonly("pingStartTime",     &oculus::OculusSimplePingResult2::pingStartTime)
        .def_readonly("dataSize",          &oculus::OculusSimplePingResult2::dataSize)
        .def_readonly("rangeResolution",   &oculus::OculusSimplePingResult2::rangeResolution)
        .def_readonly("nRanges",           &oculus::OculusSimplePingResult2::nRanges)
        .def_readonly("nBeams",            &oculus::OculusSimplePingResult2::nBeams)
        .def_readonly("spare0",            &oculus::OculusSimplePingResult2::spare0)
        .def_readonly("spare1",            &oculus::OculusSimplePingResult2::spare1)
        .def_readonly("spare2",            &oculus::OculusSimplePingResult2::spare2)
        .def_readonly("spare3",            &oculus::OculusSimplePingResult2::spare3)
        .def_readonly("imageOffset",       &oculus::OculusSimplePingResult2::imageOffset)
        .def_readonly("imageSize",         &oculus::OculusSimplePingResult2::imageSize)
        .def_readonly("messageSize",       &oculus::OculusSimplePingResult2::messageSize)
        .def("__str__", [](const oculus::OculusSimplePingResult2& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });


    py::class_<oculus::OculusVersionInfo>(m_, "OculusVersionInfo")
        .def(py::init<>())
        .def_readonly("arm0Version0", &oculus::OculusVersionInfo::arm0Version0)
        .def_readonly("arm0Date0",    &oculus::OculusVersionInfo::arm0Date0)
        .def_readonly("arm1Version1", &oculus::OculusVersionInfo::arm1Version1)
        .def_readonly("arm1Date1",    &oculus::OculusVersionInfo::arm1Date1)
        .def_readonly("coreVersion2", &oculus::OculusVersionInfo::coreVersion2)
        .def_readonly("coreDate2",    &oculus::OculusVersionInfo::coreDate2);
        // .def("__str__", [](const oculus::OculusVersionInfo& version) {
        //    std::ostringstream oss;
        //    oss << version;
        //    return oss.str();
        // });

    py::class_<oculus::OculusStatusMsg>(m_, "OculusStatusMsg")
        .def(py::init<>())
        .def_readonly("head",             &oculus::OculusStatusMsg::head)
        .def_readonly("deviceId",        &oculus::OculusStatusMsg::deviceId)
        .def_readonly("deviceType",      &oculus::OculusStatusMsg::deviceType)
        .def_readonly("partNumber",      &oculus::OculusStatusMsg::partNumber)
        .def_readonly("status",          &oculus::OculusStatusMsg::status)
        .def_readonly("versionInfo",     &oculus::OculusStatusMsg::versionInfo)
        .def_readonly("ipAddr",          &oculus::OculusStatusMsg::ipAddr)
        .def_readonly("ipMask",          &oculus::OculusStatusMsg::ipMask)
        .def_readonly("connectedIpAddr", &oculus::OculusStatusMsg::clientAddr)
        .def_readonly("macAddr0",        &oculus::OculusStatusMsg::macAddr0)
        .def_readonly("macAddr1",        &oculus::OculusStatusMsg::macAddr1)
        .def_readonly("macAddr2",        &oculus::OculusStatusMsg::macAddr2)
        .def_readonly("macAddr3",        &oculus::OculusStatusMsg::macAddr3)
        .def_readonly("macAddr4",        &oculus::OculusStatusMsg::macAddr4)
        .def_readonly("macAddr5",        &oculus::OculusStatusMsg::macAddr5)
        .def_readonly("temperature0",    &oculus::OculusStatusMsg::temperature0)
        .def_readonly("temperature1",    &oculus::OculusStatusMsg::temperature1)
        .def_readonly("temperature2",    &oculus::OculusStatusMsg::temperature2)
        .def_readonly("temperature3",    &oculus::OculusStatusMsg::temperature3)
        .def_readonly("temperature4",    &oculus::OculusStatusMsg::temperature4)
        .def_readonly("temperature5",    &oculus::OculusStatusMsg::temperature5)
        .def_readonly("temperature6",    &oculus::OculusStatusMsg::temperature6)
        .def_readonly("temperature7",    &oculus::OculusStatusMsg::temperature7)
        .def_readonly("pressure",        &oculus::OculusStatusMsg::pressure)
        .def("__str__", [](const oculus::OculusStatusMsg& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<OculusPythonHandle>(m_, "OculusSonar")
        .def(py::init<>())
        .def("start",       &OculusPythonHandle::start)
        .def("stop",        &OculusPythonHandle::stop)

        .def("send_config",    &OculusPythonHandle::send_config)
        .def("request_config", &OculusPythonHandle::request_config)
        .def("current_config", &OculusPythonHandle::current_config)

        .def("add_message_callback", &OculusPythonHandle::add_message_callback)
        .def("add_ping_callback",    &OculusPythonHandle::add_ping_callback)
        .def("add_status_callback",  &OculusPythonHandle::add_status_callback)
        .def("add_config_callback",  &OculusPythonHandle::add_config_callback)

        .def("recorder_start", &OculusPythonHandle::recorder_start)
        .def("recorder_stop",  &OculusPythonHandle::recorder_stop)
        .def("is_recording",   &OculusPythonHandle::is_recording);

    init_oculus_message(m_);
    init_oculus_python_files(m_);
}
