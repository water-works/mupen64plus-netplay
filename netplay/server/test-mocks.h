#include "gmock/gmock.h"

#include "grpc++/impl/codegen/sync_stream.h"

namespace server {

template <typename T>
class MockWriter : public grpc::WriterInterface<T> {
 public:
  MOCK_METHOD2_T(Write, bool(const T&, const grpc::WriteOptions& options));
};

class MockConsole {
 public:
  MOCK_METHOD2(RequestPortMapping,
               void(const PlugControllerRequestPB&, PlugControllerResponsePB*));
  MOCK_METHOD2(RegisterStream,
               bool(int64_t, grpc::WriterInterface<IncomingEventPB>* stream));
  MOCK_METHOD1(HandleEvent, bool(const OutgoingEventPB&));
};

class MockConsoleFactory {
 public:
  typedef MockConsole Console;
  static inline Console* MakeConsole(int64_t console_id,
                                     const std::string& rom_file_md5) {
    return new Console();
  }
};

}  // namespace server
