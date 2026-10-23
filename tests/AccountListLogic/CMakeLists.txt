# Unit tests for the account switcher's list logic. Needs only QtCore and QtTest, so it builds in
# seconds without vcpkg or the MEGA SDK.
cmake_minimum_required(VERSION 3.16)
project(AccountListLogicTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Test)

set(LOGIC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../src/MEGASync/control)

add_executable(tst_AccountListLogic
    tst_AccountListLogic.cpp
    ${LOGIC_DIR}/AccountListLogic.cpp
)
target_include_directories(tst_AccountListLogic PRIVATE ${LOGIC_DIR})
target_link_libraries(tst_AccountListLogic PRIVATE Qt5::Core Qt5::Test)

enable_testing()
add_test(NAME AccountListLogic COMMAND tst_AccountListLogic)
