cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Test Widgets Network)

enable_testing()

# ClusterModel 테스트
qt_add_executable(cluster_model_test
    ClusterModelTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/cluster/models/ClusterModel.cpp
    ${CMAKE_SOURCE_DIR}/apps/cluster/models/ClusterModel.hpp
)

target_include_directories(cluster_model_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/cluster
        ${CMAKE_SOURCE_DIR}/core/can_interface/include
)

target_link_libraries(cluster_model_test
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        can_interface
)

add_test(
    NAME ClusterModelTest
    COMMAND cluster_model_test
)
