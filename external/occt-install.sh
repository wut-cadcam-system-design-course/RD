#!/bin/bash

set -e

OCCT_BUILD_DIR=occt-build
OCCT_INSTALL_DIR=occt-install
OCCT_BUILD_THREADS=$(($(nproc)/2))

mkdir -p $OCCT_BUILD_DIR
cmake -S ./occt/ \
  -DCMAKE_INSTALL_PREFIX=$OCCT_INSTALL_DIR \
  -DBUILD_MODULE_Draw=OFF \
  -DBUILD_DOC_Overview=OFF \
  -DBUILD_LIBRARY_TYPE=STATIC \
  -DCMAKE_BUILD_TYPE=Release \
  -DINSTALL_DIR=$OCCT_INSTALL_DIR \
  -B $OCCT_BUILD_DIR
cmake --build $OCCT_BUILD_DIR --parallel $OCCT_BUILD_THREADS
cmake --install $OCCT_BUILD_DIR