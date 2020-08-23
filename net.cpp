#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>

#include <array>
#include <cassert>

#include "net.h"
#include "socket.h"
#include "util.h"

namespace net {

struct net_socket_t {

  net_socket_t(socket_t socket = sock_invalid)
    : _socket(socket) {
  }

  bool send(net_event_t &event) {
    if (_socket == sock_invalid)
      return false;
    // set the checksum
    event.header.checksum = event.checksum();
    // calculate full event size
    const uint32_t size = sizeof(net_event_t::header_t) + event.header.size;
    // send this event
    return sock_send(_socket, event.raw(), size) == size;
  }

  net_event_t *recv() {
    if (_socket == sock_invalid)
      return nullptr;
    // create new event
    auto event = std::make_unique<net_event_t>();
    // read data from socket into event
    const uint32_t received = sock_recv(_socket, event->raw(), event->max_size);
    if (received == sock_recv_error || received == sock_recv_none)
      return nullptr;
    // make sure we read the full packet
    if (event->header.size > received)
      return nullptr;
    // check the checksum
    if (event->header.checksum != event->checksum())
      return nullptr;
    return event.release();
  }

protected:
  socket_t _socket;
};

struct net_client_impl_t : public net_client_t {

  net_client_impl_t()
    : _socket(INVALID_SOCKET) {
    _last_ticks = get_ticks();
    _interval = target_interval;
  }

  bool connect(const uint8_t ip[4], const uint16_t port) override {
    if (connected()) {
      return false;
    }
    _socket = sock_open(ip, port);
    _net_sock.reset(new net_socket_t(_socket));
    return connected();
  }

  bool disconnect() override {
    if (_socket != sock_invalid) {
      _net_sock.reset();
      closesocket(_socket);
      _socket = sock_invalid;
    }
    return false;
  }

  bool connected() const override {
    return _socket != sock_invalid && _net_sock;
  }

  bool send(net_event_t &event) override {
    return _net_sock->send(event);
  }

  net_event_t *recv() override {
    net_event_t *event = _net_sock->recv();
    // if we received a tick then update tween
    if (event->header.type == event_tick) {
      on_tick();
    }
    return event;
  }

  float tween() override {
    const uint64_t ticks = get_ticks();
    const uint64_t interval = ticks - _last_ticks;
    return float(interval) / float(_interval);
  }

protected:

  void on_tick() {
    const uint64_t new_ticks = get_ticks();
    assert(new_ticks >= _last_ticks);
    _interval = new_ticks - _last_ticks;
  }

  static const uint64_t max_interval = 1000 / 30;
  static const uint64_t target_interval = 1000 / 20;

  // connection between client and relay
  socket_t _socket;
  std::unique_ptr<net_socket_t> _net_sock;

  // the last time we received a tick message
  uint64_t _last_ticks;
  // the last interval between tick messages
  // XXX: filter this value
  uint64_t _interval;

}; // struct net_impl_t

net_client_t *net_client_t::create() {
  return new net_client_impl_t;
}

} // namespace net
