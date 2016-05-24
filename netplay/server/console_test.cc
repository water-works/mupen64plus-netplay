#include "server/console.h"

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "server/test-mocks.h"

using testing::_;
using testing::UnorderedElementsAre;
using testing::Contains;

namespace server {

class ConsoleTest : public ::testing::Test {
 protected:
  static const int64_t kConsoleId = 101;
  static const char kRomFileMD5[];

  ConsoleTest()
      : console_(kConsoleId, std::string(kRomFileMD5)),
        client_ids_{-1, -1, -1, -1} {
    request_template_.set_console_id(kConsoleId);
    request_template_.set_rom_file_md5(kRomFileMD5);
  };

  // Takes the ports in the response and does the following:
  //  - Remove duplicates
  //  - Removes ports not representing an actual allocation (i.e. UNKNOWN, etc.)
  std::set<Port> GetAllocatedPorts(const PlugControllerResponsePB& response) {
    std::set<Port> ports;
    for (const auto p : response.port()) {
      if (p == Port::PORT_ANY || p == Port::PORT_1 || p == Port::PORT_2 ||
          p == Port::PORT_3 || p == Port::PORT_4) {
        ports.insert(static_cast<Port>(p));
      }
    }
    return ports;
  }

  // Helper method to initialize the console with a different client at each
  // port.
  bool PopulateConsole() {
    PlugControllerRequestPB request = request_template_;
    int i = 0;
    for (const Port p :
         {Port::PORT_1, Port::PORT_2, Port::PORT_3, Port::PORT_4}) {
      request.set_requested_port_1(p);

      PlugControllerResponsePB response;
      console_.RequestPortMapping(request, &response);
      if (response.status() != PlugControllerResponsePB::SUCCESS) {
        return false;
      }

      client_ids_[i] = response.client_id();
      ports_[i] = response.port(0);

      ++i;
    }

    return true;
  }

  bool PopulateConsoleAndRegisterStreams() {
    PopulateConsole();

    return console_.RegisterStream(client_ids_[0], &stream_1_) &&
           console_.RegisterStream(client_ids_[1], &stream_2_) &&
           console_.RegisterStream(client_ids_[2], &stream_3_) &&
           console_.RegisterStream(client_ids_[3], &stream_4_);
  }

  Console console_;
  PlugControllerRequestPB request_template_;
  MockWriter<IncomingEventPB> stream_1_, stream_2_, stream_3_, stream_4_;
  int64_t client_ids_[4];
  Port ports_[4];
};

const char ConsoleTest::kRomFileMD5[] = "rom file md5";

// -----------------------------------------------------------------------------
// RequestPortMapping

TEST_F(ConsoleTest, RequestPortMappingSuccessSpecificPorts) {
  PlugControllerRequestPB request = request_template_;
  request.set_requested_port_1(Port::PORT_1);
  request.set_requested_port_2(Port::PORT_2);
  // Skip requested_port_3
  request.set_requested_port_4(Port::PORT_3);

  PlugControllerResponsePB response;
  console_.RequestPortMapping(request, &response);
  EXPECT_EQ(kConsoleId, response.console_id());
  EXPECT_EQ(
      PlugControllerResponsePB::Status_Name(PlugControllerResponsePB::SUCCESS),
      PlugControllerResponsePB::Status_Name(response.status()));
  EXPECT_EQ(1, response.client_id());

  EXPECT_THAT(response.port(),
              UnorderedElementsAre(Port::PORT_1, Port::PORT_2, Port::PORT_3));
}

TEST_F(ConsoleTest, RequestPortMappingSuccessAnyPorts) {
  PlugControllerRequestPB request = request_template_;
  request.set_requested_port_1(Port::PORT_ANY);
  request.set_requested_port_2(Port::PORT_ANY);
  // Skip requested_port_3
  request.set_requested_port_4(Port::PORT_ANY);

  PlugControllerResponsePB response;
  console_.RequestPortMapping(request, &response);
  EXPECT_EQ(kConsoleId, response.console_id());
  EXPECT_EQ(
      PlugControllerResponsePB::Status_Name(PlugControllerResponsePB::SUCCESS),
      PlugControllerResponsePB::Status_Name(response.status()));
  EXPECT_EQ(1, response.client_id());

  EXPECT_EQ(3, response.port().size());
  EXPECT_EQ(3, GetAllocatedPorts(response).size());
}

TEST_F(ConsoleTest, RequestPortMappingSuccessMixed) {
  PlugControllerRequestPB request = request_template_;
  request.set_requested_port_1(Port::PORT_1);
  request.set_requested_port_2(Port::PORT_ANY);
  // Skip requested_port_3
  request.set_requested_port_4(Port::PORT_3);

  PlugControllerResponsePB response;
  console_.RequestPortMapping(request, &response);
  EXPECT_EQ(kConsoleId, response.console_id());
  EXPECT_EQ(
      PlugControllerResponsePB::Status_Name(PlugControllerResponsePB::SUCCESS),
      PlugControllerResponsePB::Status_Name(response.status()));
  EXPECT_EQ(1, response.client_id());

  EXPECT_THAT(response.port(), Contains(PORT_1));
  EXPECT_THAT(response.port(), Contains(PORT_3));
  EXPECT_EQ(3, response.port().size());
  EXPECT_EQ(3, GetAllocatedPorts(response).size());
}

TEST_F(ConsoleTest, RequestPortMappingMultipleRequestsSuccess) {
  // Request ports 1 and 2
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_1);
    request.set_requested_port_2(Port::PORT_2);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(response.status()));
    EXPECT_EQ(1, response.client_id());

    EXPECT_THAT(response.port(),
                UnorderedElementsAre(Port::PORT_1, Port::PORT_2));
  }

  // Request port 3 and any other available port (port 4)
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_3);
    request.set_requested_port_2(Port::PORT_ANY);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(response.status()));
    EXPECT_EQ(2, response.client_id());

    EXPECT_THAT(response.port(),
                UnorderedElementsAre(Port::PORT_3, Port::PORT_4));
  }
}

TEST_F(ConsoleTest, RequestPortMappingAllPortsOccupied) {
  // Request all available ports
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_1);
    request.set_requested_port_2(Port::PORT_ANY);
    request.set_requested_port_3(Port::PORT_4);
    request.set_requested_port_4(Port::PORT_ANY);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(response.status()));
    EXPECT_EQ(1, response.client_id());

    EXPECT_THAT(response.port(),
                UnorderedElementsAre(Port::PORT_1, Port::PORT_2, Port::PORT_3,
                                     Port::PORT_4));
  }

  // Fail to request additional ports
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_3);
    request.set_requested_port_2(Port::PORT_ANY);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::PORT_REQUEST_REJECTED),
              PlugControllerResponsePB::Status_Name(response.status()));
  }
}

TEST_F(ConsoleTest, RequestPortMappingClientIdPreserved) {
  // Request ports 1 and 2
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_1);
    request.set_requested_port_2(Port::PORT_2);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(response.status()));
    EXPECT_EQ(1, response.client_id());

    EXPECT_THAT(response.port(),
                UnorderedElementsAre(Port::PORT_1, Port::PORT_2));
  }

  // Request port 1 again and fail.
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_1);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::PORT_REQUEST_REJECTED),
              PlugControllerResponsePB::Status_Name(response.status()));
  }

  // Request port 3 and expect that the console ID was not incremented by the
  // failure to allocate the previous request.
  {
    PlugControllerRequestPB request = request_template_;
    request.set_requested_port_1(Port::PORT_3);

    PlugControllerResponsePB response;
    console_.RequestPortMapping(request, &response);
    EXPECT_EQ(kConsoleId, response.console_id());
    EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                  PlugControllerResponsePB::SUCCESS),
              PlugControllerResponsePB::Status_Name(response.status()));

    EXPECT_EQ(2, response.client_id());

    EXPECT_THAT(response.port(), UnorderedElementsAre(Port::PORT_3));
  }
}

TEST_F(ConsoleTest, RequestPortMappingNoRequests) {
  PlugControllerRequestPB request = request_template_;

  PlugControllerResponsePB response;
  console_.RequestPortMapping(request, &response);
  EXPECT_EQ(kConsoleId, response.console_id());
  EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                PlugControllerResponsePB::NO_PORTS_REQUESTED),
            PlugControllerResponsePB::Status_Name(response.status()));
}

TEST_F(ConsoleTest, RequestPortMappingMD5Mismatch) {
  PlugControllerRequestPB request = request_template_;
  request.set_rom_file_md5("invalid md5 hash");
  request.set_requested_port_1(Port::PORT_1);

  PlugControllerResponsePB response;
  console_.RequestPortMapping(request, &response);
  EXPECT_EQ(kConsoleId, response.console_id());
  EXPECT_EQ(PlugControllerResponsePB::Status_Name(
                PlugControllerResponsePB::ROM_MD5_MISMATCH),
            PlugControllerResponsePB::Status_Name(response.status()));
}

// -----------------------------------------------------------------------------
// RegisterStream

TEST_F(ConsoleTest, RegisterStreamSuccess) {
  ASSERT_TRUE(PopulateConsole());

  EXPECT_TRUE(console_.RegisterStream(client_ids_[0], &stream_1_));
  EXPECT_TRUE(console_.RegisterStream(client_ids_[1], &stream_2_));
  EXPECT_TRUE(console_.RegisterStream(client_ids_[2], &stream_3_));
  EXPECT_TRUE(console_.RegisterStream(client_ids_[3], &stream_4_));

  // Validate that the streams were actually assigned.
  for (const auto& it : console_.clients_) {
    const Console::Client& client = it.second;
    if (client.client_id == client_ids_[0]) {
      EXPECT_EQ(client.stream, &stream_1_);
    }
  }
}

TEST_F(ConsoleTest, RegisterStreamBadClientId) {
  ASSERT_TRUE(PopulateConsole());

  EXPECT_FALSE(console_.RegisterStream(999, &stream_1_));
}

// -----------------------------------------------------------------------------
// HandleEvent

MATCHER_P(ExactlyEqualsProto, incoming, "equals " + incoming.DebugString()) {
  return arg.DebugString() == incoming.DebugString();
}

TEST_F(ConsoleTest, HandleEventSuccess) {
  ASSERT_TRUE(PopulateConsoleAndRegisterStreams());

  // Console 1 sends key presses
  {
    OutgoingEventPB received;
    auto* key_press = received.add_key_press();
    key_press->set_console_id(kConsoleId);
    key_press->set_port(ports_[0]);
    key_press->set_frame_number(101);
    key_press->set_x_axis(201);

    IncomingEventPB event;
    (*event.add_key_press()) = *key_press;
    EXPECT_CALL(stream_2_, Write(ExactlyEqualsProto(event), _));
    EXPECT_CALL(stream_3_, Write(ExactlyEqualsProto(event), _));
    EXPECT_CALL(stream_4_, Write(ExactlyEqualsProto(event), _));

    EXPECT_TRUE(console_.HandleEvent(received));
  }

  // Console 4 sends key presses
  {
    OutgoingEventPB received;
    auto* key_press = received.add_key_press();
    key_press->set_console_id(kConsoleId);
    key_press->set_port(ports_[3]);
    key_press->set_frame_number(102);
    key_press->set_x_axis(202);

    IncomingEventPB event;
    (*event.add_key_press()) = *key_press;
    EXPECT_CALL(stream_1_, Write(ExactlyEqualsProto(event), _));
    EXPECT_CALL(stream_2_, Write(ExactlyEqualsProto(event), _));
    EXPECT_CALL(stream_3_, Write(ExactlyEqualsProto(event), _));

    EXPECT_TRUE(console_.HandleEvent(received));
  }
}

TEST_F(ConsoleTest, HandleEventConsoleIdMismatch) {
  ASSERT_TRUE(PopulateConsoleAndRegisterStreams());

  OutgoingEventPB received;
  received.add_key_press()->set_console_id(kConsoleId);
  received.add_key_press()->set_console_id(kConsoleId);
  received.add_key_press()->set_console_id(kConsoleId);
  received.add_key_press()->set_console_id(kConsoleId + 1);

  EXPECT_FALSE(console_.HandleEvent(received));
}

}  // namespace server
