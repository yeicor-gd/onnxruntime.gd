cmake_minimum_required(VERSION 3.25)

include(CheckCXXCompilerFlag)

message(STATUS "Using toolchain file: " "${CMAKE_TOOLCHAIN_FILE}")
option(ENABLE_WERROR "Enable warnings as errors (ON|OFF)" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers (ON|OFF)" OFF)
# Auto-force full LTO in release builds if available
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_SUPPORT_OUTPUT)
  if(IPO_SUPPORTED)
    message(STATUS "IPO (LTO) is supported, enabling it for Release builds")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO (LTO) is not supported: "
            "${IPO_SUPPORT_OUTPUT}")
  endif()
else()
  message(STATUS "IPO (LTO) is not enabled for Debug "
        "builds")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ONNXRuntime.gd CXX)  # Microsoft ONNX Runtime wrapper for Godot

# Map imported debug dependencies to Release libraries to prevent debug schema assertion in ONNX
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)

# Automatically use ccache or sccache as a compiler launcher when one is available
find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)
if(CCACHE_PROGRAM OR SCCACHE_PROGRAM)
  if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache for compilation: ${CCACHE_PROGRAM}")
  else()
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "Using sccache for compilation: ${SCCACHE_PROGRAM}")
  endif()
endif()

# Check that both CMAKE_BUILD_TYPE and GODOTCPP_TARGET are set correctly
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND
    NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(FATAL_ERROR "CMAKE_BUILD_TYPE must be either 'Release' or 'Debug'. "
        "Current value: ${CMAKE_BUILD_TYPE}")
endif ()
if (NOT GODOTCPP_TARGET STREQUAL "template_release" AND
    NOT GODOTCPP_TARGET STREQUAL "template_debug")
  message(FATAL_ERROR "GODOTCPP_TARGET must be either 'template_release' or "
        "'template_debug'. Current value: ${GODOTCPP_TARGET}")
endif ()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}, "
    "GODOTCPP_TARGET: ${GODOTCPP_TARGET}")

# No-op when CMAKE_BUILD_TYPE does not match the GODOTCPP_TARGET
if (CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT GODOTCPP_TARGET STREQUAL "template_release")
  message(WARNING "CMAKE_BUILD_TYPE is 'Release' but GODOTCPP_TARGET is not 'template_release'. Ignoring this build to speed up the build process.")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/") # No-op installation
  return()
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT GODOTCPP_TARGET STREQUAL "template_debug")
  message(WARNING "CMAKE_BUILD_TYPE is 'Debug' but GODOTCPP_TARGET is not 'template_debug'. Ignoring this build to speed up the build process.")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/") # No-op installation
  return()
endif ()

# Fix CMAKE_OSX_SYSROOT for iOS to use full SDK path
set(ENV{SDKROOT} "${CMAKE_OSX_SYSROOT}")
set(GODOTCPP_DISABLE_EXCEPTIONS ON)

# Godot C++ bindings
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp")

# Dependencies: ONNX Runtime (consumed via vcpkg).
find_package(onnxruntime CONFIG)
if(NOT onnxruntime_FOUND)
  find_package(onnxruntime REQUIRED)
endif()

# Regenerate the ONNX Runtime autowrapper bindings at configure time when needed
set(AUTOWRAPPER_DIR "${PROJECT_SOURCE_DIR}/onnxruntime.gd-autowrapper")
set(AUTOWRAPPER_STAMP "${PROJECT_SOURCE_DIR}/src/autowrapper/.autowrapper-stamp")
set(AUTOWRAPPER_NEED_REGEN FALSE)
if (NOT EXISTS "${AUTOWRAPPER_STAMP}")
  set(AUTOWRAPPER_NEED_REGEN TRUE)
  message(STATUS "ONNX Runtime autowrapper: no stamp, will regenerate")
else()
  file(STRINGS "${AUTOWRAPPER_STAMP}" _stamp_lines)
  set(_stamp_ort "")
  foreach(_l IN LISTS _stamp_lines)
    if (_l MATCHES "^ort=(.*)$")
      set(_stamp_ort "${CMAKE_MATCH_1}")
      break()
    endif()
  endforeach()
  if (NOT "${_stamp_ort}" STREQUAL "${onnxruntime_VERSION}")
    set(AUTOWRAPPER_NEED_REGEN TRUE)
    message(STATUS "ONNX Runtime autowrapper: ORT version changed "
        "(${_stamp_ort} -> ${onnxruntime_VERSION}), will regenerate")
  else()
    file(GLOB_RECURSE _gen_srcs "${AUTOWRAPPER_DIR}/autogen/*.py")
    foreach(_s IN LISTS _gen_srcs)
      if ("${_s}" IS_NEWER_THAN "${AUTOWRAPPER_STAMP}")
        set(AUTOWRAPPER_NEED_REGEN TRUE)
        message(STATUS "ONNX Runtime autowrapper: generator sources newer than "
            "stamp (${_s}), will regenerate")
        break()
      endif()
    endforeach()
  endif()
endif()
if (AUTOWRAPPER_NEED_REGEN)
  if (DEFINED ENV{PYTHON} AND NOT "$ENV{PYTHON}" STREQUAL "")
    set(_autogen_python "$ENV{PYTHON}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(_autogen_python "${Python3_EXECUTABLE}")
  endif()
  cmake_host_system_information(RESULT _nproc QUERY NUMBER_OF_LOGICAL_CORES)
  if (NOT _nproc MATCHES "^[0-9]+$" OR _nproc EQUAL 0)
    set(_nproc 4)
  endif()
  if (DEFINED ENV{GITHUB_ACTIONS})
    if (_nproc GREATER_EQUAL 4)
      set(_autogen_jobs 4)
    else()
      set(_autogen_jobs "${_nproc}")
    endif()
  else()
    set(_autogen_jobs "${_nproc}")
  endif()
  if (_autogen_jobs LESS 1)
    set(_autogen_jobs 1)
  endif()
  if (DEFINED VCPKG_TARGET_TRIPLET)
    set(ENV{VCPKG_DEFAULT_TRIPLET} "${VCPKG_TARGET_TRIPLET}")
  endif()
  message(STATUS "ONNX Runtime autowrapper: regenerating bindings "
      "(ort ${onnxruntime_VERSION}, jobs ${_autogen_jobs})...")
  execute_process(
    COMMAND "${_autogen_python}" -m autogen regenerate --jobs "${_autogen_jobs}"
    WORKING_DIRECTORY "${AUTOWRAPPER_DIR}"
    RESULT_VARIABLE _autogen_rc
    OUTPUT_VARIABLE _autogen_out
    ERROR_VARIABLE _autogen_err)
  if (_autogen_out)
    message(STATUS "ONNX Runtime autowrapper output:\n${_autogen_out}")
  endif()
  if (_autogen_err)
    message(STATUS "ONNX Runtime autowrapper stderr:\n${_autogen_err}")
  endif()
  if (NOT _autogen_rc EQUAL 0)
    message(FATAL_ERROR "ONNX Runtime autowrapper generation failed (rc=${_autogen_rc}):\n"
        "${_autogen_out}\n${_autogen_err}")
  endif()
endif()

# Define the documentation sources
if(GODOTCPP_TARGET MATCHES "editor|template_debug")
  find_package(Python3 3.4 REQUIRED)
  file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/doc_classes/*.xml"
        "${PROJECT_SOURCE_DIR}/doc_classes/autowrapper/*.xml")
  set(DOC_GEN_FILE "${CMAKE_CURRENT_BINARY_DIR}/gen/doc_data.cpp")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")
  add_custom_command(
    OUTPUT ${DOC_GEN_FILE}
    COMMAND ${Python3_EXECUTABLE} "-c"
        "from doc_source_generator import generate_doc_source_from_directory; generate_doc_source_from_directory('${DOC_GEN_FILE}', '${PROJECT_SOURCE_DIR}/doc_classes/autowrapper')"
    VERBATIM
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp
    DEPENDS ${DOC_XML})
  add_custom_target(generate_doc_data DEPENDS ${DOC_GEN_FILE})
endif()

# Define the shared library target
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/**/*.cpp"
    "src/autowrapper/*.cpp"
    "src/autowrapper/**/*.cpp"
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/autowrapper")
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  add_library(${PROJECT_NAME} MODULE ${SOURCES} ${DOC_GEN_FILE})
  target_link_options(${PROJECT_NAME} PRIVATE
    "-sSIDE_MODULE=2"
    "-sEXPORTED_FUNCTIONS=_gdext_library_init"
  )
else()
  add_library(${PROJECT_NAME} SHARED ${SOURCES} ${DOC_GEN_FILE})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-z,muldefs")
elseif(MINGW)
  target_link_libraries(${PROJECT_NAME} PRIVATE "-Wl,--allow-multiple-definition")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
  find_program(GDEXT_LLD NAMES ld.lld)
  if(GDEXT_LLD)
    message(STATUS "AArch64 Linux: linking with LLD (${GDEXT_LLD})")
    target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=lld")
  else()
    message(WARNING "AArch64 Linux: LLD not found")
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,-Bsymbolic"
    "-Wl,-Bsymbolic-functions"
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/gdext.version"
    "-Wl,-rpath,$ORIGIN"
    "-static-libstdc++"
    "-static-libgcc"
  )
endif()

# Link godot-cpp and onnxruntime
if(TARGET onnxruntime::onnxruntime)
  target_link_libraries(${PROJECT_NAME} PRIVATE godot-cpp onnxruntime::onnxruntime)
elseif(TARGET onnxruntime)
  target_link_libraries(${PROJECT_NAME} PRIVATE godot-cpp onnxruntime)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE godot-cpp ${ONNXRUNTIME_LIBRARIES})
endif()


target_include_directories(${PROJECT_NAME} PRIVATE src)
find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime onnxruntime/core/session)
find_path(ONNXRUNTIME_BASE_INCLUDE_DIR NAMES onnxruntime/core/session/onnxruntime_cxx_api.h)
if(ONNXRUNTIME_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
endif()
if(ONNXRUNTIME_BASE_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${ONNXRUNTIME_BASE_INCLUDE_DIR}"
    "${ONNXRUNTIME_BASE_INCLUDE_DIR}/onnxruntime"
    "${ONNXRUNTIME_BASE_INCLUDE_DIR}/onnxruntime/core/session"
  )
endif()
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE src/autowrapper)
get_target_property(GODOTCPP_INCLUDES godot-cpp INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${GODOTCPP_INCLUDES})

# Define NOMINMAX and WIN32_LEAN_AND_MEAN to prevent windows.h macro collisions
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

# Force-include the exception guard header for all generated and handwritten TUs
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/ort_guard.h"
    "/utf-8"
    "/wd4828"
    "/bigobj"
  )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/ort_guard.h"
  )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:-g1;-fno-var-tracking>"
  )
endif()

if(MSVC)
  set(WARN_FLAGS "/W4")
  if(ENABLE_WERROR)
    list(APPEND WARN_FLAGS "/WX")
  endif()
else()
  set(WARN_FLAGS "-Wall;-Wextra;-Wpedantic")
  if(ENABLE_WERROR)
    list(APPEND WARN_FLAGS "-Werror")
  endif()
endif()
target_compile_options(${PROJECT_NAME} PRIVATE "${WARN_FLAGS}")

# Sanitizers in debug builds if supported
if(ENABLE_SANITIZERS)
  function(enable_sanitizers target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Sanitizers skipped: MinGW GCC lacks support.")
        return()
      endif()
      set(SAN_FLAGS "-fsanitize=address,undefined")
      check_cxx_compiler_flag("-fsanitize=leak" HAS_LSAN)
      if(HAS_LSAN)
        set(SAN_FLAGS "${SAN_FLAGS},leak")
      endif()
      target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fstack-protector-all>)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:${SAN_FLAGS} -static-libasan -fno-omit-frame-pointer -g>)
      target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:${SAN_FLAGS} -static-libasan>)
    elseif(MSVC)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/RTC1>)
    endif()
  endfunction()
  enable_sanitizers(${PROJECT_NAME})
endif()

# Install the bindings to the demo project
get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)
if(NOT GODOTCPP_SUFFIX)
  message(FATAL_ERROR "GODOTCPP_SUFFIX property not set on godot-cpp target.")
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
string(REPLACE ".template_" ".${BUILD_TYPE_LOWER}.template_" GODOTCPP_SUFFIX "${GODOTCPP_SUFFIX}")
set(OUTPUT_NAME "gdext${GODOTCPP_SUFFIX}")
if (MINGW)
  set(OUTPUT_NAME "gdext${GODOTCPP_SUFFIX}.mingw")
endif()
set(INSTALL_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/demo/addons/onnxruntime.gd")
message("Will install shared library to: ${INSTALL_FOLDER}")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${OUTPUT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib")
set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN")
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${INSTALL_FOLDER}")

# Copy & install ONNX Runtime provider shared libraries (e.g. CUDA / shared providers)
if(DEFINED CMAKE_PREFIX_PATH)
  file(GLOB ONNXRUNTIME_PROVIDER_LIBS
    "${CMAKE_PREFIX_PATH}/lib/libonnxruntime_providers_*"
    "${CMAKE_PREFIX_PATH}/bin/onnxruntime_providers_*"
  )
endif()
if(NOT ONNXRUNTIME_PROVIDER_LIBS)
  file(GLOB ONNXRUNTIME_PROVIDER_LIBS
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/*/lib/libonnxruntime_providers_*"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/*/debug/lib/libonnxruntime_providers_*"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/*/bin/onnxruntime_providers_*"
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/*/debug/bin/onnxruntime_providers_*"
  )
endif()
if(ONNXRUNTIME_PROVIDER_LIBS)
  message(STATUS "Found ONNX Runtime provider libraries: ${ONNXRUNTIME_PROVIDER_LIBS}")
  install(FILES ${ONNXRUNTIME_PROVIDER_LIBS} DESTINATION "${INSTALL_FOLDER}")
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ONNXRUNTIME_PROVIDER_LIBS}
            "${INSTALL_FOLDER}/"
    COMMENT "Copying ONNX Runtime provider libraries..."
    VERBATIM
  )
endif()

# Post-build: repair Emscripten wasm validation bugs
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env
            "DEBUG=$<IF:$<CONFIG:Debug>,1,>"
            "EMSDK=$ENV{EMSDK}"
            ${PYTHON3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/repair_wasm.py
            $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Repairing and optimizing WASM binary..."
    VERBATIM
  )
endif()
