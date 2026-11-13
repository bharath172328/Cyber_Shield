cmake_minimum_required(VERSION 3.16)
project(CyberShield CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

# Third-party header libraries (downloaded at configure time)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(httplib json)

# Core library: everything except main() so tests can link it too
add_library(cs_core STATIC
  src/utils/Config.cpp
  src/utils/Crypto.cpp
  src/database/DatabaseManager.cpp
  src/logging/SecurityLogger.cpp
  src/authentication/AuthenticationManager.cpp
  src/security/PasswordAnalyzer.cpp
  src/security/URLAnalyzer.cpp
  src/security/PortScanner.cpp
  src/security/FileHasher.cpp
  src/security/FileAnalyzer.cpp
  src/security/FileIntegrityMonitor.cpp
  src/security/ThreatDetector.cpp
  src/api/HttpUtil.cpp
  src/api/AuthController.cpp
  src/api/SecurityController.cpp
  src/api/DashboardController.cpp
  src/api/ApiServer.cpp)
target_include_directories(cs_core PUBLIC include)
target_compile_definitions(cs_core PUBLIC
  CS_SCHEMA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/database/schema.sql"
  CS_FRONTEND_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../frontend/dist")
target_compile_options(cs_core PRIVATE -Wall -Wextra)
target_link_libraries(cs_core PUBLIC httplib::httplib nlohmann_json::nlohmann_json
  OpenSSL::Crypto SQLite::SQLite3 Threads::Threads)

add_executable(cybershield src/main.cpp)
target_link_libraries(cybershield PRIVATE cs_core)

enable_testing()
add_executable(cs_tests tests/test_main.cpp)
target_link_libraries(cs_tests PRIVATE cs_core)
add_test(NAME cyber_shield_unit_tests COMMAND cs_tests)
