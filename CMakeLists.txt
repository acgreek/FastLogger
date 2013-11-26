cmake_minimum_required (VERSION 2.8.4)
project (FastLogger)
cmake_policy(SET CMP0003 NEW)

find_package(PkgConfig)
pkg_check_modules (EXTREMECUNIT ExtremeCUnit)

string(REPLACE ";" " " EXTREMECUNIT_LDFLAGS2 "${EXTREMECUNIT_LDFLAGS}" )
string(REPLACE ";" " " EXTREMECUNIT_CFLAGS2 "${EXTREMECUNIT_CFLAGS}" )

set(CMAKE_EXE_LINKER_FLAGS "${EXTREMECUNIT_LDFLAGS2} -pie")

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

find_package(PkgConfig)
set(CMAKE_C_FLAGS "-Wall -Wextra -ggdb3" )
add_library(fastlogger SHARED fastlogger.c file_appender.c)

IF(CYGWIN)
	set(CMAKE_C_FLAGS "-Wl,--export-all-symbols ${CMAKE_C_FLAGS}" )
	set(CMAKE_CXX_FLAGS "-Wl,--export-all-symbols ${CMAKE_CXX_FLAGS}" )
	set(CMAKE_CXX_LINK_FLAGS "-Wl,--export-all-symbols ${CMAKE_CXX_LINK_FLAG}" )

	set (PC_LIBRARY_FLAGS "-Wl,--export-all-symbols -Wl,--rpath,.,--rpath,dist,--rpath,${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/libfastlogger.dll.a -lpthread")
	set (PC_C_FLAGS " -pie -lfastlogger")
	set(CMAKE_EXE_LINKER_FLAGS "-pie ")
ELSE()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC " )
	set(CMAKE_EXE_LINKER_FLAGS "-pie -rdynamic")
	set (PC_LIBRARY_FLAGS "-pie -Wl,--rpath,.,--rpath,dist -lfastlogger -lpthread" )
	set (PC_C_FLAGS "-fPIC -pie -rdynamic" )
ENDIF(CYGWIN)


configure_file (
	"${PROJECT_SOURCE_DIR}/fastlogger.pc.in"
   "${PROJECT_BINARY_DIR}/fastlogger.pc"
)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")

add_executable(test fastlogger_tests.c ) 

target_link_libraries(test ${EXTREMECUNIT_LIBRARIES} pthread fastlogger)
set_target_properties(test PROPERTIES CMAKE_C_LINK_FLAGS "${EXTREMECUNUT_LDFLAGS2}")
set_target_properties(test PROPERTIES CMAKE_CXX_LINK_FLAGS "${EXTREMECUNUT_LDFLAGS2}")

add_custom_target(run ./test)
add_custom_target(runv ./test -v)
ADD_DEPENDENCIES(run test)
ADD_DEPENDENCIES(runv test)

install(TARGETS fastlogger DESTINATION "lib")
install(FILES fastlogger.h DESTINATION "include")
install(FILES fastloger.pc DESTINATION "lib/pkgconfig")

