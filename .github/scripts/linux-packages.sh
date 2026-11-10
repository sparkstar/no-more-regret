#!/bin/sh
# The packages a Linux build of the port needs on Ubuntu, and the ones named
# on the command line. SDL is built with the port (port/CMakeLists.txt) and
# loads the system's libraries at run time; it builds its backends only for
# what the system's headers are there for: the window on X11 and Wayland,
# the sound on ALSA, PulseAudio and PipeWire, the gamepads on udev.
set -e
sudo apt-get update
sudo apt-get install -y --no-install-recommends ninja-build pkg-config "$@" \
  libasound2-dev libpulse-dev libpipewire-0.3-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
  libxkbcommon-dev libwayland-dev libdecor-0-dev libdrm-dev libgbm-dev \
  libegl1-mesa-dev libgl1-mesa-dev libgles2-mesa-dev \
  libdbus-1-dev libibus-1.0-dev libudev-dev
