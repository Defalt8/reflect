cmake_minimum_required( VERSION 3.4 )
project( Reflect VERSION 0.5.7 LANGUAGES CXX )


add_executable( reflect src/reflect.cpp )
target_include_directories( reflect PUBLIC dep/dspp/include include )
target_compile_definitions( reflect PUBLIC WORKING_DIR="${CMAKE_SOURCE_DIR}/" )
target_compile_options( reflect PUBLIC -Wno-attributes )

add_executable( main src/main.cpp )
target_include_directories( main PUBLIC dep/dspp/include include gen out/build/gen )
target_compile_definitions( main PUBLIC WORKING_DIR="${CMAKE_SOURCE_DIR}/" )
target_compile_options( main PUBLIC -Wno-attributes )