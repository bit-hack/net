#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>

#include "net.h"
#include "relay.h"

int main(int argc, char **args) {
  using namespace net;

  const uint16_t default_port = 9999;

  net_relay_t *relay = net_relay_t::create();
  if (!relay) {
    return 1;
  }

  if (!relay->activate(default_port)) {
    // relay may already be running
  }

  net_client_t *client = net_client_t::create();
  if (!client) {
    return 1;
  }

  const uint8_t ip[] = {127, 0, 0, 1};
  if (!client->connect(ip, default_port)) {
    return 1;
  }

  return 0;
}
