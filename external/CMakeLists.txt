# ------------------------------ INCLUDE OCCT ------------------------------
if(DEFINED ENV{OpenCASCADE_DIR})
    set(OpenCASCADE_DIR $ENV{OpenCASCADE_DIR} PARENT_SCOPE)
else ()
    set(OpenCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/occt-install/lib/cmake/opencascade" PARENT_SCOPE)
endif()

message("OCCT library located in: ${OpenCASCADE_DIR}")
# --------------------------------------------------------------------------