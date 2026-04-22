set(CMAKE_AUTOMOC ON)

set(CAN_SOURCES
    include/CANInterface.hpp
    src/CANInterface.cpp
    include/StubCANInterface.hpp
    src/StubCANInterface.cpp
)

if(WIN32)
    list(APPEND CAN_SOURCES
        include/can_frame_stub.hpp
    )
else()
    list(APPEND CAN_SOURCES
        include/SocketCANInterface.hpp
        src/SocketCANInterface.cpp
    )
endif()

add_library(can_interface STATIC ${CAN_SOURCES})

find_package(Qt6 REQUIRED COMPONENTS Core)

target_link_libraries(can_interface
    PRIVATE
        Qt6::Core
)

target_include_directories(can_interface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(can_interface
    PUBLIC
        cxx_std_17
)
