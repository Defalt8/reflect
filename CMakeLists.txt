cmake_minimum_required( VERSION 3.4 )
project( Reflect VERSION 0.8.12 LANGUAGES CXX )

if( MSVC )
	set( CMAKE_CXX_FLAGS -wd5030 )
else()
	set( CMAKE_CXX_FLAGS -Wno-attributes )
endif()

add_executable( reflect src/reflect.cpp )
target_include_directories( reflect PUBLIC dep/dspp/include include )
target_compile_definitions( reflect PUBLIC WORKING_DIR="${CMAKE_SOURCE_DIR}/" )


set( REFLECT_TEST_HEADER_FILES 
	"src/vec3f"
	"src/dm/vec2i"
	"src/dm/sd/var"
)

add_custom_target( reflect_test 
	COMMAND reflect ${REFLECT_TEST_HEADER_FILES} 
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_executable( main src/main.cpp )
add_dependencies( main reflect_test )
target_include_directories( main PUBLIC dep/dspp/include include gen out/build/gen )
target_compile_definitions( main PUBLIC WORKING_DIR="${CMAKE_SOURCE_DIR}/" )