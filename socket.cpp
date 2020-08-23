// windows
#if defined(_MSC_VER) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>

// linux
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <cassert>
#include <map>

#include "socket.h"

namespace {

struct sock_info_t {
  SOCKET _hsock;
};

// true if the host socket system has been initalized
bool gHostSockInit;
// next socket key
socket_t gSockIndex;
// map socket api type to host socket type
std::map<socket_t, sock_info_t> gSockMap;

// if windows os
#if defined(_MSC_VER) || defined(_WIN32)

// initalize winsock2 library
bool winsock_init() {
  if (!gHostSockInit) {
    WSADATA WSAData;
    if (0 == WSAStartup(MAKEWORD(2, 2), &WSAData)) {
      gHostSockInit = true;
      return true;
    }
  }
  return gHostSockInit;
}

// shutdown winsock2 library
bool winsock_shutdown() {
  if (gHostSockInit) {
    WSACleanup();
    gHostSockInit = false;
  }
  return true;
}
#else
// initalize winsock2 library dummy
bool winsock_init() { return true; }

// shutdown winsock2 library dummy
bool winsock_shutdown() { return false; }
#endif // defined(_MSC_VER)
} // namespace

#if 0
socket_t sock_open(const char *host, uint16_t port) {
  hostent *hostEnt = ::gethostbyname(host);
  if (hostEnt) {
    if (hostEnt->h_addrtype != AF_INET) {
      return sock_invalid;
    }
    for (uint32_t i = 0; hostEnt->h_addr_list[i]; ++i) {
      const char *addr = hostEnt->h_addr_list[i];
      // todo ...
      // socket_t sock = sock_open(nullptr, port);
      // if (sock != sock_invalid) {
      //     return sock;
      // }
    }
  }
  return sock_invalid;
}
#endif

socket_t sock_open(const uint8_t ip[4], uint16_t port) {
  // ensure sock library is opened
  if (!winsock_init()) {
    return sock_invalid;
  }
  // create a socket
  SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    return sock_invalid;
  }
  // set inet address
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&(addr.sin_addr.S_un.S_addr), ip, 4);
  addr.sin_port = htons(port);
  // connect to remote
  if (int err = ::connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
    (void)err;
    ::closesocket(sock);
    return sock_invalid;
  }
  // success
  ++gSockIndex;
  assert(gSockMap.find(gSockIndex) == gSockMap.end());
  gSockMap.insert(
      std::pair<socket_t, sock_info_t>(gSockIndex, sock_info_t{sock}));
  // return socket handle
  return gSockIndex;
}

bool sock_close(socket_t sock) {
  auto itt = gSockMap.find(sock);
  if (itt == gSockMap.end()) {
    return false;
  }
  auto &info = itt->second;
  if (info._hsock != INVALID_SOCKET) {
    ::closesocket(info._hsock);
  }
  gSockMap.erase(itt);
  return true;
}

uint32_t sock_send(socket_t sock, const void *src, uint32_t size) {
  auto itt = gSockMap.find(sock);
  if (itt == gSockMap.end()) {
    return 0;
  }
  auto &info = itt->second;
  const int ret = ::send(info._hsock, (const char *)src, size, 0);
  if (ret == SOCKET_ERROR) {
    sock_close(sock);
    return sock_send_error;
  }
  return ret;
}

uint32_t sock_recv(socket_t sock, void *dst, uint32_t max) {
  auto itt = gSockMap.find(sock);
  if (itt == gSockMap.end()) {
    return 0;
  }
  auto &info = itt->second;
  // check if we have soemthing to read
  fd_set readfds = {1, {info._hsock}};
  const timeval time = {0, 0};
  const int sret = ::select(0, &readfds, nullptr, nullptr, &time);
  if (sret != 1) {
    return (sret == 0) ? sock_recv_none : sock_recv_error;
  }
  // read from socket
  const int rret = ::recv(info._hsock, (char *)dst, max, 0);
  if (rret == 0) {
    // connection closed
    sock_close(sock);
    return sock_recv_none;
  }
  if (rret < 0) {
    // recv failed
    return sock_recv_error;
  }
  return rret;
}

bool sock_active(socket_t sock) {
  return gSockMap.find(sock) != gSockMap.end();
}

uint32_t sock_select(socket_t *sockets, size_t num) {
  fd_set readfds;
  memset(&readfds, 0, sizeof(fd_set));
  readfds.fd_count = num;
  for (size_t i = 0, j = 0; i < num; ++i) {
    auto itt = gSockMap.find(sockets[i]);
    if (itt != gSockMap.end()) {
      assert(itt->second._hsock != INVALID_SOCKET);
      readfds.fd_array[j++] = itt->second._hsock;
    }
  }
  const timeval time = {0, 0};
  const int ret = ::select(0, &readfds, nullptr, nullptr, &time);
  return ret >= 0 ? ret : 0;
}

// open a listen socket
socket_t sock_listen(uint16_t port) {
  // ensure sock library is opened
  if (!winsock_init()) {
    return sock_invalid;
  }
  // create a socket
  SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    return sock_invalid;
  }
  // set this socket to be non blocking
  u_long non_blocking = 1;
  ioctlsocket(sock, FIONBIO, &non_blocking);
  // set inet address
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  // bind listen socket to this addr
  if (int err = ::bind(sock, (const sockaddr *)&addr, sizeof(addr))) {
    (void)err;
    ::closesocket(sock);
    return sock_invalid;
  }
  // set the socket to listen for incomming connections
  if (int err = ::listen(sock, 8)) {
    (void)err;
    ::closesocket(sock);
    return sock_invalid;
  }
  // success
  ++gSockIndex;
  assert(gSockMap.find(gSockIndex) == gSockMap.end());
  gSockMap.insert(
      std::pair<socket_t, sock_info_t>(gSockIndex, sock_info_t{sock}));
  // return socket handle
  return gSockIndex;
}

// accept an incomming socket connection
socket_t sock_accept(socket_t sock) {
  auto itt = gSockMap.find(sock);
  if (itt == gSockMap.end()) {
    return sock_invalid;
  }
  // try to accept a connection
  sockaddr addr;
  int addr_len = sizeof(addr);
  SOCKET s = ::accept(itt->second._hsock, &addr, &addr_len);
  if (s == INVALID_SOCKET) {
    return sock_invalid;
  }
  // success
  ++gSockIndex;
  assert(gSockMap.find(gSockIndex) == gSockMap.end());
  gSockMap.insert(
      std::pair<socket_t, sock_info_t>(gSockIndex, sock_info_t{s}));
  // return socket handle
  return gSockIndex;
}
