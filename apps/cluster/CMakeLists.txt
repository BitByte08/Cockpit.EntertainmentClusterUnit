cmake_minimum_required(VERSION 3.16)

add_executable(cluster
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    mainwindow.ui
    models/ClusterModel.cpp
    models/ClusterModel.hpp
    widgets/GaugeWidget.cpp
    widgets/GaugeWidget.hpp
    UpdateManager.cpp
    UpdateManager.hpp
)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

target_include_directories(cluster
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cluster
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        can_interface
)
