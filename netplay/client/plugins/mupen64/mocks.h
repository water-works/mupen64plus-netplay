#ifndef CLIENT_PLUGINS_MUPEN64_MOCKS_H_
#define CLIENT_PLUGINS_MUPEN64_MOCKS_H_

#include "client/plugins/mupen64/config-handler.h"
#include "client/plugins/mupen64/util.h"
#include "gmock/gmock.h"

class MockConfigHandler : public ConfigHandlerInterface {
 public:
  MOCK_CONST_METHOD2(GetString, bool(const string&, string*));
  MOCK_CONST_METHOD1(GetInt, int(const string&));
  MOCK_CONST_METHOD1(GetBool, bool(const string&));

  MOCK_METHOD2(SetInt, bool(const string&, int));

  MOCK_CONST_METHOD3(SetDefaultString,
                     bool(const string&, const string&, const string&));
  MOCK_CONST_METHOD3(SetDefaultInt, bool(const string&, int, const string&));
  MOCK_CONST_METHOD3(SetDefaultBool, bool(const string&, bool, const string&));

  void ExpectConfig(const M64Config& config) {
    EXPECT_CALL(*this, GetString("ServerHostname", testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly(
            testing::DoAll(testing::SetArgPointee<1>(config.server_hostname),
                           testing::Return(true)));
    EXPECT_CALL(*this, GetBool("Enabled"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.enabled));
    EXPECT_CALL(*this, GetInt("ServerPort"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.server_port));
    EXPECT_CALL(*this, GetInt("ConsoleId"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.console_id));
    EXPECT_CALL(*this, GetInt("DelayFrames"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.delay_frames));
    EXPECT_CALL(*this, GetInt("Port1Request"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.port_1_request));
    EXPECT_CALL(*this, GetInt("Port2Request"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.port_2_request));
    EXPECT_CALL(*this, GetInt("Port3Request"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.port_3_request));
    EXPECT_CALL(*this, GetInt("Port4Request"))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Return(config.port_4_request));
  }
};

#endif  // CLIENT_PLUGINS_MUPEN64_MOCKS_H_
