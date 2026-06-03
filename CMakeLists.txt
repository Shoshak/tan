cmake_minimum_required(VERSION 3.26)
project(tan VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Lua REQUIRED)
find_package(efsw REQUIRED)

add_executable(main main.cpp)
target_include_directories(main PRIVATE ${LUA_INCLUDE_DIR} efsw)

target_link_libraries(main PRIVATE ${LUA_LIBRARIES} efsw::efsw)
