#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.16...3.27)
project(B1)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# See the documentation for a guide on how to enable/disable specific components
#
find_package(Geant4 REQUIRED ui_all vis_all)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, use our local headers, and link it to the Geant4 libraries
#
add_executable(lunar_device exampleB1.cc ${sources} ${headers})
target_compile_features(lunar_device PRIVATE cxx_std_17)
target_compile_definitions(lunar_device PRIVATE LUNAR_PROJECT_ROOT="${PROJECT_SOURCE_DIR}")
target_include_directories(lunar_device PRIVATE include)
target_link_libraries(lunar_device PRIVATE ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy startup, baseline batch, and optional B1 diagnostic macros to the build directory.
#
set(LUNAR_DEVICE_SCRIPTS
  init_vis.mac
  run1.mac
  run2.mac
  run_lunar.mac
  vis.mac
  )

foreach(_script ${LUNAR_DEVICE_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()
