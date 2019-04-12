cmake_minimum_required(VERSION 3.13)
project(cubicHecuba)

#add_executable(cubicHecuba ../main.cpp)


set(CMAKE_CXX_STANDARD 11)
set(HECUBA_INSTALL /home/bscuser/.local)
LINK_DIRECTORIES(${HECUBA_INSTALL}/lib)
FIND_LIBRARY(POCO NAMES PocoFoundation PATHS ${HECUBA_INSTALL}/lib)
FIND_LIBRARY(CASS NAMES cassandra PATHS ${HECUBA_INSTALL}/lib)
FIND_LIBRARY(LIBUV NAMES uv PATHS ${HECUBA_INSTALL}/lib)
FIND_LIBRARY(MPI NAMES mpi PATHS $ENV{MPI_ROOT}/lib64)  # <-------------------
find_library(HF NAMES hfetch PATHS ${HECUBA_INSTALL}/lib)
include_directories($ENV{HOME}/hecuba_from_git/hecuba_core/src)
include_directories($ENV{HOME}/.local/include)
include_directories(${HECUBA_INSTALL}/..)
include_directories(${HECUBA_INSTALL}/include)
set(ALL_LIBS ${CASS} ${UV} ${POCO})
set(SOURCE_FILES main.cpp)
add_executable(cubicHecuba ${SOURCE_FILES})
find_package(MPI REQUIRED)
target_link_libraries(cubicHecuba ${ALL_LIBS} ${HF} ${MPI_CXX_LIBRARIES})



