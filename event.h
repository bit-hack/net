#pragma once
#include <cassert>
#include <array>
#include <cstdint>
#include <memory>

#include "util.h"

namespace net {

enum {
  // event is not valid
  event_invalid = -1,
  // set by the relay periodicaly to update clients in lockstep
  event_tick = 0,
  // client request for new uuid from relay
  event_uuid_request,
  // player wants to join the game
  event_player_join,
  // player is leaving the game
  event_player_leave,
  // player is ready to start game
  event_player_ready,
};

struct net_event_t {

  // max size of an event in bytes
  static const size_t max_size = 1024;

  struct header_t {
    uint32_t type;     // event type
    uint32_t size;     // size of event
    uint32_t checksum; // event payload checksum
  };
  header_t &header;

  net_event_t() : header(*(header_t *)_data.data()) {
    memset(&header, 0, sizeof(header_t));
  }

  // return pointer to start of event data (payload)
  template <typename type_t = void> type_t *data() {
    return (type_t *)(_data.data() + sizeof(header_t));
  }

  template <typename type_t = void> const type_t *data() const {
    return (const type_t *)(_data.data() + sizeof(header_t));
  }

  // return pointer to start of event
  void *raw() { return (void *)_data.data(); }

  const void *raw() const { return (const void *)_data.data(); }

  // return the computed checksum of the event payload
  uint32_t checksum() const {
    assert(header.size <= max_size);
    return ::adler32(_data.data() + sizeof(header_t), header.size);
  }

protected:
  std::array<uint8_t, max_size> _data;
};

} // namespace net
