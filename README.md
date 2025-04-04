# Rib Detector :D

This project is an application built with Open Cascade Technology (OCCT) for 3D model visualization and automatic detection of ribs (reinforcement features) in CAD geometry.  It provides a simple interface for loading, viewing, and analyzing 3D shapes.

## Building 

In order to build RD application several development tools are needed:

- **C++ compiler** (e.g. `g++`)
- **CMake**
- **Git**

 Make sure they are installed and reachable from console. OCCT is managed as a git submodule, but its dependencies 
 aren't (they might be in the future!). To initialize it run the following command inside git repository
 ```
 git submodule update --init --recursive
 ```
 Now let's try to install OCCT binaries. For lucky group of Linux users there is a bash script that does it for us.
 To run it, go to `external` directory and type in terminal
```
./occt-install.sh
```
However, before doing that make sure you have all necessary OCCT dependencies on your machine. To learn more about
those dependencies check out [OCCT official repository](https://github.com/Open-Cascade-SAS/OCCT/blob/master/dox/build/build_occt/building_occt.md), or just run the script and add them as you go. If the
installation completed successfully, you should see 2 new directories - `occt-build` and `occt-install`. The first one
can be safely deleted, although I recommend keeping it in case you ever need to do a rebuild. Note that the installation 
process will take some time so be patient. If you don't want to use the script you can build OCCT by yourself. Just make
sure that you adjust **OpenCASCADE_DIR** variable inside `external/CMakeLists.txt` appropriately.

Once OCCT is installed, you can build the main project by using **CMake** and **CMakeLists.txt** inside project root
directory.  

## Docker

If you wish to run the application inside a docker container, there is a **Dockerfile** inside project root directory 
you can use. Before running the container, docker has to be allowed to connect to your X server (to display image). To do
that run
```
xhost +local:docker
```
After that, run the image by typing
```
docker run \
    --rm \
    -it \
    -e DISPLAY=$DISPLAY \
    -e XAUTHORITY=$XAUTHORITY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $XAUTHORITY:$XAUTHORITY \
    -v $(pwd):/usr/work/rd \
    username/container
```
