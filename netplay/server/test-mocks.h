#include "gmock/gmock.h"

#include "grpc++/impl/codegen/sync_stream.h"

namespace server {

template <typename T>
class MockWriter : public grpc::WriterInterface<T> {
 public:
  MOCK_METHOD2_T(Write, bool(const T&, const grpc::WriteOptions& options));
};

}  // namespace server
