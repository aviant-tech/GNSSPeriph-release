#pragma once
#include <AP_Param/AP_Param.h>

#ifdef ENABLE_BASE_MODE
class GPS_Rover {
public:
    GPS_Rover();
    void update();
    static const struct AP_Param::GroupInfo var_info[];
private:
    void init();
    void handle_feedback(uint8_t pin, bool pin_state, uint32_t timestamp);
    void ubx_rawdata_cb(const uint8_t *data, uint32_t length);
    bool _initialized;
    AP_Int8 _enabled;
    AP_Int8 _debug;
    int ubx_log_fd = -1;
    char _ubx_log_filename[48];
};

#endif