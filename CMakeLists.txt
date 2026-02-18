cmake_minimum_required(VERSION 3.16)
project(achivementbot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

add_executable(achivementbot
    main.cpp
    Bot.cpp
    Database.cpp
    PartyManager.cpp
    BoardManager.cpp
    TaskManager.cpp
    StreakManager.cpp
)

target_include_directories(achivementbot PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(achivementbot PRIVATE SQLite::SQLite3 CURL::libcurl)
