#pragma once
#include <cstdint>

namespace net {

struct net_relay_t {

  virtual bool activate(uint16_t port) = 0;
  virtual bool set_tick_rate(uint32_t hz) = 0;
  virtual bool deactivate() = 0;

  // create net_relay_t instance
  static net_relay_t *create();
};

} // namespace net
