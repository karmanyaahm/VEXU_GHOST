cmake_minimum_required(VERSION 3.8)
project(ghost_viz)

# Add Global CMake Config for Ghost Packages
set(Ghost_DIR "$ENV{VEXU_HOME}/cmake")
find_package(Ghost CONFIG REQUIRED)

# Adds all our package dependencies to one list
set(DEPENDENCIES
  ament_cmake
  )

# Iterates through the list and finds the packages!
foreach(pkg ${DEPENDENCIES})
  find_package(${pkg} REQUIRED)
endforeach()

# Tell any packages that depend on us what dependencies we need.
ament_export_dependencies(${DEPENDENCIES})

install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME})

# This generates the ROS2 package and should always be the last line.
ament_package()
