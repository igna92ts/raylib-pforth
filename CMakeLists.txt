cmake_minimum_required(VERSION 3.15)

project(forthray C)

# Disable building raylib examples
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/raylib)

# pforth dir variable
set(PFORTH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pforth)
set(PFORTH_CSRC ${PFORTH_DIR}/csrc)
file(STRINGS ${PFORTH_CSRC}/sources.cmake PFORTH_SOURCES)
list(TRANSFORM PFORTH_SOURCES PREPEND ${PFORTH_CSRC}/)

if(WIN32)
    set(PFORTH_PLATFORM
        ${PFORTH_CSRC}/stdio/pf_fileio_stdio.c
        ${PFORTH_CSRC}/win32_console/pf_io_win32_console.c)
else()
    set(PFORTH_PLATFORM
        ${PFORTH_CSRC}/posix/pf_io_posix.c
        ${PFORTH_CSRC}/stdio/pf_fileio_stdio.c)
endif()

add_executable(forthray
    src/rl_bindings.c
    ${PFORTH_CSRC}/pf_main.c
    ${PFORTH_SOURCES}
    ${PFORTH_PLATFORM}
)

target_include_directories(forthray PRIVATE ${PFORTH_CSRC})
target_compile_definitions(forthray PRIVATE PF_USER_CUSTOM PF_SUPPORT_FP PF_NO_GLOBAL_INIT)

if(NOT MSVC)
    target_compile_options(forthray PRIVATE
        -fsigned-char -fno-builtin -fno-unroll-loops)
endif()

if(WIN32)
    target_compile_definitions(forthray PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(forthray PRIVATE raylib)

if(UNIX)
    target_link_libraries(forthray PRIVATE m)
endif()

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/pforth.dic
    COMMAND $<TARGET_FILE:forthray> -i system.fth
    COMMAND ${CMAKE_COMMAND} -E rename pforth.dic ${CMAKE_BINARY_DIR}/pforth.dic
    WORKING_DIRECTORY ${PFORTH_DIR}/fth
    DEPENDS forthray
    COMMENT "Regenerating pforth.dic (dictionary)"
    VERBATIM
)

add_custom_target(dictionary ALL DEPENDS ${CMAKE_BINARY_DIR}/pforth.dic)
