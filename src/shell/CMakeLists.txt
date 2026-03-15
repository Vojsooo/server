cmake_minimum_required (VERSION 3.28)
project (shell)

join_list("${CASPARCG_MODULE_INCLUDE_STATEMENTS}" "\n" CASPARCG_MODULE_INCLUDE_STATEMENTS)
join_list("${CASPARCG_MODULE_INIT_STATEMENTS}" "\n" CASPARCG_MODULE_INIT_STATEMENTS)
join_list("${CASPARCG_MODULE_UNINIT_STATEMENTS}" "\n" CASPARCG_MODULE_UNINIT_STATEMENTS)
join_list("${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}" "\n" CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS)
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/included_modules.tmpl" "${CMAKE_BINARY_DIR}/generated/included_modules.h")

set(SOURCES
		casparcg.config
		main.cpp
		server.cpp
)
set(HEADERS
		platform_specific.h
		server.h
)

if (MSVC)
	list(APPEND SOURCES
			resource.h
			shell.rc
			windows_specific.cpp
	)
else ()
	list(APPEND SOURCES
			linux_specific.cpp
	)
endif ()


add_executable(casparcg ${SOURCES} ${HEADERS})
set_target_properties(casparcg PROPERTIES
        OUTPUT_NAME "${CASPARCG_BINARY_NAME}"
    )
target_compile_features(casparcg PRIVATE cxx_std_17)
target_include_directories(casparcg PRIVATE
    ..
    ${BOOST_INCLUDE_PATH}
    )
casparcg_add_build_dependencies(casparcg)

source_group(sources ./*)

target_link_libraries(casparcg
		accelerator
		common
		core
		protocol

		"${CASPARCG_MODULE_TARGETS}"

		TBB::tbb
		TBB::tbbmalloc
		GLEW::glew
)

if (MSVC)
	target_link_libraries(casparcg
		Winmm.lib
		Ws2_32.lib
		OpenGL32.lib
		debug zlibstaticd.lib
		optimized zlibstatic.lib

		OpenAL::OpenAL
		optimized TBB::tbbmalloc_proxy

		avformat.lib
		avcodec.lib
		avutil.lib
		avfilter.lib
		avdevice.lib
		swscale.lib
		swresample.lib
	)
else ()
	target_link_libraries(casparcg
		${Boost_LIBRARIES}
		OpenGL::GL
		OpenGL::OpenGL
		OpenGL::EGL
		${X11_LIBRARIES}
		${FFMPEG_LIBRARIES}
		dl
		icui18n
		icuuc
		z
		pthread
	)
endif ()

if (MSVC)
set(CASPARCG_RUNTIME_COPY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_dependencies.cmake")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/copy_runtime_dependencies.cmake.in"
	"${CASPARCG_RUNTIME_COPY_SCRIPT}"
	@ONLY)

if (is_multi_config)
	set(CASPARCG_RUNTIME_COPY_CONFIGURATION "$<CONFIG>")
elseif (CMAKE_BUILD_TYPE)
	set(CASPARCG_RUNTIME_COPY_CONFIGURATION "${CMAKE_BUILD_TYPE}")
else ()
	set(CASPARCG_RUNTIME_COPY_CONFIGURATION "Release")
endif ()

function(casparcg_add_runtime_copy_command TARGET_NAME)
	add_custom_command(
		TARGET ${TARGET_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND}
			"-DCASPARCG_RUNTIME_CONFIGURATION=${CASPARCG_RUNTIME_COPY_CONFIGURATION}"
			"-DCASPARCG_RUNTIME_OUTPUT_DIR=$<TARGET_FILE_DIR:casparcg>"
			"-DCASPARCG_RUNTIME_MIRROR_DIR=${CMAKE_CURRENT_BINARY_DIR}"
			-P "${CASPARCG_RUNTIME_COPY_SCRIPT}"
		VERBATIM
	)
endfunction()

casparcg_add_runtime_copy_command(casparcg)

add_custom_target(casparcg_copy_dependencies
	COMMAND ${CMAKE_COMMAND}
		"-DCASPARCG_RUNTIME_CONFIGURATION=${CASPARCG_RUNTIME_COPY_CONFIGURATION}"
		"-DCASPARCG_RUNTIME_OUTPUT_DIR=$<TARGET_FILE_DIR:casparcg>"
		"-DCASPARCG_RUNTIME_MIRROR_DIR=${CMAKE_CURRENT_BINARY_DIR}"
		-P "${CASPARCG_RUNTIME_COPY_SCRIPT}"
	DEPENDS casparcg
	VERBATIM
)
else ()
	install(TARGETS casparcg)

    if ("${CASPARCG_BINARY_NAME}" STREQUAL "casparcg")
        # Looks like a 'default' build, produce a portable folder
	    install(FILES casparcg.config run.sh DESTINATION ".")
    else ()
        # Looks like a build for a system package manager
        install(FILES casparcg.config DESTINATION "share/${CASPARCG_BINARY_NAME}")
    endif ()
endif ()
