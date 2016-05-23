#include "server/console.h"

#include "base/netplayServiceProto.pb.h"
#include "base/netplayServiceProto.grpc.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::UnorderedElementsAre;
using testing::Contains;

namespace server {

class ConsoleTest : public ::testing::Test {
 protected:
  static const int64_t kConsoleId = 101;
  static const char kRomFileMD5[];

  ConsoleTest() : console_(kConsoleId, std::string(kRomFileMD5)){
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

  Console console_;

  PlugControllerRequestPB request_template_;
};

const char ConsoleTest::kRomFileMD5[] = "rom file md5";

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

}  // namespace server
