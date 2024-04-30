#include "AP_Periph.h"
#include <AP_GPS/RTCM3_Parser.h>

#if 0
#define Debug(...) do { hal.console->printf(__VA_ARGS__); } while(0)
#else
#define Debug(...)
#endif
 
/*
  update CAN GPS
 */
void AP_Periph_DroneCAN::can_gps_update(void)
{
    auto &gps = periph.gps;
    // we need to record this time as its reset when we call gps.update()
    uint64_t last_message_local_time_us = gps.last_pps_time_usec();
    gps.update();
    send_moving_baseline_msg();
    send_relposheading_msg();
    if (periph.last_gps_update_ms == gps.last_message_time_ms()) {
        return;
    }
    periph.last_gps_update_ms = gps.last_message_time_ms();
    // send time sync message
    uavcan_protocol_GlobalTimeSync ts {};
    for (uint8_t i=0; i<HAL_NUM_CAN_IFACES; i++) {
        uint8_t idx; // unused
        if ((gps.status() < AP_GPS::GPS_OK_FIX_3D) ||
            !gps.is_healthy() ||
            gps.first_unconfigured_gps(idx)) {
            // no good fix and no healthy GPS, so we don't send timesync
            break;
        }
        uint64_t last_message_epoch_usec = gps.last_message_epoch_usec();
        if (last_message_local_time_us != 0 &&
            last_message_epoch_usec != 0 &&
            canard_iface.get_node_id() != CANARD_BROADCAST_NODE_ID) {
            // (last_message_epoch_usec - last_message_local_time_us) represents the offset between the time in gps epoch and the local time of the node
            // periph.get_tracked_tx_timestamp(i) represent the offset timestamp in the local time of the node
            if (periph.get_tracked_tx_timestamp(i)) {
            ts.previous_transmission_timestamp_usec = (last_message_epoch_usec - last_message_local_time_us) + periph.get_tracked_tx_timestamp(i);
            } else {
                ts.previous_transmission_timestamp_usec = 0;
            }
            uint8_t buffer[UAVCAN_PROTOCOL_GLOBALTIMESYNC_MAX_SIZE] {};
            uint16_t total_size = uavcan_protocol_GlobalTimeSync::cxx_iface::encode(&ts, buffer, false);
            // create transfer object
            CanardTxTransfer transfer_object = {
                .transfer_type = CanardTransferTypeBroadcast,
                .data_type_signature = UAVCAN_PROTOCOL_GLOBALTIMESYNC_SIGNATURE,
                .data_type_id = UAVCAN_PROTOCOL_GLOBALTIMESYNC_ID,
                .inout_transfer_id = &timesync_tid[i],
                .priority = CANARD_TRANSFER_PRIORITY_HIGH,
                .payload = (uint8_t*)buffer,
                .payload_len = total_size,
                .canfd = false,
                .deadline_usec = AP_HAL::micros64()+1000000U,
                .iface_mask = uint8_t(1<<i),
            };
            canardBroadcastObj(&canard_iface.get_canard(), &transfer_object);
        }
    }

    {
        /*
          send Fix2 packet
        */
        uavcan_equipment_gnss_Fix2 pkt {};
        const Location &loc = gps.location();
        const Vector3f &vel = gps.velocity();

        if (gps.status() < AP_GPS::GPS_OK_FIX_2D && !periph.saw_lock_once) {
            pkt.timestamp.usec = AP_HAL::micros64();
            pkt.gnss_timestamp.usec = 0;
        } else {
            periph.saw_lock_once = true;
            pkt.timestamp.usec = gps.time_epoch_usec();
            pkt.gnss_timestamp.usec = gps.last_message_epoch_usec();
        }
        if (pkt.gnss_timestamp.usec == 0) {
            pkt.gnss_time_standard = UAVCAN_EQUIPMENT_GNSS_FIX_GNSS_TIME_STANDARD_NONE;
        } else {
            pkt.gnss_time_standard = UAVCAN_EQUIPMENT_GNSS_FIX_GNSS_TIME_STANDARD_UTC;
        }
        pkt.longitude_deg_1e8 = uint64_t(loc.lng) * 10ULL;
        pkt.latitude_deg_1e8 = uint64_t(loc.lat) * 10ULL;
        pkt.height_msl_mm = loc.alt * 10;
        float undulation = 0.0f;
        if (AP::gps().get_undulation(undulation)) {
            pkt.height_ellipsoid_mm = loc.alt * 10 - undulation * 1000;
        } else {
            pkt.height_ellipsoid_mm = 0.0f;
        }
        for (uint8_t i=0; i<3; i++) {
            pkt.ned_velocity[i] = vel[i];
        }
        pkt.sats_used = gps.num_sats();
        switch (gps.status()) {
        case AP_GPS::GPS_Status::NO_GPS:
        case AP_GPS::GPS_Status::NO_FIX:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_NO_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_SINGLE;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_DGPS_OTHER;
            break;
        case AP_GPS::GPS_Status::GPS_OK_FIX_2D:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_2D_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_SINGLE;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_DGPS_OTHER;
            break;
        case AP_GPS::GPS_Status::GPS_OK_FIX_3D:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_3D_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_SINGLE;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_DGPS_OTHER;
            break;
        case AP_GPS::GPS_Status::GPS_OK_FIX_3D_DGPS:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_3D_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_DGPS;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_DGPS_SBAS;
            break;
        case AP_GPS::GPS_Status::GPS_OK_FIX_3D_RTK_FLOAT:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_3D_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_RTK;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_RTK_FLOAT;
            break;
        case AP_GPS::GPS_Status::GPS_OK_FIX_3D_RTK_FIXED:
            pkt.status = UAVCAN_EQUIPMENT_GNSS_FIX2_STATUS_3D_FIX;
            pkt.mode = UAVCAN_EQUIPMENT_GNSS_FIX2_MODE_RTK;
            pkt.sub_mode = UAVCAN_EQUIPMENT_GNSS_FIX2_SUB_MODE_RTK_FIXED;
            break;
        }

        pkt.covariance.len = 6;

        float hacc;
        if (gps.horizontal_accuracy(hacc)) {
            pkt.covariance.data[0] = pkt.covariance.data[1] = sq(hacc);
        }
    
        float vacc;
        if (gps.vertical_accuracy(vacc)) {
            pkt.covariance.data[2] = sq(vacc);
        }

        float sacc;
        if (gps.speed_accuracy(sacc)) {
            float vc3 = sq(sacc);
            pkt.covariance.data[3] = pkt.covariance.data[4] = pkt.covariance.data[5] = vc3;
        }
        
#if defined(HAL_PERIPH_ENABLE_GPS) && GPS_MOVING_BASELINE
    // PX4 relies on the ecef_position_velocity field for heading, heading offset and heading accuracy,
    // since there is no support for RelPosHeading as of 1.14 stable
    float yaw;
    float yaw_accuracy;
    uint32_t curr_timestamp;
    static float last_timestamp = 0;

    if (gps.gps_yaw_deg(yaw, yaw_accuracy, curr_timestamp) && curr_timestamp != last_timestamp){
        pkt.ecef_position_velocity.len = 1;
        pkt.ecef_position_velocity.data[0].velocity_xyz[0] = wrap_PI(yaw * DEG_TO_RAD);
        pkt.ecef_position_velocity.data[0].velocity_xyz[1] = 0; // Offset already compensated for.
        pkt.ecef_position_velocity.data[0].velocity_xyz[2] = yaw_accuracy * DEG_TO_RAD;
    }
    last_timestamp = curr_timestamp;
#endif

        fix2_pub.broadcast(pkt);
    }
    
    /*
      send aux packet
     */
    {
        uavcan_equipment_gnss_Auxiliary aux {};
        aux.hdop = gps.get_hdop() * 0.01;
        aux.vdop = gps.get_vdop() * 0.01;

        aux_pub.broadcast(aux);
    }

    // send the gnss status packet
    {
        ardupilot_gnss_Status status {};

        status.healthy = gps.is_healthy();
        if (gps.logging_present() && gps.logging_enabled() && !gps.logging_failed()) {
            status.status |= ARDUPILOT_GNSS_STATUS_STATUS_LOGGING;
        }
        uint8_t idx; // unused
        if (status.healthy && !gps.first_unconfigured_gps(idx)) {
            status.status |= ARDUPILOT_GNSS_STATUS_STATUS_ARMABLE;
        }

        uint32_t error_codes;
        if (gps.get_error_codes(error_codes)) {
            status.error_codes = error_codes;
        }

        gnss_status_pub.broadcast(status);
    }
}

void AP_Periph_DroneCAN::send_moving_baseline_msg()
{
#if defined(HAL_PERIPH_ENABLE_GPS) && GPS_MOVING_BASELINE
    auto &gps = periph.gps;
    const uint8_t *data = nullptr;
    uint16_t len = 0;
    if (!gps.get_RTCMV3(data, len)) {
        return;
    }
    if (len == 0 || data == nullptr) {
        return;
    }
    // send the packet from Moving Base to be used RelPosHeading calc by GPS module
    ardupilot_gnss_MovingBaselineData mbldata {};
    // get the data from the moving base
    // static_assert(sizeof(ardupilot_gnss_MovingBaselineData::data.data) == RTCM3_MAX_PACKET_LEN, "Size of Moving Base data is wrong");
    while (len != 0) {
        mbldata.data.len = MIN(len, sizeof(mbldata.data.data));
        memcpy(mbldata.data.data, data, mbldata.data.len);
        moving_baseline_pub.broadcast(mbldata);
        len -= mbldata.data.len;
        data += mbldata.data.len;
    }
    gps.clear_RTCMV3();
#endif // HAL_PERIPH_ENABLE_GPS && GPS_MOVING_BASELINE
}

void AP_Periph_DroneCAN::send_relposheading_msg() {
#if defined(HAL_PERIPH_ENABLE_GPS) && GPS_MOVING_BASELINE
    auto &gps = periph.gps;
    float reported_heading;
    float relative_distance;
    float relative_down_pos;
    float reported_heading_acc;
    static uint32_t last_timestamp = 0;
    uint32_t curr_timestamp = 0;
    gps.get_RelPosHeading(curr_timestamp, reported_heading, relative_distance, relative_down_pos, reported_heading_acc);
    if (last_timestamp == curr_timestamp) {
        return;
    }
    last_timestamp = curr_timestamp;
    ardupilot_gnss_RelPosHeading relpos {};
    relpos.timestamp.usec = uint64_t(curr_timestamp)*1000LLU;
    relpos.reported_heading_deg = reported_heading;
    relpos.relative_distance_m = relative_distance;
    relpos.relative_down_pos_m = relative_down_pos;
    relpos.reported_heading_acc_deg = reported_heading_acc;
    relpos.reported_heading_acc_available = true;

    relposheading_pub.broadcast(relpos);
#endif // HAL_PERIPH_ENABLE_GPS && GPS_MOVING_BASELINE
}


/*
  handle gnss::RTCMStream
 */
void AP_Periph_DroneCAN::handle_RTCMStream(const CanardRxTransfer& transfer, const uavcan_equipment_gnss_RTCMStream &req)
{
    periph.gps.handle_gps_rtcm_fragment(0, req.data.data, req.data.len);
}

/*
    handle gnss::MovingBaselineData
*/
#if GPS_MOVING_BASELINE
void AP_Periph_DroneCAN::handle_MovingBaselineData(const CanardRxTransfer& transfer, const ardupilot_gnss_MovingBaselineData &msg)
{
    periph.gps.inject_MBL_data((uint8_t*)msg.data.data, msg.data.len);
    Debug("MovingBaselineData: len=%u\n", msg.data.len);
}
#endif // GPS_MOVING_BASELINE
