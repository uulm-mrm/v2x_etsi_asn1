Example
=======

This folder contains an example program that uses the v2x utility libaries. It can receive CAM, CPM, VAM, and MCM messages, and send CAM messages by itself.

Usage
=====

Make sure your system has the dependencies installed (see toplevel README) and aduulm_logger and aduulm_cmake_tools are downloaded in separate folders. You may need to adjust the paths to these two dependencies in the example CMakeLists.txt. Then, build as usual:
```bash
$ mkdir -p build; cd build; cmake ..; make -j$(nproc); cd ..
```
To run the example, you can use the provided Docker image to start an AMQP 1.0 broker (make sure that Docker is installed):
```bash
$ ./test_broker/start_broker.sh
$ ./build/v2x_example
```
