HeeksCNC
========

This file describes how to build and install HeeksCNC under Unix systems.

0. Requirements
---------------

To build HeeksCNC, you need to install these requirements (with develoment files)
  * HeeksCAD
  * OpenCASCADE or OCE (OpenCASCADE Community Edition)
  * wxWidgets 2.8

1. Preparation
--------------

Create a build directory (e.g. build/ in sources root directory):
  mkdir build
  cd build

2. Configure build
------------------

If you want a default prefix (/usr/local) and a "Release" type, simply run:
  cmake ..

If you want to change install prefix (e.g. /usr):
  cmake -DCMAKE_INSTALL_PREFIX=/usr ..

If you want to debug HeeksCNC and its install:
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/install ..

Important note: HeeksCNC's prefix should be the same as HeeksCAD to work correctly

3. Build
--------

After a successful CMake configuration, you can build using:
  make

If you want more output (ie. to debug):
  make VERBOSE=1

4. Install
----------

Using default or system-wide prefix:
  sudo make install

Please note that if you installed it in /usr/local, you may need to run:
  sudo ldconfig

If you choose a user-writable prefix, superuser privileges are not needed:
  make install

5. Run
------

HeeksCNC is used through HeeksCAD interface.
HeeksCNC requires additional python modules at runtime:
 - python module built by libarea
 - python module built by opencamlib

X. One-liner snippets
---------------------
Default:
  mkdir build && cd build && cmake .. && make

Debug:
  mkdir debug && cd debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/install .. && make && make install
  LD_LIBRARY_PATH=install/lib install/bin/heekscad
