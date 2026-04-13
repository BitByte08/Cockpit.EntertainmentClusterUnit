cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# VERSION 파일에서 버전 읽기
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" APP_VERSION_RAW)
string(STRIP "${APP_VERSION_RAW}" APP_VERSION)

project(CarEntertainmentClusterUnit
    VERSION ${APP_VERSION}
    LANGUAGES C CXX
)

# 버전 전체에서 컴파일 타임 상수로 노출
add_compile_definitions(APP_VERSION="${APP_VERSION}")

option(BUILD_TESTS "유닛 테스트 빌드" OFF)

add_subdirectory(core)
add_subdirectory(apps)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
