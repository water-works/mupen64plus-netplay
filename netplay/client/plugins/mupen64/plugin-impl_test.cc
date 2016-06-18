#include "client/plugins/mupen64/plugin-impl.h"

#include <set>

#include "client/mocks.h"
#include "client/plugins/mupen64/mocks.h"
#include "client/plugins/mupen64/util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "m64p_plugin.h"

using std::set;

using testing::_;
using testing::AtMost;
using testing::Contains;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

class PluginImplTest : public testing::Test {
 protected:
  typedef StrictMock<MockEventStreamHandler<BUTTONS>>
      StrictMockEventStreamHandler;
  typedef StrictMock<MockNetplayClient<BUTTONS>> StrictMockClient;

  PluginImplTest()
      : default_local_ports_({PORT_1, PORT_4}),
        default_remote_ports_({PORT_2}) {}

  void Init(Port port_1_request, Port port_2_request, Port port_3_request,
            Port port_4_request, const set<int>& input_channels,
            bool enabled = true) {
    mock_config_handler_ = new MockConfigHandler();

    M64Config config;
    config.enabled = enabled;
    config.server_hostname = "server.hostname.com";
    config.server_port = 1234;
    config.console_id = kConsoleId;
    config.delay_frames = kDelayFrames;
    config.port_1_request = util::PortToM64RequestedInt(port_1_request);
    config.port_2_request = util::PortToM64RequestedInt(port_2_request);
    config.port_3_request = util::PortToM64RequestedInt(port_3_request);
    config.port_4_request = util::PortToM64RequestedInt(port_4_request);
    mock_config_handler_->ExpectConfig(config);

    mock_client_ = new StrictMockClient();
    plugin_impl_.reset(new PluginImpl(
        mock_config_handler_, &mock_cin_, &mock_cout_,
        std::unique_ptr<MockNetplayClient<BUTTONS>>(mock_client_)));

    controls_[0] = {0};
    controls_[1] = {0};
    controls_[2] = {0};
    controls_[3] = {0};
    for (int i = 0; i < 4; ++i) {
      if (input_channels.find(i) != input_channels.end()) {
        controls_[i].Present = 1;
      }
    }

    netplay_controllers_[0] = {0};
    netplay_controllers_[1] = {0};
    netplay_controllers_[2] = {0};
    netplay_controllers_[3] = {0};

    netplay_info_.Enabled = &netplay_enabled_;
    netplay_info_.Controls = controls_;
    netplay_info_.NetplayControls = netplay_controllers_;

    input_channels_ = input_channels;

    SetUpInputExectations();
  }

  void ExpectNetplayInfo(const set<Port>& local_ports,
                         const set<Port>& remote_ports) {
    // Local ports must be marked as such and be marked one-to-one to the
    // available channels.
    set<int> allocated_channels;
    for (Port local_port : local_ports) {
      SCOPED_TRACE("Local port: " + Port_Name(local_port));

      const NETPLAY_CONTROLLER* controller =
          &netplay_controllers_[util::PortToM64Port(local_port)];
      EXPECT_EQ(1, controller->Present);
      EXPECT_EQ(0, controller->Remote);
      EXPECT_EQ(kDelayFrames, controller->Delay);
      EXPECT_THAT(input_channels_, Contains(controller->Channel));

      allocated_channels.insert(controller->Channel);
    }
    EXPECT_THAT(allocated_channels, UnorderedElementsAreArray(input_channels_));

    // Remote ports must be marked as such.
    for (Port remote_port : remote_ports) {
      SCOPED_TRACE("Remote port: " + Port_Name(remote_port));

      const NETPLAY_CONTROLLER* controller =
          &netplay_controllers_[util::PortToM64Port(remote_port)];
      EXPECT_EQ(1, controller->Present);
      EXPECT_EQ(1, controller->Remote);
      EXPECT_EQ(0, controller->Delay);
      EXPECT_EQ(-1, controller->Channel);
    }

    // All other ports must be marked as absent.
    for (Port unallocated_port : {PORT_1, PORT_2, PORT_3, PORT_4}) {
      SCOPED_TRACE("Unallocated port: " + Port_Name(unallocated_port));

      if (local_ports.find(unallocated_port) != local_ports.end() ||
          remote_ports.find(unallocated_port) != remote_ports.end()) {
        continue;
      }

      const NETPLAY_CONTROLLER* controller =
          &netplay_controllers_[util::PortToM64Port(unallocated_port)];
      EXPECT_EQ(0, controller->Present);
      EXPECT_EQ(0, controller->Remote);
      EXPECT_EQ(-1, controller->Delay);
      EXPECT_EQ(-1, controller->Channel);
    }

    EXPECT_TRUE(netplay_enabled_);
  }

  void ExpectUnitializedNetplayInfo(bool enabled) {
    for (int i = 0; i < 4; ++i) {
      const NETPLAY_CONTROLLER* controller = &netplay_controllers_[i];
      EXPECT_EQ(0, controller->Present);
      EXPECT_EQ(0, controller->Remote);
      EXPECT_EQ(-1, controller->Delay);
      EXPECT_EQ(-1, controller->Channel);
    }

    EXPECT_EQ(enabled, netplay_enabled_);
  }

  // ---------------------------------------------------------------------------
  // Helpers for tests other than InitiateNetplay*

  void InitDefault() {
    Init(PORT_1,   // Port 1 request
         UNKNOWN,  // Port 2 request
         PORT_3,   // Port 3 request
         UNKNOWN,  // Port 4 request
         {0, 2});  // input_channels
  }

  // Initiate netplay with default values.
  void InitiateNetplayDefault() {
    EXPECT_CALL(
        *mock_client_,
        PlugControllers(kConsoleId, UnorderedElementsAre(PORT_1, PORT_3), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
        .WillOnce(
            Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

    void* tag = reinterpret_cast<void*>(999LL);
    EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
    EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_stream_handler_, local_ports())
        .WillOnce(Return(set<Port>(default_local_ports_)));
    EXPECT_CALL(*mock_stream_handler_, remote_ports())
        .WillOnce(Return(set<Port>(default_remote_ports_)));

    EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_1))
        .WillOnce(Return(kDelayFrames));
    EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_4))
        .WillOnce(Return(kDelayFrames));
    EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_2))
        .WillOnce(Return(0));

    ASSERT_TRUE(plugin_impl_->InitiateNetplay(&netplay_info_));

    ExpectNetplayInfo(default_local_ports_, default_remote_ports_);
  }

  void SetUpInputExectations() {
    // Do you want to create a console?
    mock_cin_ << "n" << std::endl;
    // Enter a console ID
    mock_cin_ << kConsoleId << std::endl;
  }

  static const int kDelayFrames;
  static const int kConsoleId;

  std::stringstream mock_cin_;
  std::stringstream mock_cout_;

  // Owned by plugin_impl_
  StrictMockClient* mock_client_;
  StrictMockEventStreamHandler* mock_stream_handler_;

  MockConfigHandler* mock_config_handler_;
  unique_ptr<PluginImpl> plugin_impl_;

  // mupen64plus-core data structures
  CONTROL controls_[4];
  NETPLAY_CONTROLLER netplay_controllers_[4];
  int netplay_enabled_;
  NETPLAY_INFO netplay_info_;

  set<int> input_channels_;

  // Default configuration.
  const set<Port> default_local_ports_, default_remote_ports_;
};

const int PluginImplTest::kDelayFrames = 3;
const int PluginImplTest::kConsoleId = 3;

// -----------------------------------------------------------------------------
// InitiateNetplay

TEST_F(PluginImplTest, InitiateNetplaySuccessSpecificPorts) {
  Init(PORT_1,   // Port 1 request
       UNKNOWN,  // Port 2 request
       PORT_3,   // Port 3 request
       UNKNOWN,  // Port 4 request
       {0, 2});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId, UnorderedElementsAre(PORT_1, PORT_3), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
      .WillOnce(
          Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

  const set<Port> local_ports({PORT_1, PORT_4});
  const set<Port> remote_ports({PORT_2});

  EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, local_ports())
      .WillOnce(Return(set<Port>(local_ports)));
  EXPECT_CALL(*mock_stream_handler_, remote_ports())
      .WillOnce(Return(set<Port>(remote_ports)));

  // Delay profile
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_1))
      .WillOnce(Return(kDelayFrames));
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_4))
      .WillOnce(Return(kDelayFrames));
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_2))
      .WillOnce(Return(0));

  ASSERT_TRUE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectNetplayInfo(local_ports, remote_ports);
}

TEST_F(PluginImplTest, InitiateNetplaySuccessAnyPorts) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 2});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId, UnorderedElementsAre(PORT_ANY, PORT_ANY), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
      .WillOnce(
          Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

  const set<Port> local_ports({PORT_1, PORT_4});
  const set<Port> remote_ports({PORT_2});

  EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, local_ports())
      .WillOnce(Return(set<Port>(local_ports)));
  EXPECT_CALL(*mock_stream_handler_, remote_ports())
      .WillOnce(Return(set<Port>(remote_ports)));

  // Delay profile
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_1))
      .WillOnce(Return(kDelayFrames));
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_4))
      .WillOnce(Return(kDelayFrames));
  EXPECT_CALL(*mock_stream_handler_, DelayFramesForPort(PORT_2))
      .WillOnce(Return(0));

  ASSERT_TRUE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectNetplayInfo(local_ports, remote_ports);
}

TEST_F(PluginImplTest, InitiateNetplayControllerWithRawData) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 2});

  controls_[0].RawData = 1;

  ASSERT_FALSE(plugin_impl_->InitiateNetplay(&netplay_info_));
  ExpectUnitializedNetplayInfo(true /* enabled */);
}

TEST_F(PluginImplTest, InitiateNetplayMoreLocalPortsThanInputChannels) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 2});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId, UnorderedElementsAre(PORT_ANY, PORT_ANY), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
      .WillOnce(
          Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

  const set<Port> local_ports({PORT_1, PORT_3, PORT_4});
  const set<Port> remote_ports({PORT_2});

  EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, local_ports())
      .Times(AtMost(1))
      .WillOnce(Return(set<Port>(local_ports)));
  EXPECT_CALL(*mock_stream_handler_, remote_ports())
      .Times(AtMost(1))
      .WillOnce(Return(set<Port>(remote_ports)));

  EXPECT_FALSE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectUnitializedNetplayInfo(true);
}

TEST_F(PluginImplTest, InitiateNetplayMoreInputChannelsThanLocalPorts) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       PORT_ANY,  // Port 4 request
       {0, 2, 3});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId,
                      UnorderedElementsAre(PORT_ANY, PORT_ANY, PORT_ANY), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
      .WillOnce(
          Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

  const set<Port> local_ports({PORT_1, PORT_4});
  const set<Port> remote_ports({PORT_2});

  void* tag = reinterpret_cast<void*>(999LL);
  EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, local_ports())
      .Times(AtMost(1))
      .WillOnce(Return(set<Port>(local_ports)));
  EXPECT_CALL(*mock_stream_handler_, remote_ports())
      .Times(AtMost(1))
      .WillOnce(Return(set<Port>(remote_ports)));

  EXPECT_FALSE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectUnitializedNetplayInfo(true);
}

TEST_F(PluginImplTest, InitiateNetplayPlugControllersFails) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 3});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId, UnorderedElementsAre(PORT_ANY, PORT_ANY), _))
      .WillOnce(Return(false));

  EXPECT_FALSE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectUnitializedNetplayInfo(true);
}

TEST_F(PluginImplTest, InitiateNetplayFailToWaitForGameStartFails) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 3});

  EXPECT_CALL(
      *mock_client_,
      PlugControllers(kConsoleId, UnorderedElementsAre(PORT_ANY, PORT_ANY), _))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_client_, MakeEventStreamHandlerRaw())
      .WillOnce(
          Return(mock_stream_handler_ = new StrictMockEventStreamHandler()));

  void* tag = reinterpret_cast<void*>(999LL);
  EXPECT_CALL(*mock_stream_handler_, ClientReady()).WillOnce(Return(true));
  EXPECT_CALL(*mock_stream_handler_, WaitForConsoleStart())
      .WillOnce(Return(false));

  EXPECT_FALSE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectUnitializedNetplayInfo(true);
}

TEST_F(PluginImplTest, InitiateNetplayNetplayDisabled) {
  Init(PORT_ANY,  // Port 1 request
       UNKNOWN,   // Port 2 request
       PORT_ANY,  // Port 3 request
       UNKNOWN,   // Port 4 request
       {0, 3},
       false);  // enabled

  EXPECT_TRUE(plugin_impl_->InitiateNetplay(&netplay_info_));

  ExpectUnitializedNetplayInfo(false);
}

// -----------------------------------------------------------------------------
// PutButtons

bool operator==(std::tuple<Port, int, BUTTONS> t1,
                std::tuple<Port, int, BUTTONS> t2) {
  const BUTTONS& b1 = std::get<2>(t1);
  const BUTTONS& b2 = std::get<2>(t2);

  return std::get<0>(t1) == std::get<0>(t2) &&
         std::get<1>(t1) == std::get<1>(t2) && b1.Value == b2.Value;
}

TEST_F(PluginImplTest, PutButtonsSuccess) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS b1 = {1};
  BUTTONS b2 = {2};
  BUTTONS b3 = {3};

  m64p_netplay_frame_update updates[4]{{.port = 0,  // PORT_1
                                        .frame = 10,
                                        .buttons = &b1},
                                       {.port = 2,  // PORT_3
                                        .frame = 10,
                                        .buttons = &b2},
                                       {.port = 3,  // PORT_4
                                        .frame = 10,
                                        .buttons = &b3}};

  EXPECT_CALL(*mock_stream_handler_,
              PutButtons(UnorderedElementsAre(std::make_tuple(PORT_1, 10, b1),
                                              std::make_tuple(PORT_3, 10, b2),
                                              std::make_tuple(PORT_4, 10, b3))))
      .WillOnce(Return(
          EventStreamHandlerInterface<BUTTONS>::PutButtonsStatus::SUCCESS));

  EXPECT_TRUE(plugin_impl_->PutButtons(updates, 3));
}

TEST_F(PluginImplTest, PutButtonsInvalidPort) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS b1 = {1};
  BUTTONS b2 = {2};
  BUTTONS b3 = {3};

  m64p_netplay_frame_update updates[4]{
      {.port = -1, .frame = 10, .buttons = &b1},
      {.port = 100, .frame = 10, .buttons = &b2},
      {.port = 3, /*valid*/
       .frame = 10,
       .buttons = &b3}};

  EXPECT_FALSE(plugin_impl_->PutButtons(updates, 3));
}

TEST_F(PluginImplTest, PutButtonsFailedToPutButtons) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS b1 = {1};
  BUTTONS b2 = {2};
  BUTTONS b3 = {3};

  m64p_netplay_frame_update updates[4]{{.port = 0,  // PORT_1
                                        .frame = 10,
                                        .buttons = &b1},
                                       {.port = 2,  // PORT_3
                                        .frame = 10,
                                        .buttons = &b2},
                                       {.port = 3,  // PORT_4
                                        .frame = 10,
                                        .buttons = &b3}};

  EXPECT_CALL(*mock_stream_handler_,
              PutButtons(UnorderedElementsAre(std::make_tuple(PORT_1, 10, b1),
                                              std::make_tuple(PORT_3, 10, b2),
                                              std::make_tuple(PORT_4, 10, b3))))
      .WillOnce(Return(EventStreamHandlerInterface<
                       BUTTONS>::PutButtonsStatus::FAILED_TO_ENCODE));

  EXPECT_FALSE(plugin_impl_->PutButtons(updates, 3));
}

// -----------------------------------------------------------------------------
// GetButtons

bool operator==(const BUTTONS& b1, const BUTTONS& b2) {
  return b1.Value == b2.Value;
}

TEST_F(PluginImplTest, GetButtonsSuccess) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS buttons = {0};
  m64p_netplay_frame_update update = {
      .port = 1, .frame = 11, .buttons = &buttons};

  BUTTONS expected_buttons = {.Value = 100};
  EXPECT_CALL(*mock_stream_handler_, GetButtons(PORT_2, 11, &buttons))
      .WillOnce(DoAll(
          SetArgPointee<2>(expected_buttons),
          Return(EventStreamHandler<BUTTONS>::GetButtonsStatus::SUCCESS)));

  ASSERT_TRUE(plugin_impl_->GetButtons(&update));

  EXPECT_EQ(expected_buttons, *update.buttons);
}

TEST_F(PluginImplTest, GetButtonsInvalidPort) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS buttons = {0};
  m64p_netplay_frame_update update = {
      .port = -1, .frame = 11, .buttons = &buttons};

  EXPECT_FALSE(plugin_impl_->GetButtons(&update));
}

TEST_F(PluginImplTest, GetButtonsStreamGetButtonsFails) {
  InitDefault();
  InitiateNetplayDefault();

  BUTTONS buttons = {0};
  m64p_netplay_frame_update update = {
      .port = 1, .frame = 11, .buttons = &buttons};

  BUTTONS expected_buttons = {.Value = 100};
  EXPECT_CALL(*mock_stream_handler_, GetButtons(PORT_2, 11, &buttons))
      .WillOnce(DoAll(
          SetArgPointee<2>(expected_buttons),
          Return(EventStreamHandler<BUTTONS>::GetButtonsStatus::FAILURE)));

  EXPECT_FALSE(plugin_impl_->GetButtons(&update));
}
