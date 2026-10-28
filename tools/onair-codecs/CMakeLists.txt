cmake_minimum_required(VERSION 3.20)
project(onair_mbelib_neo_plugins LANGUAGES C)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "OnAir build expects a 64-bit Windows toolchain")
endif()

foreach(var MBELIB_NEO_ROOT MBELIB_NEO_BUILD MBELIB_NEO_DLL MBELIB_NEO_IMPLIB ONAIR_WRAPPER_SOURCE)
    if(NOT DEFINED ${var} OR NOT EXISTS "${${var}}")
        message(FATAL_ERROR "Missing required path: ${var}=${${var}}")
    endif()
endforeach()

add_library(mbe_neo_import SHARED IMPORTED GLOBAL)
set_target_properties(mbe_neo_import PROPERTIES
    IMPORTED_LOCATION "${MBELIB_NEO_DLL}"
    IMPORTED_IMPLIB "${MBELIB_NEO_IMPLIB}"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")

foreach(codec imbe ambe2 ambe)
    set(target "onair-codec-${codec}")
    add_library(${target} SHARED "${ONAIR_WRAPPER_SOURCE}")
    target_compile_features(${target} PRIVATE c_std_99)
    target_compile_definitions(${target} PRIVATE MBELIB_NEO=1)
    if(codec STREQUAL "imbe")
        target_compile_definitions(${target} PRIVATE CODEC_IMBE=1)
    elseif(codec STREQUAL "ambe2")
        target_compile_definitions(${target} PRIVATE CODEC_AMBE2=1)
    else()
        target_compile_definitions(${target} PRIVATE CODEC_AMBE=1)
    endif()
    target_include_directories(${target} PRIVATE
        "${MBELIB_NEO_ROOT}/include"
        "${MBELIB_NEO_BUILD}/include"
    )
    target_link_libraries(${target} PRIVATE mbe_neo_import)
    set_target_properties(${target} PROPERTIES PREFIX "" OUTPUT_NAME "${target}")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MBELIB_NEO_DLL}" "$<TARGET_FILE_DIR:${target}>/mbe-neo.dll"
        VERBATIM
    )
endforeach()


# TETRA uses a different speech format: 137 unpacked bits, 240 PCM samples.
add_library(onair-codec-tetra SHARED "${CMAKE_CURRENT_LIST_DIR}/tetra_wrapper.c")
target_compile_features(onair-codec-tetra PRIVATE c_std_99)
target_include_directories(onair-codec-tetra PRIVATE
    "${MBELIB_NEO_ROOT}/include"
    "${MBELIB_NEO_BUILD}/include"
)
target_link_libraries(onair-codec-tetra PRIVATE mbe_neo_import)
set_target_properties(onair-codec-tetra PROPERTIES
    PREFIX "" OUTPUT_NAME "onair-codec-tetra"
)
add_custom_command(TARGET onair-codec-tetra POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${MBELIB_NEO_DLL}" "$<TARGET_FILE_DIR:onair-codec-tetra>/mbe-neo.dll"
    VERBATIM
)

enable_testing()
add_executable(onair-codec-smoke "${CMAKE_CURRENT_LIST_DIR}/smoke.c")
foreach(codec imbe ambe2 ambe tetra)
    add_test(NAME "onair-${codec}-abi-smoke"
        COMMAND "$<TARGET_FILE:onair-codec-smoke>" "$<TARGET_FILE:onair-codec-${codec}>")
endforeach()
