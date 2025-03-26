# -------------------------------- INCLUDE OCCT ----------------------------------
include(ExternalProject)

ExternalProject_Add(occt
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/occt
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${OpenCASCADE_INSTALL_DIR}
        -DBUILD_MODULE_Draw=OFF
        -DBUILD_DOC_Overview=OFF
        -DBUILD_LIBRARY_TYPE=STATIC
        -DCMAKE_BUILD_TYPE=Release
        -DINSTALL_DIR=${OpenCASCADE_INSTALL_DIR}
        BUILD_ALWAYS OFF
        UPDATE_COMMAND ""
)

include_directories(${OpenCASCADE_INSTALL_DIR}/include/opencascade)
link_directories(${OpenCASCADE_INSTALL_DIR}/lib)
# --------------------------------------------------------------------------------