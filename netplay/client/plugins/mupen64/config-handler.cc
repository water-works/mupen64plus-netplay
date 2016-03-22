#include "client/plugins/mupen64/config-handler.h"

ConfigHandler::ConfigHandler(m64p_handle config_handle)
    : config_handle_(config_handle) {}

bool ConfigHandler::GetString(const string& param_name, string* out) const {
  char out_char[4096];
  if (ConfigGetParameter(config_handle_, param_name.c_str(), M64TYPE_STRING,
                         &out_char, 4096) != M64ERR_SUCCESS) {
    return false;
  }
  *out = out_char;
  return true;
}

int ConfigHandler::GetInt(const string& param_name) const {
  return ConfigGetParamInt(config_handle_, param_name.c_str());
}

bool ConfigHandler::GetBool(const string& param_name) const {
  return ConfigGetParamBool(config_handle_, param_name.c_str());
}

bool ConfigHandler::SetDefaultString(const string& param_name,
                                     const string& value,
                                     const string& doc) const {
  return ConfigSetDefaultString(config_handle_, param_name.c_str(),
                                value.c_str(), doc.c_str()) == M64ERR_SUCCESS;
}

bool ConfigHandler::SetDefaultInt(const string& param_name, int value,
                                  const string& doc) const {
  return ConfigSetDefaultInt(config_handle_, param_name.c_str(), value,
                             doc.c_str()) == M64ERR_SUCCESS;
}

bool ConfigHandler::SetDefaultBool(const string& param_name, bool value,
                                   const string& doc) const {
  return ConfigSetDefaultBool(config_handle_, param_name.c_str(), value,
                              doc.c_str()) == M64ERR_SUCCESS;
}

unique_ptr<ConfigHandler> ConfigHandler::MakeConfigHandler(
    const m64p_dynlib_handle& core_lib_handle, const string& section_name) {
  m64p_handle handle;
  ptr_ConfigOpenSection ConfigOpenSection = (ptr_ConfigOpenSection)(
      osal_dynlib_getproc(core_lib_handle, "ConfigOpenSection"));

  if (ConfigOpenSection(section_name.c_str(), &handle) != M64ERR_SUCCESS) {
    return unique_ptr<ConfigHandler>();
  }

  unique_ptr<ConfigHandler> config_handler(new ConfigHandler(handle));

  config_handler->ConfigGetParameter =
      (ptr_ConfigGetParameter)osal_dynlib_getproc(core_lib_handle,
                                                  "ConfigGetParameter");
  config_handler->ConfigSetDefaultInt =
      (ptr_ConfigSetDefaultInt)osal_dynlib_getproc(core_lib_handle,
                                                   "ConfigSetDefaultInt");
  config_handler->ConfigSetDefaultBool =
      (ptr_ConfigSetDefaultBool)osal_dynlib_getproc(core_lib_handle,
                                                    "ConfigSetDefaultBool");
  config_handler->ConfigSetDefaultString =
      (ptr_ConfigSetDefaultString)osal_dynlib_getproc(core_lib_handle,
                                                      "ConfigSetDefaultString");
  config_handler->ConfigGetParamInt =
      (ptr_ConfigGetParamInt)osal_dynlib_getproc(core_lib_handle,
                                                 "ConfigGetParamInt");
  config_handler->ConfigGetParamBool =
      (ptr_ConfigGetParamBool)osal_dynlib_getproc(core_lib_handle,
                                                  "ConfigGetParamBool");

  if (!config_handler->ConfigGetParameter ||
      !config_handler->ConfigSetDefaultInt ||
      !config_handler->ConfigSetDefaultBool ||
      !config_handler->ConfigSetDefaultString ||
      !config_handler->ConfigGetParamInt ||
      !config_handler->ConfigGetParamBool) {
    return unique_ptr<ConfigHandler>();
  }

  return config_handler;
}

M64Config M64Config::FromConfigHandler(
    const ConfigHandlerInterface& config_handler) {
  M64Config config;
  config.enabled = config_handler.GetBool("Enabled");

  // ServerHostname
  if (!config_handler.GetString("ServerHostname", &config.server_hostname)) {
    return M64Config();
  }

  // ServerPort
  config.server_port = config_handler.GetInt("ServerPort");

  // ConsoleId
  config.console_id = config_handler.GetInt("ConsoleId");

  // DelayFrames
  config.delay_frames = config_handler.GetInt("DelayFrames");

  // Port*Request
  config.port_1_request = config_handler.GetInt("Port1Request");
  config.port_2_request = config_handler.GetInt("Port2Request");
  config.port_3_request = config_handler.GetInt("Port3Request");
  config.port_4_request = config_handler.GetInt("Port4Request");

  return config;
}
