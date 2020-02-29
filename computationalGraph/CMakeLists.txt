cmake_minimum_required(VERSION 3.0.0)
project(computationalGraph VERSION 0.1.0)

# link directory
execute_process (
  COMMAND /usr/local/opt/llvm/bin/llvm-config --libdir
  OUTPUT_VARIABLE LLVM_LIB_DIRECTORY
)
string(STRIP ${LLVM_LIB_DIRECTORY} LLVM_LIB_DIRECTORY)
MESSAGE(STATUS "LLVM_LIB_DIRECTORY=${LLVM_LIB_DIRECTORY}")
link_directories(BEFORE ${LLVM_LIB_DIRECTORY})

# set source files
file(GLOB MY_SOURCE_FILES *.hpp *.cpp)
add_executable(computationalGraph ${MY_SOURCE_FILES})

# compile options
execute_process (
  COMMAND /usr/local/opt/llvm/bin/llvm-config --cxxflags 
  OUTPUT_VARIABLE LLVM_CMAKE_CXX_FLAGS
)
string(STRIP ${LLVM_CMAKE_CXX_FLAGS} LLVM_CMAKE_CXX_FLAGS)
MESSAGE(STATUS "LLVM_CMAKE_CXX_FLAGS=${LLVM_CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CMAKE_CXX_FLAGS}")

# libraries for LLVM
execute_process (
  COMMAND /usr/local/opt/llvm/bin/llvm-config --libs core orcjit mcjit native
  OUTPUT_VARIABLE LLVM_LIBRARIES
)
string(REGEX REPLACE "-l" "" LLVM_LIBRARIES ${LLVM_LIBRARIES})
string(REGEX REPLACE " " ";" LLVM_LIBRARIES ${LLVM_LIBRARIES})
foreach(library IN LISTS LLVM_LIBRARIES)
  string(STRIP ${library} library)
  target_link_libraries(computationalGraph ${library})
endforeach()

# system libraries for LLVM
execute_process (
  COMMAND /usr/local/opt/llvm/bin/llvm-config --system-libs
  OUTPUT_VARIABLE LLVM_SYSTEM_LIBRARIES
)
string(REGEX REPLACE "-l" "" LLVM_SYSTEM_LIBRARIES ${LLVM_SYSTEM_LIBRARIES})
string(REGEX REPLACE " " ";" LLVM_SYSTEM_LIBRARIES ${LLVM_SYSTEM_LIBRARIES})
foreach(library IN LISTS LLVM_SYSTEM_LIBRARIES)
  string(STRIP ${library} library)
  target_link_libraries(computationalGraph ${library})
endforeach()
