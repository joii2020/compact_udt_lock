cmake_minimum_required(VERSION 3.12)


project(cudt_simulator C)

SET(CMAKE_C_STANDARD 11)
SET(CUDT_LOCK_DIR ${CMAKE_SOURCE_DIR}/../..)
SET(CUDT_LOCK_C_DIR ${CMAKE_SOURCE_DIR}/../../c)


add_definitions(-DCKB_SIMULATOR)
add_definitions(-D__SHARED_LIBRARY__)
add_definitions(-DCKB_DECLARATION_ONLY)
add_definitions(-DCKB_USE_SIM)
#add_definitions(-DCKB_STDLIB_NO_SYSCALL_IMPL)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CUDT_LOCK_C_DIR})
include_directories(${CUDT_LOCK_DIR}/deps/ckb-c-std-lib)
include_directories(${CUDT_LOCK_DIR}/deps/ckb-c-std-lib/simulator)
include_directories(${CUDT_LOCK_DIR}/deps/ckb-c-std-lib/molecule)
include_directories(${CUDT_LOCK_DIR}/deps/ckb-c-std-lib/libc)

add_executable(cudt_simulator
    ${CMAKE_SOURCE_DIR}/test_compact_udt_lock.c
    ${CUDT_LOCK_C_DIR}/compact_udt_lock.c
)
