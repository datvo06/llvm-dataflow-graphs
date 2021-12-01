# LLVM requires CMake >= 3.4.3
cmake_minimum_required(VERSION 3.4.3)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(llvm-tutor)

# Gotcha 1: On Mac OS Clang default to C++ 98, LLVM is implemented in C++ 14
set(CMAKE_CXX_STANDARD 14 CACHE STRING "")


# STEP 1. Make sure that LLVMConfig.cmake _is_ on CMAKE's search path
set(LT_LLVM_INSTALL_DIR "/usr/lib/llvm-10" CACHE_PATH "LLVM installation directory")
set(LT_LLVM_CMAKE_CONFIG_DIR "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm" CACHE_PATH "LLVM installation directory")
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_CMAKE_CONFIG_DIR}")

include_directories(".")

# Step 2. Load LLVM config from ... LLVMConfig.cmake
find_package(LLVM 10.0.0 REQUIRED CONFIG)
find_package(nlohmann_json 3.2.0 REQUIRED)

# HelloWorld includes header files from LLVM
include_directories(${LLVM_INCLUDE_DIRS})

if (NOT LLVM_ENABLE_RTTI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Step 3. Define the plugin/pass/library
# Gotcha 2: You don't need to use add_llvm_library
add_library(dataflow SHARED flow/dataflow.cc utils/llvm_utils.cc)
add_executable(dataflow_exe main.cc)

# Got cha 3: By default, undefined symbols are not allowed in shared objects on Mac OS.
# This is expected through so change the behavior
target_link_libraries(dataflow nlohmann_json::nlohmann_json "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
target_link_libraries(dataflow_exe LLVMCore LLVMPasses LLVMIRReader LLVMSupport dataflow)
