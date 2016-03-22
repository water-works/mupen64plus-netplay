#ifndef INTEGRATION_TESTS_FIXTURE_H_
#define INTEGRATION_TESTS_FIXTURE_H_

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/netplayServiceProto.grpc.pb.h"
#include "jni.h"

using std::string;
using std::shared_ptr;
using std::thread;
using std::vector;

// Starts a netplay server VM.
class NetplayServerTestFixture {
 public:
  NetplayServerTestFixture(const vector<string>& jars) : jars_(jars) {}

  // Starts the server main process. Returns immediately, does not wait until 
  // the server exits.
  void StartVM();

  // Shut down the test server and wait until its thread terminates. CHECK-fails 
  // if any step in this process fails.
  void ShutDownTestServer();

  // Returns a stub pointed at the local server.
  shared_ptr<NetPlayServerService::Stub> MakeStub();

  // Send a ping request to the server and wait for the response. Returns false 
  // if all requests failed or timed out.
  bool WaitForPing(int requests, int request_timeout_millis);

 private:
  static const char kMainClass[];
  static const char kLocalhost[];

  void VMThread();

  // Parses the jar list in jars_ and emits a classpath argument string of the 
  // form -Djava.class.path=x.y.z
  string MakeClasspath();

  vector<string> jars_;
  thread vm_thread_;

  JavaVM* jvm_;
  JNIEnv* env_;
};

#endif  // INTEGRATION_TESTS_FIXTURE_H_
