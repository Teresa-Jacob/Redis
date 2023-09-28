#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// Function to print a message to standard error
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to print an error message, including the error code, and abort the program
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Function to handle communication with a connected client
static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
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

        // Handle communication with the connected client
        do_something(connfd);

        // Close the connection with the client
        close(connfd);
    }

    return 0;
}
