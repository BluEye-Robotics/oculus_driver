/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

// Changed pragma once to classical header guards to be able to include both
// this file and the deprecated driver library header file in the same source
// file.  (To be able to open data generated by the old driver and convert them
// to the new library format.).  With the pragma once approach both this header
// and the one from the old library are included and it (rightfully) cause
// multiple definition errors.
#pragma once

#include <stdint.h>

// ----------------------------------------------------------------------------
// Collection of data classes provided by JGS
// updated 10/11/15 for ping is etc
// updated 07/12/15 for additional fields

// All structures are single byte packed // carefull with this check for compiler compatibility
#pragma pack(push, 1)

// The test id contained in the oculus header file
#define OCULUS_CHECK_ID 0x4f53

namespace oculus {

enum OculusMasterStatusType // : uint8_t
{
    MasterStatusSsblBoot,
    MasterStatusSsblRun,
    MasterStatusMainBoot,
    MasterStatusMainRun,
};

enum OculusPauseReasonType // : uint8_t
{
    PauseMagSwitch = 0,
    PauseBootFromMain = 1,
    PauseFlashError = 2,
    PauseFirmwareError = 4,
    PauseCompatibilityError = 5,
    PauseBrownout = 6,
    PauseUndefined = 7,
};

enum OculusTemperatureStatusType // : uint8_t
{
    TempGood = 0,
    TempOverheat = 1,
    TempReserved = 2,
    TempOverMax = 3,
};

// -----------------------------------------------------------------------------
enum OculusDeviceType // : uint16_t
{
    DeviceTypeUndefined = 0,
    DeviceTypeImagingSonar = 1,
};

// -----------------------------------------------------------------------------
enum OculusMessageType // : uint16_t
{
    MsgStatus = 1,
    MsgSimpleFire = 21,
    MsgSimplePingResult = 35,
    MsgPingResult = 34,
    MsgUserConfig = 85,
    MsgBootInfo = 128,
    MsgDummy = 255,
};

enum PingRateType // : uint8_t
{
    PingRateNormal = 0x00, // 10Hz max ping rate (Default rate)
    PingRateHigh = 0x01, // 15Hz max ping rate
    PingRateHighest = 0x02, // 40Hz max ping rate
    PingRateLow = 0x03, // 5Hz max ping rate
    PingRateLowest = 0x04, // 2Hz max ping rate
    PingRateStandby = 0x05, // Disable ping
};

// -----------------------------------------------------------------------------
enum DataSizeType // : uint8_t
{
    ImageData8Bit = 0,
    ImageData16Bit = 1,
    ImageData24Bit = 2,
    ImageData32Bit = 3,
};

// -----------------------------------------------------------------------------
enum OculusPartNumberType // : uint16_t
{
    Undefined = 0,
    M370s = 1041,
    MT370s = 2418,
    MD370s = 1433,
    MF370s = 1436,
    MA370s = 1229,
    M750d = 1032,
    MT750d = 2419,
    MD750d = 1434,
    MF750d = 1134,
    MA750d = 1135,
    M1200d = 1042,
    MT1200d = 2420,
    MD1200d = 1435,
    MF1200d = 1437,
    MA1200d = 1228,
    M3000d = 2203,
    MT3000d = 2599,
    MF3000d = 2466,
    MA3000d = 2924,
    partNumberEnd = 0xFFFF
};

typedef struct
{
    uint16_t oculusId; // Fixed for Oculus Sonar: 0x4f53
    uint16_t srcDeviceId; // The device id of the source
    uint16_t dstDeviceId; // The device id of the destination
    uint16_t msgId; // Message identifier
    uint16_t msgVersion;
    uint32_t payloadSize; // The size of the message payload (header not included)
    uint16_t partNumber;
} OculusMessageHeader;

// -----------------------------------------------------------------------------
typedef struct
{
    OculusMessageHeader head; // The standard message header

    uint8_t masterMode; // mode 1 - Low Frequency Mode (wide aperture, navigation)
                        // mode 2 - High Frequency Mode (narrow aperture, target identification)
    uint8_t pingRate; // Sets the maximum ping rate. was PingRateType
    uint8_t networkSpeed; // Used to reduce the network comms speed (useful for high latency shared links)
    uint8_t gammaCorrection; // 0 and 0xff = gamma correction = 1.0
                             // Set to 127 for gamma correction = 0.5
    uint8_t flags; // bit 0: [RangeInMetres]  1: Metres. 0: Percentage.
                   // bit 1: [16BitImg]       1: 16-bit. 0: 8-bit.
                   // bit 2: [GainSend]       1: Return the gain at the start of each line. 0: No gain return
                   // bit 3: [SimpleReturn]   1: Ouput simple fire returns. 0: Output full fire returns if full fire sent.
                   // bit 4: [GainAssist]     1: Gain assist disabled. 0: Gain assist enabled.
                   // bit 5: [LowPower]       1: Low power mode enabled. 0: Disabled.
                   // bit 6: [FullBeams]      1: Use 512 beams. 0: Use 256 beams.
                   // bit 7: [NetworkTrigger] 1: Only fires when instructed. 0: Fires automatically according to PingRate.
    double range; // The range demand in percent or m depending on flags
    double gain; // The gain demand
    double speedOfSound; // ms-1, if set to zero then internal calc will apply using salinity
    double salinity; // ppt, set to zero if we are in fresh water
} OculusSimpleFireMessage;

typedef struct
{
    OculusMessageHeader head;
    uint8_t masterMode;
    uint8_t pingRate; // was PingRateType
    uint8_t networkSpeed; // The max network speed in Mbs , set to 0x00 or 0xff to use link speed
    uint8_t gammaCorrection; // The gamma correction - 255 is equal to a gamma correction of 1.0
    uint8_t flags;
    double range; // The range demand (%)
    double gain; // The percentage gain
    double speedOfSound; // The speed of sound - set to zero to use internal calculations
    double salinity; // THe salinity to be used with internal speed of sound calculations (ppt)
    uint32_t extFlags;
    uint32_t reserved0[2];
    uint32_t beaconLocatorFrequency;
    uint32_t reserved1[5];
} OculusSimpleFireMessage2;

// -----------------------------------------------------------------------------
typedef struct
{
    OculusSimpleFireMessage fireMessage;
    uint32_t pingId; // An incrementing number
    uint32_t status;
    double frequency; // The acoustic frequency (Hz)
    double temperature; // The external temperature (deg C)
    double pressure; // The external pressure (bar)
    double
        speeedOfSoundUsed; // The actual used speed of sound (m/s). May be different to the speed of sound set in the fire message
    uint32_t pingStartTime;
    uint8_t dataSize; // The size of the individual data entries   // was DataSizeType
    double rangeResolution; // The range in metres corresponding to a single range line
    uint16_t nRanges; // The number of range lines in the image
    uint16_t nBeams; // The number of bearings in the image
    uint32_t imageOffset; // The offset in bytes of the image data from the start of the network message
    uint32_t imageSize; // The size in bytes of the image data
    uint32_t messageSize; // The total size in bytes of the network message
    // *** NOT ADDITIONAL VARIABLES BEYOND THIS POINT ***
    // There will be an array of bearings (shorts) found at the end of the message structure
    // Allocated at run time
    // short bearings[];
    // The bearings to each of the beams in 0.01 degree resolution
} OculusSimplePingResult;

typedef struct
{
    OculusSimpleFireMessage2 fireMessage;
    uint32_t pingId; // An incrementing number
    uint32_t status;
    double frequency; // The acoustic frequency (Hz)
    double temperature; // The external temperature (deg C)
    double pressure; // The external pressure (bar)
    double heading; // The heading (degrees)
    double pitch; // The pitch (degrees)
    double roll; // The roll (degrees)
    double speeedOfSoundUsed; // The actual used speed of sound (m/s)
    double pingStartTime; // In seconds from sonar powerup (to microsecond resolution)
    uint8_t dataSize; // The size of the individual data entries  // was DataSizeType
    double rangeResolution; // The range in metres corresponding to a single range line
    uint16_t nRanges; // The number of range lines in the image
    uint16_t nBeams; // The number of bearings in the image
    uint32_t spare0;
    uint32_t spare1;
    uint32_t spare2;
    uint32_t spare3;
    uint32_t imageOffset; // The offset in bytes of the image data from the start
    uint32_t imageSize; // The size in bytes of the image data
    uint32_t messageSize; // The total size in bytes of the network message
    // uint16_t bearings[OSS_MAX_BEAMS]; // The brgs of the formed beams in 0.01 degree resolution
} OculusSimplePingResult2;

// -----------------------------------------------------------------------------
typedef struct
{
    uint32_t arm0Version0; // The arm0 firmware version major(8 bits), minor(8 bits), build (16 bits)
    uint32_t arm0Date0; // The arm0 firmware date
    uint32_t arm1Version1; // The arm1 firmware version major(8 bits), minor(8 bits), build (16 bits)
    uint32_t arm1Date1; // The arm1 firmware date
    uint32_t coreVersion2; // The bitfile version
    uint32_t coreDate2; // The bitfile date
} OculusVersionInfo;

// -----------------------------------------------------------------------------
typedef struct
{
    OculusMessageHeader head;

    uint32_t deviceId;
    uint16_t deviceType; // was OculusDeviceType
    uint16_t partNumber; // was OculusPartNumberType
    uint32_t status;
    OculusVersionInfo versionInfo;
    uint32_t ipAddr;
    uint32_t ipMask;
    uint32_t clientAddr;
    uint8_t macAddr0;
    uint8_t macAddr1;
    uint8_t macAddr2;
    uint8_t macAddr3;
    uint8_t macAddr4;
    uint8_t macAddr5;
    double temperature0;
    double temperature1;
    double temperature2;
    double temperature3;
    double temperature4;
    double temperature5;
    double temperature6;
    double temperature7;
    double pressure;
} OculusStatusMsg;

typedef struct
{
    uint32_t ipAddr;
    uint32_t ipMask;
    uint32_t dhcpEnable;
} OculusUserConfig;

typedef struct
{
    OculusMessageHeader head;
    OculusUserConfig config;
} OculusUserConfigMessage;

typedef struct
{
    uint8_t b0;
    double d0;
    double range;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
    uint16_t nBeams;
    double d7;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint16_t u0;
    uint8_t b7;
    uint8_t b8;
    uint8_t b9;
    uint8_t b10;
    uint8_t b11;
    uint8_t b12;
    uint8_t b13;
    uint8_t b14;
    uint8_t b15;
    uint8_t b16;
    uint16_t u1;
} PingConfig;

typedef struct
{
    uint8_t b0;
    double d0;
    uint16_t u0;
    uint16_t u1;
} s0;

typedef struct
{
    uint8_t b0;
} s2;

typedef struct
{
    uint8_t b0;
    uint8_t b1;
} s7;

typedef struct
{
    int i0;
    int i1;
    int i2;
    int i3;
    int i4;
    int i5;
} s9;

typedef struct
{
    uint8_t b0;
    double d0;
    double d1;
} s8;

typedef struct
{
    uint8_t b0;
    uint16_t u0;
    uint8_t b1;
    double d0;
} s1;

typedef struct
{
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint8_t b7;
    uint8_t b8;
    uint8_t b9;
    uint8_t b10;
    uint8_t b11;
    uint8_t b12;
    uint8_t b13;
    uint8_t b14;
    uint8_t b15;
    uint16_t u0;
    uint8_t b16;
    double d0;
    double d1;
} s3;

typedef struct
{
    uint8_t b0;
    uint8_t b1;
    double d0;
    double d1;
    double d2;
} s4;

typedef struct
{

    uint8_t b0;
    uint8_t b1;
    uint16_t u0;
    uint16_t u1;
    uint16_t u2;
    uint16_t u3;
    uint16_t u4;
    uint16_t u5;
} s5;

typedef struct
{
    double d0;
    double d1;
} s6;

typedef struct
{
    int i0;
    int i1;
    int i2;
    int i3;
} s10;

typedef struct
{
    double d0;
    double d1;
    double d2;
    double d3;
    double d4;
} s11;

typedef struct
{
    double d0;
    double d1;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
} s12;

typedef struct
{
    uint32_t u0;
    uint32_t u1;
    double d1;
    double d2;
    uint32_t u2;
    uint32_t u3;

    double d3;
    double d4;
    double d5;
    double d6;
    double d7;
    double d8;
    double d9;
    double d10;
    double d11;
    double d12;
    double d13;
    double d14;
    double d15;
    double d16;
    double d17;
    double d18;
    double d19;
    double d20;

    uint32_t u4;
    uint32_t nRangeLinesBfm;
    uint16_t u5;
    uint16_t u6;
    uint16_t u7;
    uint32_t u8;
    uint32_t u9;
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint32_t imageOffset; // The offset in bytes of the image data (CHN, CQI, BQI or BMG) from the start of the buffer
    uint32_t imageSize; // The size in bytes of the image data (CHN, CQI, BQI or BMG)
    uint32_t messageSize; // The total size in bytes of the network message
    // *** NOT ADDITIONAL VARIABLES BEYOND THIS POINT ***
    // There will be an array of bearings (shorts) found at the end of the message structure
    // Allocated at run time
    // short bearings[];
    // The bearings to each of the beams in 0.01 degree resolution
} PingParameters;

typedef struct
{
    OculusMessageHeader head;
    PingConfig ping;
    s0 t0;
    s1 t1;
    s2 t2;
    s3 t3;
    s4 t4;
    s5 t5;
    s6 t6;
    s7 t7;
    s8 t8;
    s9 t9;
    s10 t10;
    s11 t11;
    s12 t12;
    PingParameters ping_params;
} OculusReturnFireMessage;

}  // namespace oculus

#pragma pack(pop)
