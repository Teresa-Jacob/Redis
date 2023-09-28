#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// Function to print an error message and terminate the program
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    // Create a socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Define a structure for the server address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;            // IPv4
    addr.sin_port = ntohs(1234);         // Port number (in network byte order)
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // IP address (127.0.0.1)

    // Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // Prepare a message to send to the server
    char msg[] = "hello";

    // Send the message to the server
    write(fd, msg, strlen(msg));

    // Create a buffer to receive the server's response
    char rbuf[64] = {};

    // Read the response from the server
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read");
    }

    // Print the server's response
    printf("server says: %s\n", rbuf);

    // Close the socket
    close(fd);

    return 0;
}
