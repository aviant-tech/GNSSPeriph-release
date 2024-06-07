#include <AP_HAL/AP_HAL_Boards.h>
#include "AP_Periph.h"

extern const AP_HAL::HAL &hal;

#ifndef HAL_PERIPH_LED_BRIGHT_DEFAULT
#define HAL_PERIPH_LED_BRIGHT_DEFAULT 100
#endif

#ifndef HAL_PERIPH_RANGEFINDER_BAUDRATE_DEFAULT
#define HAL_PERIPH_RANGEFINDER_BAUDRATE_DEFAULT 115200
#endif

#ifndef HAL_PERIPH_RANGEFINDER_PORT_DEFAULT
#define HAL_PERIPH_RANGEFINDER_PORT_DEFAULT 3
#endif


#ifndef HAL_PERIPH_ADSB_BAUD_DEFAULT
#define HAL_PERIPH_ADSB_BAUD_DEFAULT 57600
#endif
#ifndef HAL_PERIPH_ADSB_PORT_DEFAULT
#define HAL_PERIPH_ADSB_PORT_DEFAULT 1
#endif

#ifndef AP_PERIPH_MSP_PORT_DEFAULT
#define AP_PERIPH_MSP_PORT_DEFAULT 1
#endif

#ifndef HAL_DEFAULT_MAV_SYSTEM_ID
#define MAV_SYSTEM_ID 3
#else
#define MAV_SYSTEM_ID HAL_DEFAULT_MAV_SYSTEM_ID
#endif

#define GSCALAR_HIDDEN(v, name, def)                { name, &AP_PARAM_VEHICLE_NAME.g.v,                   {def_value : def},                   AP_PARAM_FLAG_HIDDEN,                                                  Parameters::k_param_ ## v,          AP_PARAM_VEHICLE_NAME.g.v.vtype }
/*
 *  AP_Periph parameter definitions
 *
 */
#define AP_PARAM_VEHICLE_NAME periph

const AP_Param::Info AP_Periph_FW::var_info[] = {
    // @Param: FORMAT_VERSION
    // @DisplayName: Eeprom format version number
    // @Description: This value is incremented when changes are made to the eeprom format
    // @User: Advanced
    GSCALAR(format_version,         "FORMAT_VERSION", 0),

    // @Param: CAN_NODE
    // @DisplayName: UAVCAN node that is used for this network
    // @Description: UAVCAN node should be set implicitly or 0 for dynamic node allocation
    // @Range: 0 250
    // @User: Advanced
    // @RebootRequired: True
    GSCALAR(can_node,         "CAN_NODE", HAL_CAN_DEFAULT_NODE_ID),

    // @Param: CAN_BAUDRATE
    // @DisplayName: Bitrate of CAN interface
    // @Description: Bit rate can be set up to from 10000 to 1000000
    // @Range: 10000 1000000
    // @User: Advanced
    // @RebootRequired: True
    GARRAY(can_baudrate,     0, "CAN_BAUDRATE", 1000000),

#if HAL_NUM_CAN_IFACES >= 2
    // @Param: CAN2_BAUDRATE
    // @DisplayName: Bitrate of CAN2 interface
    // @Description: Bit rate can be set up to from 10000 to 1000000
    // @Range: 10000 1000000
    // @User: Advanced
    // @RebootRequired: True
    GARRAY(can_baudrate,     1, "CAN2_BAUDRATE", 1000000),
#endif

#if !defined(HAL_NO_FLASH_SUPPORT) && !defined(HAL_NO_ROMFS_SUPPORT)
    // @Param: FLASH_BOOTLOADER
    // @DisplayName: Trigger bootloader update
    // @Description: DANGER! When enabled, the App will perform a bootloader update by copying the embedded bootloader over the existing bootloader. This may take a few seconds to perform and should only be done if you know what you're doing.
    // @Range: 0 1
    // @User: Advanced
    GSCALAR(flash_bootloader,     "FLASH_BOOTLOADER", 0),
#endif

    // @Param: DEBUG
    // @DisplayName: Debug
    // @Description: Debug
    // @Bitmask: 0:Show free stack space, 2:Enable sending stats
    // @User: Advanced
    GSCALAR(debug, "DEBUG", 0),

    // @Param: BRD_SERIAL_NUM
    // @DisplayName: Serial number of device
    // @Description: Non-zero positive values will be shown on the CAN App Name string
    // @Range: 0 2147483648
    // @User: Advanced
    GSCALAR(serial_number, "BRD_SERIAL_NUM", 0),

    // GPS driver
    // @Group: GPS
    // @Path: ../libraries/AP_GPS/AP_GPS.cpp
    GOBJECT(gps, "GPS", AP_GPS),

    // GPS TYPE
    // @Param: GPS_TYPE
    // @DisplayName: GPS Type
    // @Description: GPS Type
    GSCALAR_HIDDEN(gps_type, "GPS_TYPE", 0),

#ifdef HAL_PERIPH_ENABLE_MAG
    // @Group: COMPASS_
    // @Path: ../libraries/AP_Compass/AP_Compass.cpp
    GOBJECT(compass,         "COMPASS_",     Compass),
#endif

#ifdef HAL_PERIPH_ENABLE_BARO
    // Baro driver
    // @Group: BARO
    // @Path: ../libraries/AP_Baro/AP_Baro.cpp
    GOBJECT(baro, "BARO", AP_Baro),

    // @Param: BARO_ENABLE
    // @DisplayName: Barometer Enable
    // @Description: Barometer Enable
    // @Values: 0:Disabled, 1:Enabled
    // @User: Standard
    GSCALAR(baro_enable, "BARO_ENABLE", 0),
#endif
    
    // @Group: NTF_
    // @Path: ../libraries/AP_Notify/AP_Notify.cpp
    GOBJECT(notify, "NTF_",  AP_Notify),

    // @Param: SYSID_THISMAV
    // @DisplayName: MAVLink system ID of this vehicle
    // @Description: Allows setting an individual system id for this vehicle to distinguish it from others on the same network
    // @Range: 1 255
    // @User: Advanced
    GSCALAR(sysid_this_mav,         "SYSID_THISMAV",  MAV_SYSTEM_ID),

    // @Group: SERIAL
    // @Path: ../libraries/AP_SerialManager/AP_SerialManager.cpp
    GOBJECT(serial_manager, "SERIAL",   AP_SerialManager),

#if AP_SCRIPTING_ENABLED
    // @Group: SCR_
    // @Path: ../libraries/AP_Scripting/AP_Scripting.cpp
    GOBJECT(scripting, "SCR_", AP_Scripting),
#endif

    // can node FD Out mode
    GSCALAR(can_fdmode,     "CAN_FDMODE", 0),

    // can node FD Out baudrate
    GARRAY(can_fdbaudrate, 0,     "CAN_FDBAUDRATE", 8000000),

#if HAL_NUM_CAN_IFACES >= 2
    // can node FD Out baudrate
    GARRAY(can_fdbaudrate, 1,     "CAN2_FDBAUDRATE", 8000000),
#endif

#ifdef I2C_SLAVE_ENABLED
    // select serial i2c mode and disable can gps
    GSCALAR(serial_i2c_mode,     "SER_I2C_MODE", 0),
#endif
    //select terminator setting
    GARRAY(can_terminator, 0,    "CAN_TERMINATOR", 0),

#if HAL_NUM_CAN_IFACES >= 2
    // can node FD Out baudrate
    GARRAY(can_terminator, 1,    "CAN2_TERMINATOR", 0),
#endif

    // CubeID firmware update
    GSCALAR(cubeid_fw_update_enabled,    "CUBEID_FW_UPDATE", 1),

#ifdef ENABLE_BASE_MODE
    // @Param: B
    // @Path: GPS_Base.cpp
    GOBJECT(gps_base, "B",  GPS_Base),
#endif

#if GPS_MOVING_BASELINE // TODO
    // @Param: MB_CAN_PORT
    // @DisplayName: Moving Baseline CAN Port option
    // @Description: Autoselect dedicated CAN port on which moving baseline data will be transmitted.
    // @Values: 0:Sends moving baseline data on all ports, other options not available
    // @User: Advanced
    // @RebootRequired: True
    GSCALAR(gps_mb_only_can_port, "GPS_MB_ONLY_PORT", 0),
#endif

// @Param: GPS_RTK_FLT
// @DisplayName: RTK data filter: Only accept RTK corrections from the following UAVCAN ids
// @Description: Only accept
// @Values: 0: Disabled, Other values: UAVCAN node id.
// @User: Advanced
// @RebootRequired: True
GSCALAR(gps_rtk_flt, "GPS_RTK_FLT", 0),

#if AP_INERTIALSENSOR_ENABLED
    // @Param: IMU_SAMPLE_RATE
    // @DisplayName: IMU Sample Rate
    // @Description: IMU Sample Rate
    // @Range: 0 1000
    // @User: Standard
    GSCALAR(imu_sample_rate, "IMU_SAMPLE_RATE", 0),

    // @Group: INS_
    // @Path: ardupilot/libraries/AP_InertialSensor/AP_InertialSensor.cpp
    GOBJECT(imu, "INS", AP_InertialSensor),
#endif

#ifdef GPIO_UBX_SAFEBOOT
    // @Param: GPS_SAFEBOOT
    // @DisplayName: GPS Safeboot
    // @Description: GPS Safeboot
    // @Values: 0:Disabled, 1:Enabled
    // @User: Standard
    GSCALAR(gps_safeboot, "GPS_SAFEBOOT", 0),
#endif

#ifdef ENABLE_BASE_MODE
    // @Param: ROVER
    // @Path: GPS_Rover.cpp
    GOBJECT(gps_rover, "ROVER",  GPS_Rover),
#endif

    AP_VAREND
};


void AP_Periph_FW::load_parameters(void)
{
    AP_Param::setup_sketch_defaults();

    AP_Param::check_var_info();

    if (!g.format_version.load() ||
        g.format_version != Parameters::k_format_version) {
        // erase all parameters
        StorageManager::erase();
        AP_Param::erase_all();

        // save the current format version
        g.format_version.set_and_save(Parameters::k_format_version);
    }

    // Load all auto-loaded EEPROM variables
    AP_Param::load_all();
}
