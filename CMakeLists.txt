# Copyright 2017 Hans Cronau
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
cmake_minimum_required (VERSION 3.8)
project (vtTileCreator)

# For linking to DevIL.
set(DEVIL_LIBRARY_DIR "C:/cpplibs/DevIL Windows SDK/lib/x64/Release" CACHE PATH "Path to the libraries to link from DevIL.")
set(DEVIL_INCLUDE_DIR "C:/cpplibs/DevIL Windows SDK/include"         CACHE PATH "Path to the headers to include from DevIL.")

# Default tile and border widths.
set(VT_SUBTEXTURE_BORDER_TEXELS_WIDE "4" CACHE STRING "The default subtexture border width in texels.")
set(VT_TILE_TEXELS_WIDE            "256" CACHE STRING "The default tile width (and height) in texels. Should be power-of-two.")
set(VT_TILE_BORDER_TEXELS_WIDE       "4" CACHE STRING "The default tile border width in texels.")
set(VT_ATLAS_TEXELS_WIDE          "8192" CACHE STRING "The default atlas width (and height) in texels. Should best be power-of-two IF atlas contains power-of-two subtextures.")

# Image format customisation.
set(VT_ATLAS_FORMAT               ".png" CACHE STRING "The image format to store atlases.")
set(VT_TILE_FORMAT                ".png" CACHE STRING "The image type to store tiles.")
# Configure a header file to pass some of the CMake settings to the source code.
configure_file (
  config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)
# Add the binary tree to the search path for include files so that we will find config.h.
include_directories(${PROJECT_BINARY_DIR})

# Include DevIL headers and link DevIL libraries.
include_directories(${DEVIL_INCLUDE_DIR})
# DevIL.lib
find_library(DevIL_DevIL DevIL ${DEVIL_LIBRARY_DIR})
set (LIBS ${LIBS} ${DevIL_DevIL})
# ILU.lib
find_library(DevIL_ILU ILU ${DEVIL_LIBRARY_DIR})
set (LIBS ${LIBS} ${DevIL_ILU})
# ILUT.lib
find_library(DevIL_ILUT ILUT ${DEVIL_LIBRARY_DIR})
set (LIBS ${LIBS} ${DevIL_ILUT})
# DevIL C++ wrapper
add_library(DevIL_wrapper STATIC DevIL/devil_cpp_wrapper.cpp DevIL/devil_cpp_wrapper.h)
set (LIBS ${LIBS} DevIL_wrapper)

# Include and link PugiXML.
add_library(PugiXML STATIC pugixml/pugixml.cpp pugixml/pugixml.hpp)
set (LIBS ${LIBS} PugiXML)

# Include and link Boost.Filesystem.
set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package( Boost 1.63.0 REQUIRED COMPONENTS filesystem program_options regex )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  set (LIBS ${LIBS} ${Boost_LIBRARIES})
endif()

# Include and link RectangleBinPack by Jukka Jylänki.
add_library(RectangleBinPack STATIC RectangleBinPack/RectangleBinPack.cpp RectangleBinPack/RectangleBinPack.h)
set (LIBS ${LIBS} RectangleBinPack)

# Link personal helper functions.
add_library(HelperFunctions STATIC helper_functions.cpp helper_functions.h)
set (LIBS ${LIBS} HelperFunctions)

# Add the main executable.
add_executable(vtTileCreator vt_tile_creator.cxx)
target_link_libraries ( vtTileCreator ${LIBS} )
