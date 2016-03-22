#include "integration_tests/netplay-server-test-fixture.h"

#include <chrono>
#include <iostream>
#include <sstream>

#include <string>

#include "glog/logging.h"
#include "base/netplayServiceProto.pb.h"
#include "grpc++/channel.h"
#include "grpc++/create_channel.h"
#include "grpc++/security/credentials.h"

// Choose the JNI version.
#if defined JNI_VERSION_1_8
static const auto NETPLAY_JNI_VERSION = JNI_VERSION_1_8;
#elif defined JNI_VERSION_1_6
static const auto NETPLAY_JNI_VERSION = JNI_VERSION_1_6;
#endif  // defined JNI_VERSION_1_8

using std::stringstream;

const char NetplayServerTestFixture::kMainClass[] = "netplayServer/ServerStart";
const char NetplayServerTestFixture::kLocalhost[] = "localhost:10001";

void NetplayServerTestFixture::StartVM() {
  vm_thread_ = thread(&NetplayServerTestFixture::VMThread, this);
}

void NetplayServerTestFixture::ShutDownTestServer() {
  // Create a handle to the server and tell it to shut down.
  ShutDownServerRequestPB req;
  std::shared_ptr<grpc::Channel> channel =
      grpc::CreateChannel(kLocalhost, grpc::InsecureChannelCredentials());
  std::shared_ptr<NetPlayServerService::StubInterface> stub =
      NetPlayServerService::NewStub(channel);

  grpc::ClientContext context;
  ShutDownServerResponsePB resp;
  auto status = stub->ShutDownServer(&context, req, &resp);
  if (!status.ok()) {
    LOG(FATAL) << "ShutDownServer RPC failed: " << status.error_message();
  }

  if (!resp.server_will_die()) {
    LOG(FATAL) << "Test server did not agree to die.";
  }

  vm_thread_.join();
}

void NetplayServerTestFixture::VMThread() {
  JavaVMInitArgs vm_args;
  JavaVMOption options[1];

  string classpath_str = MakeClasspath();
  std::unique_ptr<char> classpath(new char[classpath_str.size()]);
  memcpy(classpath.get(), classpath_str.c_str(), classpath_str.size() + 1);
  options[0].optionString = classpath.get();

  LOG(INFO) << "Starting Netplay server with arguments \'"
            << options[0].optionString << "\' and main class " << kMainClass;

  vm_args.version = NETPLAY_JNI_VERSION;
  vm_args.nOptions = 1;
  vm_args.options = options;
  vm_args.ignoreUnrecognized = false;

  int result = JNI_CreateJavaVM(&jvm_, (void**)&env_, &vm_args);
  if (result < 0) {
    LOG(ERROR) << "Failed to start Java VM";
    exit(1);
  }

  jclass main_class = env_->FindClass(kMainClass);
  if (!main_class) {
    LOG(ERROR) << "Failed to find main class '" << kMainClass
               << "'. One possible cause is some jars not being passed as "
                  "commandline arguments to this test.";
    exit(1);
  }

  jmethodID main_method =
      env_->GetStaticMethodID(main_class, "main", "([Ljava/lang/String;)V");
  if (!main_method) {
    LOG(ERROR) << "Failed to find main method in main class " << kMainClass;
    exit(1);
  }

  jobjectArray args =
      env_->NewObjectArray(2, env_->FindClass("java/lang/String"), NULL);
  env_->SetObjectArrayElement(args, 0, env_->NewStringUTF("--port=10001"));
  env_->SetObjectArrayElement(args, 1, env_->NewStringUTF("--testMode"));
  env_->CallStaticVoidMethod(main_class, main_method, args);

  jvm_->DestroyJavaVM();
}

shared_ptr<NetPlayServerService::Stub> NetplayServerTestFixture::MakeStub() {
  std::shared_ptr<grpc::Channel> channel =
      grpc::CreateChannel(kLocalhost, grpc::InsecureChannelCredentials());
  return NetPlayServerService::NewStub(channel);
}

bool NetplayServerTestFixture::WaitForPing(int requests,
                                           int request_timeout_millis) {
  PingPB ping_request;
  auto stub = MakeStub();
  for (int i = 0; i < requests; ++i) {
    PingPB ping_response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() +
                         std::chrono::milliseconds(request_timeout_millis));
    auto status = stub->Ping(&context, ping_request, &ping_response);
    if (status.ok()) {
      return true;
    }

    LOG(ERROR) << "Server ping wait attempt number " << i + 1
               << " failed with error message \"" << status.error_message()
               << "\"." << (i == requests - 1 ? "" : " Retrying.");
  }

  return false;
}

string NetplayServerTestFixture::MakeClasspath() {
  stringstream classpath;
  classpath << "-Djava.class.path=";
  for (int i = 0; i < jars_.size(); ++i) {
    classpath << jars_[i];
    if (i != jars_.size() - 1) {
      classpath << ':';
    }
  }
  return classpath.str();
}
