#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "util/config.h"
#include "util/err.h"

int network_receive(void* framebuffer) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        ERROR("Error opening network data socket");
        return -1;
    }

    // Socket address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(config.network.port);

    // Bind the socket
    if (bind(fd, (struct sockaddr *) &server_addr, sizeof server_addr) < 0) {
        ERROR("Error binding network data socket");
        close(fd);
        return -1;
    }

    size_t expected_bytes = 2 * 4 * config.pattern.master_width * config.pattern.master_height;

    while(1) {
        int bytes_read = read(fd, framebuffer, expected_bytes);
        if (bytes_read < (int) expected_bytes) {
            ERROR("Expected to receive %zu bytes over the network but received %d", expected_bytes, bytes_read);
        } else {
            INFO("Received network packet!");
        }
    }

    return 0;
}
