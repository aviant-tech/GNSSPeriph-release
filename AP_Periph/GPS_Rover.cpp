#include "AP_Periph.h"
#include <AP_Filesystem/AP_Filesystem.h>
#include <stdio.h>
#include <hal.h>

#ifdef ENABLE_BASE_MODE
// table of user settable parameters
const AP_Param::GroupInfo GPS_Rover::var_info[] = {
    // @Param: ENABLE
    // @DisplayName: Enable GPS Rover Mode
    // @Description: Enable GPS Rover Mode
    // @User: Standard
    AP_GROUPINFO_FLAGS("_ENABLE",  1, GPS_Rover, _enabled, 0, AP_PARAM_FLAG_ENABLE),

    // @Param: DEBUG
    // @DisplayName: Print Debug Messages
    // @Description: Print Debug Messages
    // @User: Standard
    AP_GROUPINFO("_DEBUG",  2, GPS_Rover, _debug, 0),

    AP_GROUPEND
};

extern const AP_HAL::HAL &hal;

GPS_Rover::GPS_Rover() {
    // setup parameters
    AP_Param::setup_object_defaults(this, var_info);
}

void GPS_Rover::init() {
    if (!_enabled) {
        return;
    }
    can_printf("GPS Rover Mode Enabled\n");
    // set gps raw data if not set
    float value;
    AP_Param::get("GPS_RAW_DATA", value);
    if (value < 1.0) {
        AP_Param::set_and_save_by_name("GPS_RAW_DATA", 1);
    }
    // set callback for GPS RAW data
    AP::gps().set_gps_raw_cb(FUNCTOR_BIND_MEMBER(&GPS_Rover::ubx_rawdata_cb, void, const uint8_t*, uint32_t));

    // Set PA13 and PA14 mode as input
    palSetLineMode(HAL_GPIO_PIN_JTCK_SWCLK, PAL_MODE_INPUT);
    palSetLineMode(HAL_GPIO_PIN_JTMS_SWDIO, PAL_MODE_INPUT);

    // set callback for EXTERN_GPIO1
    hal.gpio->attach_interrupt(EXTERN_GPIO1, FUNCTOR_BIND_MEMBER(&GPS_Rover::handle_feedback, void, uint8_t, bool, uint32_t), AP_HAL::GPIO::INTERRUPT_BOTH);
}

void GPS_Rover::handle_feedback(uint8_t pin, bool pin_state, uint32_t timestamp)
{
    // trigger EXTINT
    if (pin == EXTERN_GPIO1) {
        hal.gpio->write(GPS_EXTINT, pin_state);
    }
}

void GPS_Rover::update() {
    // do nothing if not enabled
    if (!_initialized && _enabled && (AP::gps().status() >= AP_GPS::GPS_OK_FIX_3D)) {
        init();
        _initialized = true;
    }
}

void GPS_Rover::ubx_rawdata_cb(const uint8_t *data, uint32_t length) {
    if (ubx_log_fd == -1 && (AP::gps().status() >= AP_GPS::GPS_OK_FIX_3D) && AP::gps().time_week()) {    
        struct stat st;
        int ret = AP::FS().stat("rover", &st);
        if (ret == -1) {
            ret = AP::FS().mkdir("rover");
        }
        date_time dt;
        GPS_Base::gps_week_time(dt, AP::gps().time_week(), AP::gps().time_week_ms());
        snprintf(_ubx_log_filename, sizeof(_ubx_log_filename), "/rover/UTC_%04d_%02d_%02d_%02d_%02d_%02d.ubx", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        ubx_log_fd = AP::FS().open(_ubx_log_filename, O_CREAT | O_WRONLY | O_TRUNC);
        if (ubx_log_fd < 0) {
            can_printf("Failed to create rover log file %d\n", errno);
        } else {
            can_printf("Opened rover log file %s\n", _ubx_log_filename);
        }
        if (!AP::FS().set_mtime(_ubx_log_filename, dt.utc_sec)) {
            can_printf("Failed to set file time %s\n", strerror(errno));
        }
    }

    if (ubx_log_fd != -1) {
        // check if UBX_TIM_TM2 is received
        if ((data[2] == 0x0D) && (data[3] == 0x03) && _debug) {
            can_printf("Received UBX_TIM_TM2!\n");
        }
        AP::FS().write(ubx_log_fd, data, length);
        AP::FS().fsync(ubx_log_fd);
    }
}

#endif // ENABLE_BASE_MODE