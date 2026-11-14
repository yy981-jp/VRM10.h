cmake_minimum_required(VERSION 3.15)

project(VRM10 VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(nlohmann_json 3.10 REQUIRED)

add_library(vrm10 INTERFACE)
add_library(VRM10::VRM10 ALIAS vrm10)
set_target_properties(vrm10 PROPERTIES EXPORT_NAME VRM10)

target_compile_features(vrm10 INTERFACE cxx_std_17)
target_include_directories(vrm10 INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(vrm10 INTERFACE nlohmann_json::nlohmann_json)

option(VRM10_BUILD_TESTS "Build VRM10 tests" OFF)
if(VRM10_BUILD_TESTS)
	enable_testing()
	add_executable(vrm10_tests
		tests/test_vrm_parser.cpp
	)
	target_link_libraries(vrm10_tests PRIVATE VRM10::VRM10)
	add_test(NAME vrm10_tests COMMAND vrm10_tests)
endif()

install(TARGETS vrm10 EXPORT VRM10Targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT VRM10Targets
	FILE VRM10Targets.cmake
	NAMESPACE VRM10::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VRM10
)

configure_package_config_file(
	cmake/VRM10Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/VRM10Config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VRM10
)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/VRM10ConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/VRM10Config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/VRM10ConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VRM10
)
