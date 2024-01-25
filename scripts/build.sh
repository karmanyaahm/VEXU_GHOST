#!/bin/bash

if [ "$1" == "-h" ];
then
    echo "Builds ROS and PROS Workspace."
    echo "Specify -r to skip PROS build"
    exit 0
fi

# Get processor architecture to determine if we should build simulator or not (not on robot hardware)
arch=$(uname -p)

# Assumes repository is in base directory
cd ~/VEXU_GHOST
echo "---Building Ghost ROS Packages---"

# Build ignores simulator packages on embedded devices
if [ "$arch" == 'x86_64' ];
then 
    colcon build --symlink-install --packages-skip --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

if [ "$arch" == 'aarch64' ];
then 
    colcon build --symlink-install --packages-skip ghost_sim --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

RETURN=$?
if [ $RETURN -ne 0 ]; then
    exit 1;
fi

if [ "$1" != "-r" ];
then
  cd ~/VEXU_GHOST
  echo
  bash scripts/pros_build.sh
fi