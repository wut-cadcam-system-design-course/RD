cmake_minimum_required(VERSION 3.27)
project(RD)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

find_package(OpenCASCADE REQUIRED)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

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
        glfw
        imgui)
