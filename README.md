[project title here]
====================

Note this repository is structured as a superbuild. All the dependencies of the 
primary project are included in this repository as git submodules and are built 
as part of the project build process. For the main [xxx] README, see the 
`netplay` directory.

Building the Project
--------------------

For ninety-nine percent of users, it is sufficient to first configure using 
CMake (`cmake .`) and then build using the build system of your choice.

See each submodule for documentation (or lack of documentation) concerning the 
build dependencies for each subproject. Note the following dependencies for this 
project:

* CMake version 3.2 and up
* A C++ compiler that supports C++11
* Java 7 and up, both JDK and JRE

Advanced Build Topics
---------------------

##Warning##

Strictly speaking, this project targets 32 and 64 bit x86 Unix platforms. Builds 
on other systems are supported on a best-effort basis.

##ARM Build Notes##

The `grpc-java` build does not seem to behave very well on ARM, complaining 
about an incompatible toolchain, even on systems where the compiler supports ARM 
output. As a workaround, you can try configuring to project to skip building 
that target and manually building it yourself.

As of this writing, the GRPC-Java `protoc` plugin consists of two `.cpp` files
and a relatively small number of linked libraries. You should be able to compile
it yourself by hand, although you should be careful that the output executable 
is in the same place as the build system would have placed it. Here is a sample 
set of invocations to guide you, targeting a Unix environment. Note the argument 
to the `-o` parameter, which denotes the expected output binary path:

    # Configure to skip building the grpc-java project
    cmake . -DSKIP_GRPC_JAVA:bool=yes

    # Build the project. This will fail while building netplay/ due to the
    # missing protoc plugin, but more importantly will build the required
    # libraries.
    make

    # Manually build the executable. Note the output binary path (-o) and the
    # link path (-L)
    g++ -std=c++11 grpc-java/compiler/src/java_plugin/cpp/java_*.cpp \
      -L./usr/lib -lgrpc -lgpr -lprotobuf -lprotoc \
      -o grpc-java/compiler/build/binaries/java_pluginExecutable/protoc-gen-grpc-java

    # Build the project again. This will succeed because the binary has been 
    # generated
    make
