#ifndef CLIENT_PLUGINS_MUPEN64_PLUGIN_IMPL_H_
#define CLIENT_PLUGINS_MUPEN64_PLUGIN_IMPL_H_

#include <memory>
#include <vector>
#include <set>

#include "client/client.h"
#include "client/event-stream-handler.h"
#include "client/plugins/mupen64/config-handler.h"
#include "m64p_plugin.h"

class PluginImpl {
 public:
  typedef NetplayClientInterface<BUTTONS> M64Client;

  static const char kPluginName[];

  PluginImpl(const M64Config& config, unique_ptr<M64Client> client)
      : configuration_(config), client_(std::move(client)) {}

  // Returns a reference to the internal configuration struct.
  const M64Config& configuration() const { return configuration_; }

  // ---------------------------------------------------------------------------
  // mupen64plus-core API method implementations

  // Request the ports specified in the configuration and update netplay_info 
  // accordingly.
  int InitiateNetplay(NETPLAY_INFO* netplay_info);

  // Places local buttons into the respective queue and transmits them over the 
  // network.
  int PutButtons(const m64p_netplay_frame_update *updates, int nupdates);

  // Fetches the buttons from the stream and places them into the update's 
  // *value field.
  int GetButtons(m64p_netplay_frame_update* update);

 private:
  typedef EventStreamHandlerInterface<BUTTONS> M64StreamHandler;

  bool PermuteNetplayControllers(const std::vector<Port>& requested_ports,
                                 NETPLAY_INFO* netplay_info);

  const M64Config configuration_;
  unique_ptr<M64Client> client_;
  unique_ptr<EventStreamHandlerInterface<BUTTONS>> stream_handler_;
};

#endif  // CLIENT_PLUGINS_MUPEN64_PLUGIN_IMPL_H_
