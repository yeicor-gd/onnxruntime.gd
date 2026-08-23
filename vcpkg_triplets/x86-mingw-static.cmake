include("triplets/community/x86-mingw-static.cmake")

# onnxruntime/core/common/denormal.cc uses _MM_SET_FLUSH_ZERO_MODE (SSE) and
# _MM_SET_DENORMALS_ZERO_MODE (SSE3), both always_inline intrinsics. The i686
# default instruction set has no SSE, so GCC fails with "target specific option
# mismatch". Enable SSE2 with SSE math to satisfy them.
#
# Large generated sources (onnxruntime, protobuf) exceed GCC's default object
# file section capacity; allow the assembler to use the big-object format.
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} -msse2 -mfpmath=sse -Wa,-mbig-obj")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -msse2 -mfpmath=sse -Wa,-mbig-obj")
