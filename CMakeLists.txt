cmake_minimum_required( VERSION 3.4 )
project( Reflect VERSION 0.5.7 LANGUAGES CXX )


add_executable( reflect src/reflect.cpp )
target_include_directories( reflect PUBLIC dep/dspp/include include )
target_compile_definitions( reflect PUBLIC WORKING_DIR="${CMAKE_SOURCE_DIR}/" )
