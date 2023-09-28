#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

const size_t k_max_msg = 4096;

// Function to read exactly 'n' bytes from a file descriptor 'fd'
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Function to write exactly 'n' bytes to a file descriptor 'fd'
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Function to handle one client request
static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");  // End of file
        } else {
            msg("read() error");  // Read error
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // Assuming little endian
    if (len > k_max_msg) {
        msg("too long");  // Request too long
        return -1;
    }

    // Request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");  // Read error
        return err;
    }

    // Handle the request
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // Reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    // Create a socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Set socket options to allow reusing the address
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind the socket to a specific address and port
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;            // IPv4
    addr.sin_port = ntohs(1234);         // Port number (in network byte order)
    addr.sin_addr.s_addr = ntohl(0);    // Wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // Listen for incoming connections
    rv = listen(fd, SOMAXCONN); // SOMAXCONN is the maximum queue length for pending connections
    if (rv) {
        die("listen()");
    }

    while (1) {
        // Accept an incoming connection
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // Error, continue to accept other connections
        }

        while (1) {
            // Handle requests from the connected client
            int32_t err = one_request(connfd);
            if (err) {
                break; // End of requests or error, close the connection
            }
        }
        close(connfd);
    }

    return 0;
}
