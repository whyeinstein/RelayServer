
# add_executable(mainparam main.cpp)
# target_link_libraries(mainparam mylib)

add_executable(loadgenerator loadGenerator.cpp)
aux_source_directory(. LG)
# list(REMOVE_ITEM LG "./main.cpp")
add_library(LGlib ${LG})
target_link_libraries(loadgenerator LGlib)
