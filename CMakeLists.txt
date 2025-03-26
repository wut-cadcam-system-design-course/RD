cmake_minimum_required(VERSION 3.27)
project(RD)

set(CMAKE_CXX_STANDARD 17)

set(OpenCASCADE_INSTALL_DIR ${CMAKE_BINARY_DIR}/occt-install)
set(OpenCASCADE_DIR "${OpenCASCADE_INSTALL_DIR}/lib/cmake/opencascade")
add_subdirectory(external)
find_package(OpenCASCADE REQUIRED)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

add_dependencies(${PROJECT_NAME} occt)

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
        # OpenCASCADE specific
        TKOpenGl
        TKV3d
        TKBool
        TKG3d
        # TKernel
        # TKService
        # TKMath
        # TKBRep
        # TKGeomBase
        # TKPrim
        # TKTopAlgo
        # TKShHealing
        #
        # Other
        glfw)
