# ------------------------------ INCLUDE OCCT ------------------------------
set(OpenCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/occt-install/lib/cmake/opencascade" PARENT_SCOPE)
# --------------------------------------------------------------------------

# ------------------------------ INCLUDE IMGUI ------------------------------
set(IMGUI_PATH "imgui")

file(GLOB IMGUI_SOURCES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
        ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_PATH}/misc/cpp/imgui_stdlib.cpp)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${IMGUI_PATH})
# -------------------------------------------------------------------------