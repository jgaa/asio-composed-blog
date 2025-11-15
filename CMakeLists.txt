cmake_minimum_required(VERSION 3.20)


message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (GCC11_INCLUDE_DIR)
    include_directories(${GCC11_INCLUDE_DIR})
endif()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBoost_NO_WARN_NEW_VERSIONS=ON)

find_package(Boost 1.75 MODULE REQUIRED
    program_options
    date_time
    context
    coroutine
    chrono
    )

include_directories(${Boost_INCLUDE_DIR})

option(BOOST_ERROR_CODE_HEADER_ONLY "Work-around for another boost issue" ON)
if (BOOST_ERROR_CODE_HEADER_ONLY)
    add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY=1)
endif()


if (UNIX)
  find_package(Threads REQUIRED)
endif()

add_executable(echo-client
  echo-client.cpp
  )

set_property(TARGET echo-client PROPERTY CXX_STANDARD 20)

target_compile_options(echo-client PRIVATE
    -std=c++20 -fcoroutines
    )
target_link_libraries(echo-client PRIVATE
  ${Boost_LIBRARIES}
  stdc++fs
  ${CMAKE_THREAD_LIBS_INIT}
  )

add_executable(echo-server
  echo-server.cpp
  )

set_property(TARGET echo-server PROPERTY CXX_STANDARD 20)

target_compile_options(echo-server PRIVATE
    -std=c++20 -fcoroutines
    )
target_link_libraries(echo-server PRIVATE
  ${Boost_LIBRARIES}
  stdc++fs
  ${CMAKE_THREAD_LIBS_INIT}
  )
