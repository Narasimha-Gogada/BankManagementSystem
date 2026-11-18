cmake_minimum_required(VERSION 3.20)
project(bank_management_web LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz
)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(cpp_httplib nlohmann_json)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_executable(bank-web
    BankManagementSystem/WebServer.cpp
    BankManagementSystem/Account.cpp
    BankManagementSystem/Bank.cpp
    BankManagementSystem/Transaction.cpp
)

target_link_libraries(bank-web PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
    Threads::Threads
)

add_executable(bank-console
    BankManagementSystem/main.cpp
    BankManagementSystem/BankSystem.cpp
    BankManagementSystem/Account.cpp
    BankManagementSystem/Bank.cpp
    BankManagementSystem/Transaction.cpp
)

target_link_libraries(bank-console PRIVATE OpenSSL::Crypto)