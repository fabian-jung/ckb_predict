cmake_minimum_required(VERSION 3.0)

project(ckb_predict)

FILE(GLOB_RECURSE src-files ${CMAKE_CURRENT_LIST_DIR}/src *.cpp)

find_package(Threads REQUIRED)

add_executable(ckb_predict main.cpp)
target_compile_features(ckb_predict PUBLIC cxx_std_17)
# target_sources(ckb_predict PRIVATE ${src-files})
target_include_directories(ckb_predict PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(ckb_predict Threads::Threads)

install(TARGETS ckb_predict RUNTIME DESTINATION bin)
