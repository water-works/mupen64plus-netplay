#ifndef CLIENT_PLUGINS_MUPEN64_CONFIG_HANDLER_H
#define CLIENT_PLUGINS_MUPEN64_CONFIG_HANDLER_H

#include <memory>
#include <string>

#include "m64p_config.h"

#include "client/plugins/mupen64/osal_dynamiclib.h"

using std::string;
using std::unique_ptr;

// Pure virtual interface for use in mocking. See definition of child below for
// details.
class ConfigHandlerInterface {
 public:
  virtual bool GetString(const string& param_name, string* out) const = 0;
  virtual int GetInt(const string& param_name) const = 0;
  virtual bool GetBool(const string& param_name) const = 0;

  virtual bool SetDefaultString(const string& param_name, const string& value,
                                const string& doc) const = 0;
  virtual bool SetDefaultInt(const string& param_name, int value,
                             const string& doc) const = 0;
  virtual bool SetDefaultBool(const string& param_name, bool value,
                              const string& doc) const = 0;
};

// Mockable configuration wrapper around the m64p configuration code.
class ConfigHandler : public ConfigHandlerInterface {
 public:
  // Returns a config handler, or nullptr on error.
  static unique_ptr<ConfigHandler> MakeConfigHandler(
      const m64p_dynlib_handle& core_lib_handle, const string& section_name);

  bool GetString(const string& param_name, string* out) const override;
  int GetInt(const string& param_name) const override;
  bool GetBool(const string& param_name) const override;

  bool SetDefaultString(const string& param_name, const string& value,
                        const string& doc) const override;
  bool SetDefaultInt(const string& param_name, int value,
                     const string& doc) const override;
  bool SetDefaultBool(const string& param_name, bool value,
                      const string& doc) const override;

 private:
  ConfigHandler(m64p_handle config_handle_);

  m64p_handle config_handle_;
  ptr_ConfigGetParameter ConfigGetParameter = NULL;
  ptr_ConfigSetDefaultInt ConfigSetDefaultInt = NULL;
  ptr_ConfigSetDefaultBool ConfigSetDefaultBool = NULL;
  ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
  ptr_ConfigGetParamInt ConfigGetParamInt = NULL;
  ptr_ConfigGetParamBool ConfigGetParamBool = NULL;
};

// Stores the netplay configuration.
struct M64Config {
  // Returns a default-constructed configuration on config parse error.
  static M64Config FromConfigHandler(const ConfigHandlerInterface& handler);

  bool enabled = false;
  string server_hostname = "";
  int server_port = 9889;
  int console_id = -1;
  int delay_frames = 0;
  int port_1_request = -1;
  int port_2_request = -1;
  int port_3_request = -1;
  int port_4_request = -1;
};

#endif  // CLIENT_PLUGINS_MUPEN64_CONFIG_HANDLER_H
