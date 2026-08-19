include("triplets/x64-linux.cmake")

# CUDA 13.0+ removed support for Maxwell, Pascal (sm_60) and Volta (sm_70).
# Override CMAKE_CUDA_ARCHITECTURES to Turing+ (sm_75 to sm_120) to avoid nvcc compiler errors.
if(NOT DEFINED VCPKG_CMAKE_CONFIGURE_OPTIONS)
  set(VCPKG_CMAKE_CONFIGURE_OPTIONS "")
endif()
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_CUDA_ARCHITECTURES=75;80;86;89;90;100;120")

set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS}")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -Wno-template-body")

# Forward -Wno-template-body to the host compiler through nvcc when compiling CUDA source files (.cu)
list(APPEND VCPKG_ENV_PASSTHROUGH_UNTRACKED NVCC_APPEND_FLAGS)
set(ENV{NVCC_APPEND_FLAGS} "-Xcompiler -Wno-template-body")
