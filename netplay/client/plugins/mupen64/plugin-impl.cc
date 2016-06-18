#include "client/plugins/mupen64/plugin-impl.h"

#include <map>
#include <set>
#include <vector>

#include "base/netplayServiceProto.pb.h"
#include "client/host-utils.h"
#include "client/plugins/mupen64/util.h"
#include "glog/logging.h"

using std::map;
using std::vector;
using std::set;

const char PluginImpl::kPluginName[] = "NoNameNetplay";

// -----------------------------------------------------------------------------
// InitiateNetplay

int PluginImpl::InitiateNetplay(NETPLAY_INFO* netplay_info) {
  // Initalize the netplay structs to default values.
  for (int i = 0; i < 4; ++i) {
    NETPLAY_CONTROLLER* controller = &netplay_info->NetplayControls[i];
    controller->Present = 0;
    controller->Remote = 0;
    controller->Delay = -1;
    controller->Channel = -1;
  }

  M64Config configuration = M64Config::FromConfigHandler(*config_handler_);
  *netplay_info->Enabled = configuration.enabled;

  if (!configuration.enabled) {
    LOG(INFO) << "Netplay disabled, returning.";
    return 1;
  }

  // Disallow any controllers with RawData enabled.
  for (int i = 0; i < 4; ++i) {
    if (netplay_info->Controls[i].RawData) {
      LOG(ERROR) << "Input controller " << i
                 << " is in RawData mode. Netplay does not support RawData "
                    "mode and will now abort initialization.";
      return 0;
    }
  }

  int64_t new_console_id;
  bool created_new_console;
  if (!InteractiveConfig(&new_console_id, &created_new_console)) {
    // Error already logged
    return 0;
  }

  // Fill requested ports
  vector<Port> requested_ports;
  for (int encoded_port :
       {configuration.port_1_request, configuration.port_2_request,
        configuration.port_3_request, configuration.port_4_request}) {
    if (encoded_port == -1) {
      continue;
    }
    if (!Port_IsValid(encoded_port)) {
      LOG(ERROR) << "Invalid encoded port: " << encoded_port;
      return 0;
    }
    Port port = util::M64RequestedIntToPort(encoded_port);
    VLOG(3) << "Found requested port " << Port_Name(port);
    requested_ports.push_back(port);
  }

  // Request ports
  PlugControllerResponsePB::Status status;
  VLOG(3) << "Requesting found ports";
  if (!client_->PlugControllers(new_console_id, requested_ports, &status)) {
    LOG(ERROR) << "PlugControllers returned bad status: "
               << PlugControllerResponsePB::Status_Name(status);
    return 0;
  }

  // Start the stream and notify the server that we're ready to play.
  stream_handler_ = client_->MakeEventStreamHandler();
  VLOG(3)
      << "Indicating the netplay plugin is ready and waiting for console start";

  void* tag = stream_handler_->ClientReady();
  if (created_new_console && !InteractiveStartConsole(new_console_id)) {
    // Error already logged
    return 0;
  }

  cout_ << "Waiting for the console to start..." << std::endl;
  if (!stream_handler_->WaitForConsoleStart(tag)) {
    LOG(ERROR) << "Failed waiting for the game to start.";
    return 0;
  }

  if (!PermuteNetplayControllers(requested_ports, netplay_info)) {
    LOG(ERROR) << "Failed to permute game controllers.";
    return 0;
  }

  return 1;
}

// -----------------------------------------------------------------------------
// PutButtons

int PluginImpl::PutButtons(const m64p_netplay_frame_update* updates,
                           int nupdates) {
  vector<M64StreamHandler::ButtonsFrameTuple> button_frames_tuples;

  for (int i = 0; i < nupdates; ++i) {
    const Port port = util::M64PortToPort(updates[i].port);

    if (port == UNKNOWN) {
      LOG(ERROR) << "Attempted to put invalid port number " << updates[i].port;
      return false;
    }

    VLOG(3) << "Adding buttons for port " << Port_Name(port) << " and frame "
            << updates[i].frame;
    button_frames_tuples.push_back(
        std::make_tuple(port, updates[i].frame, *updates[i].buttons));
  }

  M64StreamHandler::PutButtonsStatus status =
      stream_handler_->PutButtons(button_frames_tuples);
  if (status != M64StreamHandler::PutButtonsStatus::SUCCESS) {
    LOG(ERROR) << "Failed to put buttons into the client.";
    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
// GetButtons

int PluginImpl::GetButtons(m64p_netplay_frame_update* update) {
  const Port port = util::M64PortToPort(update->port);
  if (port == UNKNOWN) {
    LOG(ERROR) << "Called GetButtons on invalid port " << update->port;
    return false;
  }

  VLOG(3) << "Requesting buttons for port " << Port_Name(port) << " and frame "
          << update->frame;

  M64StreamHandler::GetButtonsStatus status =
      stream_handler_->GetButtons(port, update->frame, update->buttons);
  if (status != M64StreamHandler::GetButtonsStatus::SUCCESS) {
    LOG(ERROR) << "Failed to get buttons for port " << Port_Name(port)
               << " and frame " << update->frame << " from stream";
    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
// Helper Methods

bool PluginImpl::PermuteNetplayControllers(const vector<Port>& requested_ports,
                                           NETPLAY_INFO* netplay_info) {
  // Extract the available input plugin channels.
  vector<int> available_channels;
  for (int i = 0; i < 4; ++i) {
    if (netplay_info->Controls[i].Present) {
      VLOG(3) << "Listing available channel " << i;
      available_channels.push_back(i);
    }
  }

  const set<Port> local_ports = stream_handler_->local_ports();
  if (available_channels.size() != local_ports.size()) {
    LOG(ERROR) << "Server returned a number of local ports ("
               << local_ports.size()
               << ") not equal to the number of input channels ("
               << available_channels.size() << ")";
    return false;
  }
  VLOG(3) << "Server returned " << local_ports.size() << " local port(s)";

  // Allocate local ports to input plugin channels. Put this in a block to scope
  // the iterators.
  {
    auto channel = available_channels.begin();
    auto port = local_ports.begin();
    for (; channel != available_channels.end() && port != local_ports.end();
         ++channel, ++port) {
      VLOG(3) << "Allocating available channel " << *channel
              << " to local port " << Port_Name(*port);
      NETPLAY_CONTROLLER* local_controller =
          &netplay_info->NetplayControls[util::PortToM64Port(*port)];
      local_controller->Present = 1;
      local_controller->Remote = 0;
      local_controller->Delay = stream_handler_->DelayFramesForPort(*port);
      local_controller->Channel = *channel;
    }
  }

  // Set up remote ports.
  for (const auto remote_port : stream_handler_->remote_ports()) {
    VLOG(3) << "Requesting remote port " << Port_Name(remote_port);
    NETPLAY_CONTROLLER* remote_controller =
        &netplay_info->NetplayControls[util::PortToM64Port(remote_port)];
    remote_controller->Present = 1;
    remote_controller->Remote = 1;
    remote_controller->Delay = stream_handler_->DelayFramesForPort(remote_port);
    remote_controller->Channel = -1;
  }

  return true;
}

namespace {

bool ReadBool(const string& prompt, std::istream& cin, std::ostream& cout,
              bool* result) {
  while (true) {
    string str;
    cout << std::endl << prompt;
    cin >> str;

    if (!cin.good()) {
      return false;
    }

    if (str == "y" || str == "yes") {
      *result = true;
      return true;
    } else if (str == "n" || str == "no") {
      *result = false;
      return true;
    }
  }
}

bool ReadInt64(const string& prompt, std::istream& cin, std::ostream& cout,
               int64_t* console_id) {
  while (true) {
    cout << std::endl << prompt;
    cin >> *console_id;
    return cin.good();
  }
}

}  // namespace

bool PluginImpl::InteractiveConfig(int64_t* console_id, bool* created_console) {
  using std::endl;

  *console_id = M64Config::FromConfigHandler(*config_handler_).console_id;
  *created_console = false;

  static const char kBreakLine[] =
      "==================================================================";

  cout_ << endl << endl << kBreakLine;
  if (!ReadBool("Do you want to create a new console? [y/n]: ", cin_, cout_,
                created_console)) {
    return false;
  }

  if (*created_console) {
    int console_id_int;
    MakeConsoleResponsePB::Status status = MakeConsoleResponsePB::UNKNOWN;
    if (!host_utils::MakeConsole("dummy-console", "dummy-rom", client_->stub(),
                                 &status, &console_id_int)) {
      LOG(ERROR) << "Failed to create a new console with status "
                 << MakeConsoleResponsePB::Status_Name(status);
      return false;
    }
    *console_id = console_id_int;

    cout_ << endl << endl << kBreakLine << endl;
    cout_ << "Created new console with ID: " << *console_id;
  } else {
    cout_ << endl << endl << kBreakLine;
    if (!ReadInt64("Enter new console ID: ", cin_, cout_, console_id)) {
      return false;
    }
  }

  return true;
}

bool PluginImpl::InteractiveStartConsole(int64_t console_id) {
  using std::endl;

  static const char kBreakLine[] =
      "==================================================================";

  cout_ << endl;
  cout_ << endl;
  cout_ << kBreakLine << endl;
  cout_ << "Press enter to start the console...";
  cin_.ignore(std::numeric_limits<std::streamsize>::max(),'\n');

  StartGameResponsePB::Status status = StartGameResponsePB::UNKNOWN;
  if (!host_utils::StartGame(console_id, client_->stub(), &status)) {
    LOG(ERROR) << "Failed to start console " << console_id
               << ". Saw status: " << StartGameResponsePB::Status_Name(status);
    return false;
  }

  return true;
}
