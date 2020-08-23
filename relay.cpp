#include <cstdint>
#include <vector>

#include "net.h"
#include "relay.h"
#include "socket.h"
#include "thread.h"

namespace net {

struct net_relay_impl_t;

// container for new clients
struct net_client_info_t {
  socket_t socket;
  uint32_t uuid;
};

// net relay implementation
struct net_relay_impl_t : public net_relay_t, private thread_t {

  static const uint16_t default_port = 12345;

  net_relay_impl_t()
    : _next_uuid(0)
    , _tick_interval(0)
    , _old_ticks(0)
    , _listen_port(default_port)
    , _listen_sock(sock_invalid)
    , _can_accept(false)
  {
    set_tick_rate(20);
  }

  virtual bool activate(uint16_t port) override {
    // start listening
    _listen_port = port;
    if (!listen()) {
      return false;
    }
    // we can accept new clients
    _can_accept = true;
    // start this thread
    start();
    return true;
  }

  virtual bool set_tick_rate(uint32_t hz) override {
    _tick_interval = 1000 / hz;
    return true;
  }

  // deactivate this thread
  virtual bool deactivate() override {
    ((thread_t *)this)->stop();
    return false;
  }

  // start the listen socket
  bool listen() {
    if (!sock_active(_listen_sock)) {
      _listen_sock = sock_listen(_listen_port);
    }
    return sock_active(_listen_sock);
  }

  // broadcast an event to all clients
  void broadcast(const net_event_t &event, uint32_t size) {
    // broadcast this to all clients
    for (const auto &client : _clients) {
      if (sock_send(client.socket, event.data(), size) != size) {
        // XXX: handle this
      }
    }
  }

  void send_tick() {
    // create a new tick message
    net_event_t tick;
    tick.header.size = 0;
    tick.header.type = event_tick;
    tick.header.checksum = tick.checksum();
    // broadcast tick to all clients
    broadcast(tick, sizeof(net_event_t::header_t) + tick.header.size);
  }

  // thread function
  virtual void run() {
    // send out new tick messages when needed
//    while ((get_ticks() - _old_ticks) >= _tick_interval) {
//      _old_ticks += _tick_interval;
//      send_tick();
//    }
    // try to accept any new clients
    if (_can_accept) {
      if (!clients_accept()) {
        // XXX: handle this
      }
    }
    // poll all clients for incomming events
    net_event_t event;
    for (const auto &client : _clients) {
      socket_t in = client.socket;
      if (uint32_t size = sock_recv(in, event.data(), event.max_size)) {
        broadcast(event, size);
      }
    }
    // dont burn the cpu
    std::this_thread::yield();
  }

  // poll for new clients
  bool clients_accept() {
    while (sock_select(&_listen_sock, 1)) {
      socket_t client = sock_accept(_listen_sock);
      client_add(client);
    }
    return true;
  }

  // add a new client
  void client_add(socket_t client) {
    printf("new client connected!\n");
    assert(client != sock_invalid);
    _clients.push_back(net_client_info_t{client, ++_next_uuid});

    // XXX: send a hello message with their uuid
  }

protected:
  // the list of connected clients
  std::vector<net_client_info_t> _clients;

  // the next uuid to issue
  uint32_t _next_uuid;

  // tick rate tracking
  uint32_t _tick_interval;
  uint64_t _old_ticks;
  
  // socket listening for incomming connections
  uint16_t _listen_port;
  socket_t _listen_sock;

  // can accept new clients
  // goes false when the game begins
  bool _can_accept;
};

net_relay_t *net_relay_t::create() {
  return new net_relay_impl_t;
}

} // namespace net
