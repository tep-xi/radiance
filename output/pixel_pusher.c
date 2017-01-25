#include "pixel_pusher.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "output/config.h"
#include "util/err.h"

// This file implements a PixelPusher output.  It is designed to discover only 1 PixelPusher,
// but to use multiple grids attached to that PixelPusher

static struct pp_discovery_packet * pp_info = NULL;

static struct pp_device * grid_devices = NULL;
static size_t n_grid_devices = 0;

// Reusable state for sending data packets
static int out_fd = -1;
static uint32_t seq_num = 0;
static struct sockaddr_in * out_addr = NULL;

static uint8_t * out_packet;

static struct timespec packet_interval = { .tv_sec = 0, .tv_nsec = 5000*1000};

static int pp_find_pusher() {
    INFO("Initializing PixelPusher");

    if (output_config.pixel_pusher.ip0) {
        pp_info = calloc(1, sizeof *pp_info);
        if (pp_info == NULL) MEMFAIL();

        pp_info->info.my_port = output_config.pixel_pusher.data_port;
        pp_info->header.ip_addr = (output_config.pixel_pusher.ip3 << 24) | (output_config.pixel_pusher.ip2 << 16) | (output_config.pixel_pusher.ip1 << 8) | output_config.pixel_pusher.ip0;

        struct in_addr ip_addr;
        ip_addr.s_addr = pp_info->header.ip_addr;
        INFO("Found PixelPusher at IP address %s", inet_ntoa(ip_addr));

        return 0;
    }

    // Try to open a socket
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        PERROR("Error opening PixelPusher discovery socket");
        return -1;
    }

    // Set its timeout
    struct timeval tv;
    tv.tv_sec = output_config.pixel_pusher.discovery_seconds;
    if (!tv.tv_sec) {
        ERROR("PixelPusher discovery seconds must be non-zero");
        close(server_fd);
        return -1;
    }

    tv.tv_usec = 0;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval)) < 0) {
        PERROR("Error setting PixelPusher discovery socket timeout");
        close(server_fd);
        return -1;
    }

    // Socket address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(output_config.pixel_pusher.port);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof server_addr) < 0) {
        PERROR("Error binding PixelPusher discovery socket");
        close(server_fd);
        return -1;
    }

    // Read
    if (pp_info) {
        free(pp_info);
    }
    pp_info = calloc(1, sizeof *pp_info);
    if (pp_info == NULL) MEMFAIL();

    size_t expected_bytes = sizeof *pp_info;
    int bytes_read = read(server_fd, pp_info, expected_bytes);
    if (bytes_read < (int) expected_bytes) {
        PERROR("Expected to read %zu bytes from PixelPusher but read %d bytes", expected_bytes, bytes_read);
        free(pp_info);
        pp_info = NULL;
        close(server_fd);
        return -1;
    }

    close(server_fd);

    struct in_addr ip_addr;
    ip_addr.s_addr = pp_info->header.ip_addr;
    INFO("Found PixelPusher at IP address %s", inet_ntoa(ip_addr));

    return 0;
}

static int pp_add_grids() {
    n_grid_devices = output_config.n_pixel_pusher_grids;
    grid_devices = calloc(n_grid_devices, sizeof *grid_devices);
    if (grid_devices == NULL) MEMFAIL();

    // This is mostly copied from lux.c:425
    for (size_t i = 0; i < n_grid_devices; i++) {
        struct pp_device* device = &grid_devices[i];
        memset(device, 0, sizeof *device);

        // Hook ourselves into the output_device_head list
        if (!output_config.pixel_pusher_grids[i].configured)
            continue;
        if (output_device_head != NULL)
            output_device_head->prev = &device->base;
        device->base.next = output_device_head;
        output_device_head = &device->base;
        device->base.prev = NULL;

        // General device configuration
        device->base.active = true;
        device->base.ui_name = output_config.pixel_pusher_grids[i].ui_name;
        device->strip_num = output_config.pixel_pusher_grids[i].strip_num;

        // Geometry and pixel arrangement
        // TODO: Do better argument checking
        // TODO: Read this from the device itself (via the PixelPusher struct in packet.py)
        device->width = output_config.pixel_pusher_grids[i].width;
        device->height = output_config.pixel_pusher_grids[i].height;
        device->base.pixels.length = device->width * device->height;
        device->base.vertex_head = output_config.pixel_pusher_grids[i].vertexlist;

        int res = output_device_arrange_grid(&device->base, device->width, device->height);
        if (res < 0) {
            ERROR("Unable to arrange pixels for PixelPusher grid %zu", i);
            device->base.active = false;
            return -1;
        }
    }

    return 0;
}

static int pp_init_out() {
    seq_num = 0;

    // Try to open a socket
    if (out_fd > 0) {
        close(out_fd);
    }
    out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (out_fd < 0) {
        PERROR("Error opening PixelPusher data socket");
        return -1;
    }

    // Target address
    // This is only called if pp_find_pusher finishes successfully, so
    // pp_info is set
    out_addr = calloc(1, sizeof *out_addr);
    out_addr->sin_family = AF_INET;
    out_addr->sin_addr.s_addr = pp_info->header.ip_addr;
    out_addr->sin_port = htons(pp_info->info.my_port);

    out_packet = calloc(1, 4 + 2*(1+3*240));
    if (out_packet == NULL) MEMFAIL();

    return 0;
}

int output_pp_init() {
    if (pp_find_pusher() < 0) {
        return -1;
    }

    if (pp_init_out() < 0) {
        return -1;
    }

    return pp_add_grids();
}

void output_pp_term() {
    INFO("Terminating PixelPusher");

    if (pp_info) {
        free(pp_info);
        pp_info = NULL;
    }

    // TODO: Free device list

    if (out_fd) {
        close(out_fd);
        out_fd = -1;
    }

    if (out_addr) {
        free(out_addr);
        out_addr = NULL;
    }

    if (out_packet) {
        free(out_packet);
        out_packet = NULL;
    }
}

int output_pp_do_frame() {
    seq_num++;

    ((uint32_t *) out_packet)[0] = seq_num;
    size_t out_packet_idx = 4;

    for (size_t i = 0; i < n_grid_devices; i++) {
        struct pp_device * device = &grid_devices[i];

        // Make sure it was successfully initialized
        if (!device->base.active) continue;

        // Copy the data to send into a buffer - sadly SDL_Color is rgba
        // so we can't just send a header and it using sendmsg
        out_packet[out_packet_idx++] = device->strip_num;

        // Snake!
        for (int j = 0; j < device->height; j++) {
            for (int k = 0; k < device->width; k++) {
                int idx = (j % 2 ? device->width - 1 - k: k)*(device->height) + j;

                SDL_Color color = device->base.pixels.colors[idx];
                double alpha = color.a / 255.0;
                out_packet[out_packet_idx++] = color.r * alpha;
                out_packet[out_packet_idx++] = color.g * alpha;
                out_packet[out_packet_idx++] = color.b * alpha;
            }
        }

        // Send it
        if (i % 2) {
            size_t sent_bytes = sendto(out_fd, out_packet, out_packet_idx, 0, (struct sockaddr *) out_addr, sizeof *out_addr);
            if (sent_bytes < out_packet_idx) {
                PERROR("Error sending PixelPusher data");
                return -1;
            }

            // The PixelPusher doesn't have a very large Ethernet buffer, and UDP
            // obviously won't resend stuff
            nanosleep(&packet_interval, NULL);

            out_packet_idx = 4;
        }
    }

    return 0;
}
