cmake_minimum_required(VERSION 3.16)

add_executable(entertainment
    main.cpp
    EntertainmentWindow.cpp
    EntertainmentWindow.hpp
    TileMapWidget.cpp
    TileMapWidget.hpp
    models/EntertainmentModel.cpp
    models/EntertainmentModel.hpp
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

target_include_directories(entertainment
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(entertainment
    PRIVATE
        Qt6::Widgets
        can_interface
)
