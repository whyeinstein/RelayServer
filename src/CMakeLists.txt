
# gprof
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(main RelayServer.cpp)
aux_source_directory(. DIR_SRCS)
# list(REMOVE_ITEM DIR_SRCS "./main.cpp")
add_library(mylib ${DIR_SRCS})
target_link_libraries(main mylib)
