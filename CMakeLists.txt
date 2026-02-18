cmake_minimum_required(VERSION 3.16)
project(achievementbot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

add_executable(achievementbot
    main.cpp
    Bot.cpp
    Database.cpp
    PartyManager.cpp
    BoardManager.cpp
    TaskManager.cpp
    StreakManager.cpp
)

target_include_directories(achievementbot PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(achievementbot PRIVATE SQLite::SQLite3 CURL::libcurl)
