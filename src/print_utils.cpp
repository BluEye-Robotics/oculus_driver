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

#include "oculus_driver/print_utils.h"

namespace oculus
{

std::string ip_to_string(uint32_t ip)
{
    std::ostringstream oss;
    oss << (ip & 0x000000ff) << "." << ((ip & 0x0000ff00) >> 8) << "." << ((ip & 0x00ff0000) >> 16) << "."
        << ((ip & 0xff000000) >> 24);
    return oss.str();
}

std::string mac_to_string(const uint8_t* mac)
{
    std::ostringstream oss;
    oss << std::hex << (unsigned int)mac[0] << ":" << (unsigned int)mac[1] << ":" << (unsigned int)mac[2] << ":"
        << (unsigned int)mac[3] << ":" << (unsigned int)mac[4] << ":" << (unsigned int)mac[5];
    return oss.str();
}

std::string to_string(DataSizeType dataType)
{
    switch(dataType)
    {
    case ImageData8Bit:
        return "8bit";
    case ImageData16Bit:
        return "16bit";
    case ImageData24Bit:
        return "24bit";
    case ImageData32Bit:
        return "32bit";
    default:
        return "invalid";
    }
}

std::string to_string(PingRateType pingRate)
{
    switch(pingRate)
    {
    case PingRateNormal:
        return "normal (10Hz)";
    case PingRateHigh:
        return "high (15Hz)";
    case PingRateHighest:
        return "highest (40Hz)";
    case PingRateLow:
        return "low (5Hz)";
    case PingRateLowest:
        return "lowest (2Hz)";
    case PingRateStandby:
        return "Disable ping";
    default:
        return "invalid (" + std::to_string(pingRate) + ")";
    }
}

std::string to_string(OculusPartNumberType partNumber)
{
    switch(partNumber)
    {
    case M370s:
        return "M370s";
    case MT370s:
        return "MT370s";
    case MD370s:
        return "MD370s";
    case MF370s:
        return "MF370s";
    case MA370s:
        return "MA370s";
    case M750d:
        return "M750d";
    case MT750d:
        return "MT750d";
    case MD750d:
        return "MD750d";
    case MF750d:
        return "MF750d";
    case MA750d:
        return "MA750d";
    case M1200d:
        return "M1200d";
    case MT1200d:
        return "MT1200d";
    case MD1200d:
        return "MD1200d";
    case MF1200d:
        return "MF1200d";
    case MA1200d:
        return "MA1200d";
    case M3000d:
        return "M3000d";
    case MT3000d:
        return "MT3000d";
    case MF3000d:
        return "MF3000d";
    case MA3000d:
        return "MA3000d";
    default:
        return "unknown";
    }
}

std::string to_string(const OculusMessageHeader& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "oculusId    : " << msg.oculusId << prefix << "srcDeviceId : " << msg.srcDeviceId << prefix
        << "dstDeviceId : " << msg.dstDeviceId << prefix << "msgId       : " << msg.msgId << prefix
        << "msgVersion  : " << msg.msgVersion << prefix << "payloadSize : " << msg.payloadSize << prefix
        << "Part #      : " << msg.partNumber;
    return oss.str();
}

std::string to_string(const OculusStatusMsg& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "Device Id   : " << msg.deviceId << prefix << "Status      : " << msg.status << prefix
        << "Part #      : " << msg.partNumber << prefix << "IP          : " << ip_to_string(msg.ipAddr) << prefix
        << "Mask        : " << ip_to_string(msg.ipMask) << prefix << "Client IP   : " << ip_to_string(msg.clientAddr) << prefix
        << "MAC         : " << mac_to_string(&msg.macAddr0) << prefix << "Temperature0: " << msg.temperature0 << prefix
        << "Temperature1: " << msg.temperature1 << prefix << "Temperature2: " << msg.temperature2 << prefix
        << "Temperature3: " << msg.temperature3 << prefix << "Temperature4: " << msg.temperature4 << prefix
        << "Temperature5: " << msg.temperature5 << prefix << "Temperature6: " << msg.temperature6 << prefix
        << "Temperature7: " << msg.temperature7 << prefix << "Pressure    : " << msg.pressure;
    return oss.str();
}

std::string to_string(const OculusSimpleFireMessage& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "masterMode      : " << (int)msg.masterMode << prefix << "pingRate        : " << (int)msg.pingRate << prefix
        << "networkSpeed    : " << (int)msg.networkSpeed << prefix << "gammaCorrection : " << (int)msg.gammaCorrection << prefix
        << "flags           : " << std::hex << (int)msg.flags << prefix << "range           : " << msg.range << prefix
        << "gainPercent     : " << msg.gainPercent << prefix << "speedOfSound    : " << msg.speedOfSound << prefix
        << "salinity        : " << msg.salinity;
    return oss.str();
}

std::string to_string(const OculusSimplePingResult& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "pingId            : " << msg.pingId << prefix << "status            : " << msg.status << prefix
        << "frequency         : " << msg.frequency << prefix << "temperature       : " << msg.temperature << prefix
        << "pressure          : " << msg.pressure << prefix << "speeedOfSoundUsed : " << msg.speeedOfSoundUsed << prefix
        << "pingStartTime     : " << msg.pingStartTime << prefix << "dataSize          : " << msg.dataSize << prefix
        << "rangeResolution   : " << msg.rangeResolution << prefix << "nRanges           : " << msg.nRanges << prefix
        << "nBeams            : " << msg.nBeams << prefix << "imageOffset       : " << msg.imageOffset << prefix
        << "imageSize         : " << msg.imageSize << prefix << "messageSize       : " << msg.messageSize;
    return oss.str();
}

std::string to_string(const OculusSimpleFireMessage2& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "masterMode      : " << (int)msg.masterMode << prefix << "pingRate        : " << (int)msg.pingRate << prefix
        << "networkSpeed    : " << (int)msg.networkSpeed << prefix << "gammaCorrection : " << (int)msg.gammaCorrection << prefix
        << "flags           : " << std::hex << (int)msg.flags << prefix << "rangePercent    : " << msg.rangePercent << prefix
        << "gainPercent     : " << msg.gainPercent << prefix << "speedOfSound    : " << msg.speedOfSound << prefix
        << "salinity        : " << msg.salinity << prefix << "extFlags        : " << std::hex << msg.extFlags << prefix
        << "reserved        :";
    for(unsigned int i = 0; i < 8; i++)
    {
        oss << " " << msg.reserved[i];
    }
    return oss.str();
}

std::string to_string(const OculusSimplePingResult2& msg, const std::string& prefix)
{
    std::ostringstream oss;
    oss << prefix << "pingId            : " << msg.pingId << prefix << "status            : " << msg.status << prefix
        << "frequency         : " << msg.frequency << prefix << "temperature       : " << msg.temperature << prefix
        << "pressure          : " << msg.pressure << prefix << "heading           : " << msg.heading << prefix
        << "pitch             : " << msg.pitch << prefix << "roll              : " << msg.roll << prefix
        << "speedOfSoundUsed  : " << msg.speeedOfSoundUsed << prefix << "pingStartTime     : " << msg.pingStartTime << prefix
        << "dataSize          : " << msg.dataSize << prefix << "rangeResolution   : " << msg.rangeResolution << prefix
        << "nRanges           : " << msg.nRanges << prefix << "nBeams            : " << msg.nBeams << prefix
        << "spare0            : " << msg.spare0 << prefix << "spare1            : " << msg.spare1 << prefix
        << "spare2            : " << msg.spare2 << prefix << "spare3            : " << msg.spare3 << prefix
        << "imageOffset       : " << msg.imageOffset << prefix << "imageSize         : " << msg.imageSize << prefix
        << "messageSize       : " << msg.messageSize;
    return oss.str();
}

} // namespace oculus

std::ostream& operator<<(std::ostream& os, DataSizeType dataType)
{
    os << oculus::to_string(dataType);
    return os;
}

std::ostream& operator<<(std::ostream& os, PingRateType pingRate)
{
    os << oculus::to_string(pingRate);
    return os;
}

std::ostream& operator<<(std::ostream& os, OculusPartNumberType partNumber)
{
    os << oculus::to_string(partNumber);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusMessageHeader& msg)
{
    os << "OculusMessageHeader :" << oculus::to_string(msg);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusStatusMsg& msg)
{
    const std::string prefix("\n  - ");

    os << "OculusStatusMsg :"
       << "\n- header :" << oculus::to_string(msg.head, prefix) << "\n- status :" << oculus::to_string(msg, prefix);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusSimpleFireMessage& msg)
{
    const std::string prefix("\n  - ");
    os << "OculusSimpleFireMessage :"
       << "\n- header :" << oculus::to_string(msg.head, prefix) << "\n- simple fire :" << oculus::to_string(msg, prefix);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusSimplePingResult& msg)
{
    const std::string prefix("\n  - ");
    os << "OculusSimplePingMessage :"
       << "\n- header :" << oculus::to_string(msg.fireMessage.head, prefix)
       << "\n- simple fire :" << oculus::to_string(msg.fireMessage, prefix)
       << "\n- simple ping :" << oculus::to_string(msg, prefix);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusSimpleFireMessage2& msg)
{
    const std::string prefix("\n  - ");
    os << "OculusSimpleFireMessage2 :"
       << "\n- header :" << oculus::to_string(msg.head, prefix) << "\n- simple fire (v2) :" << oculus::to_string(msg, prefix);
    return os;
}

std::ostream& operator<<(std::ostream& os, const OculusSimplePingResult2& msg)
{
    const std::string prefix("\n  - ");
    os << "OculusSimplePingMessage2 :"
       << "\n- header :" << oculus::to_string(msg.fireMessage.head, prefix)
       << "\n- simple fire (v2) :" << oculus::to_string(msg.fireMessage, prefix)
       << "\n- simple ping (v2) :" << oculus::to_string(msg, prefix);
    return os;
}
