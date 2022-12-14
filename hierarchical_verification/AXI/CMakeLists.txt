cmake_minimum_required(VERSION 3.8)

# ---------------------------------------------------------------------------- #
# PROJECT
# name version language
# ---------------------------------------------------------------------------- #
project(EMESHAXI VERSION 1.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------- #
# VARIABLE
# alias and configurations
# ---------------------------------------------------------------------------- #
set(EMESHAXI_TARGET ${PROJECT_NAME})

set(CMAKE_CXX_COMPILER "g++-9")
set(CMAKE_PREFIX_PATH /home/huaixil/local)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# ---------------------------------------------------------------------------- #
# External dependencies
# ---------------------------------------------------------------------------- #
##
## ilang
##
find_package(ilang REQUIRED)

##
## Python (required if CMake version < 3.12)
##
if(${CMAKE_VERSION} VERSION_LESS 3.12)

  if(NOT TARGET Python::Python)
    add_library(Python::Python IMPORTED INTERFACE)

    set_property(TARGET Python::Python 
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIRECTORIES}
    )

    set_property(TARGET Python::Python
      PROPERTY INTERFACE_LINK_LIBRARIES ${PYTHON_LIBRARIES}
    )
    
  endif()

endif()


# ---------------------------------------------------------------------------- #
# TARGET
# library
# ---------------------------------------------------------------------------- #
add_library(${EMESHAXI_TARGET}
  src/emesh_axi_master.cc
  src/emesh_axi_util.cc
  src/emesh_axi_slave.cc
  src/emesh_axi_top.cc
)

target_include_directories(${EMESHAXI_TARGET} PRIVATE include)

target_link_libraries(${EMESHAXI_TARGET} PUBLIC ilang::ilang)


# ---------------------------------------------------------------------------- #
# TARGET
# executable
# ---------------------------------------------------------------------------- #
add_executable(${EMESHAXI_TARGET}Exe
  app/main.cc
)

target_include_directories(${EMESHAXI_TARGET}Exe PRIVATE include)

target_link_libraries(${EMESHAXI_TARGET}Exe ${EMESHAXI_TARGET})

