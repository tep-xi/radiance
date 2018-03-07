#pragma once

#include <stdint.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "output/config.h"
#include "output/slice.h"
#include "util/err.h"

// https://github.com/vishnubob/kinet
struct knt_device {
    struct output_device base;

    int width;
    int height;

    // state for sending data packets
    int out_fd;
    struct sockaddr_in out_addr;
};

int output_knt_init();
void output_knt_term();

int output_knt_do_frame();
