#pragma once
#include <cstdint>

#include "event.h"

namespace net {

struct net_client_t {

  virtual ~net_client_t() {}

  // connect to an external relay
  virtual bool connect(const uint8_t ip[4], const uint16_t port) = 0;

  // disconnect from external relay
  virtual bool disconnect() = 0;

  // is connected to relay
  virtual bool connected() const = 0;

  // non blocking event sent
  virtual bool send(net_event_t &event) = 0;

  // non blocking event receive
  virtual net_event_t *recv() = 0;

  // calculate interpolation factor between tick events
  virtual float tween() = 0;

  // create net_client_t instance
  static net_client_t *create();
};

} // namespace net
