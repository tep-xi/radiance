#pragma once

#include <stdint.h>

#include "output/slice.h"

// https://github.com/robot-head/PixelPusher-python/blob/master/heroicrobot/pixelpusher/discovery.py
// https://github.com/hzeller/rpi-matrix-pixelpusher/blob/master/universal-discovery-protocol.h
struct __attribute__((__packed__)) discovery_packet_header {
    uint8_t mac_addr[6];
    // Network byte order
    uint32_t ip_addr;
    uint8_t device_type;
    // Device protocol version, not discovery
    uint8_t protocol_version;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t hw_revision;
    uint16_t sw_revision;
    // Bits per second
    uint32_t link_speed;
};

struct __attribute__((__packed__)) pixel_pusher_info {
    uint8_t strips_attached;
    uint8_t max_strips_per_packet;
    uint16_t pixels_per_strip;
    // Microseconds
    uint32_t update_period;
    // PWM units
    uint32_t power_total;
    // Difference between received and expected sequence numbers
    uint32_t delta_sequence;
    // Ordering number for this controller
    uint32_t controller_ordinal;
    // Group number for this controller
    uint32_t group_ordinal;
    // Configured artnet starting point for this controller
    uint16_t artnet_universe;
    uint16_t artnet_channel;
    uint16_t my_port;
    // Flags for each strip, up to 8 strips
    uint8_t strip_flags[8];
    uint32_t pusher_flags;
    uint32_t segments;
};

struct __attribute__((__packed__)) pp_discovery_packet {
    struct discovery_packet_header header;
    struct pixel_pusher_info info;
};

struct pp_device {
    struct output_device base;

    int width;
    int height;

    int strip_num;
};

int output_pp_init();
void output_pp_term();

int output_pp_do_frame();
