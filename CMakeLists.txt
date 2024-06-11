cmake_minimum_required(VERSION 3.0)

project(FinanceFirst)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# Add the SQLite library
find_library(SQLITE3_LIBRARY sqlite3)

add_executable(FinanceFirst src/FinanceFirst.cpp)
target_link_libraries(FinanceFirst ${GTK3_LIBRARIES} ${SQLITE3_LIBRARY})
