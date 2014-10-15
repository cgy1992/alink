cmake_minimum_required( VERSION 2.8 )
include( CheckFunctionExists )

project( alink )
set( alink_SRCS
	alink.c
	args.c
	coff.c
	cofflib.c
	combine.c
	map.c
	mergerec.c
	message.c
	objload.c
	omflib.c
	op_bin.c
	op_exe.c
	op_pe.c
	relocs.c
	res.c
	segments.c
	symbols.c
	util.c
)
add_executable( alink ${alink_SRCS} )
set_source_files_properties( ${alink_SRCS} PROPERTIES LANGUAGE C )

check_function_exists( stricmp GOT_STRICMP )
check_function_exists( strcmpi GOT_STRCMPI )
check_function_exists( strcasecmp GOT_STRCASECMP )
check_function_exists( strupr GOT_STRUPR )
check_function_exists( strdup GOT_STRDUP )
check_function_exists( _strdup GOT__STRDUP )
check_function_exists( snprintf GOT_SNPRINTF )
check_function_exists( _snprintf GOT__SNPRINTF )
check_function_exists( vsnprintf GOT_VSNPRINTF )

if(UNIX)
	add_definitions( -DGOT_CASE_SENSITIVE_FILENAMES )
endif()

if(MSVC)
	add_definitions( -D_CRT_SECURE_NO_WARNINGS /wd4996 )
endif()

configure_file(
	${PROJECT_SOURCE_DIR}/cmake/alink_config.h.cmake
	${PROJECT_BINARY_DIR}/alink_config.h
)
include_directories( "${PROJECT_BINARY_DIR}" )

install( PROGRAMS ${PROJECT_BINARY_DIR}/alink DESTINATION bin )
