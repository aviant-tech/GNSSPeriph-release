#include "AP_Periph.h"
#include <AP_Filesystem/AP_Filesystem.h>
#include <stdio.h>

#ifdef ENABLE_BASE_MODE
// table of user settable parameters
const AP_Param::GroupInfo GPS_Rover::var_info[] = {
    // @Param: ENABLE
    // @DisplayName: Enable GPS Rover Mode
    // @Description: Enable GPS Rover Mode
    // @User: Standard
    AP_GROUPINFO_FLAGS("_ENABLE",  1, GPS_Rover, _enabled, 0, AP_PARAM_FLAG_ENABLE),

    AP_GROUPEND
};

GPS_Rover::GPS_Rover() {
    // setup parameters
    AP_Param::setup_object_defaults(this, var_info);
}

void GPS_Rover::init() {
    if (!_enabled) {
        return;
    }
    // set gps raw data if not set
    float value;
    AP_Param::get("GPS_RAW_DATA", value);
    if (value < 1.0) {
        AP_Param::set_and_save_by_name("GPS_RAW_DATA", 1);
    }
    // set callback for GPS RAW data
    AP::gps().set_gps_raw_cb(FUNCTOR_BIND_MEMBER(&GPS_Rover::ubx_rawdata_cb, void, const uint8_t*, uint32_t));
}

void GPS_Rover::update() {
    // do nothing if not enabled
    if (!_initialized && _enabled && (AP::gps().status() >= AP_GPS::GPS_OK_FIX_3D)) {
        init();
        _initialized = true;
    }
}

void GPS_Rover::ubx_rawdata_cb(const uint8_t *data, uint32_t length) {
    if (ubx_log_fd == -1) {
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
        AP::FS().write(ubx_log_fd, data, length);
        AP::FS().fsync(ubx_log_fd);
    }
}

#endif // ENABLE_BASE_MODE