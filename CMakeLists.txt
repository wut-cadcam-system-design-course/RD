cmake_minimum_required(VERSION 3.27)
project(RD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS     OFF)

add_subdirectory(external)

find_package(OpenCASCADE REQUIRED)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

list(APPEND CMAKE_PREFIX_PATH external/libtorch)
find_package(Torch REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        # OpenCASCADE specific
        TKOpenGl
        TKV3d
        TKBool
        TKG3d
        TKMesh
        TKRWMesh
        TKDEOBJ
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
        imgui
        ImGuiFileDialog
        ${TORCH_LIBRARIES})
