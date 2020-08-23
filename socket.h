#pragma once
#include <cstdint>

// opaque socket handle.
typedef uint32_t socket_t;

// socket special return codes.
enum {
  sock_invalid = -1,
  sock_recv_error = -1,
  sock_send_error = -1,
  sock_recv_none = 0
};

// open a new socket connection.
// host being the inet host name, and port being the port number in host format.
socket_t sock_open(const char *host, uint16_t port);

// open a new socket connection.
// ip being the ipv4 address, and port being the port number in host format.
socket_t sock_open(const uint8_t ip[4], uint16_t port);

// open a listen socket
socket_t sock_listen(uint16_t port);

// accept an incomming socket connection
socket_t sock_accept(socket_t);

// close an active socket connection.
// return true if the socket was closed.
// return false if there was some error.
bool sock_close(socket_t);

// send bytes to the remote side of a socket connection.
// if successfull, returns the number of bytes sent.
// on error, returns sock_send_error.
uint32_t sock_send(socket_t, const void *, uint32_t size);

// receive bytes from an open socket connection.
// if successfull, returns the number of bytes received or sock_recv_none if
// there was nothing to read. on error, returns sock_recv error.
uint32_t sock_recv(socket_t, void *dst, uint32_t max);

// test if a socket connection is still active.
// return true if socket is still open and active otherwise false.
bool sock_active(socket_t);

// return true if one input socket is ready to read or accept
uint32_t sock_select(socket_t *sockets, size_t num);
