cmake_minimum_required(VERSION 3.15)

project(VRM10PackageConsumer LANGUAGES CXX)

find_package(VRM10 0.1 CONFIG REQUIRED)

add_executable(vrm10_package_consumer
	main.cpp
)
target_link_libraries(vrm10_package_consumer PRIVATE VRM10::VRM10)

enable_testing()
add_test(NAME vrm10_package_consumer COMMAND vrm10_package_consumer)
