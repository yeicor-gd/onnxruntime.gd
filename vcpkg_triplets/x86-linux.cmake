include("triplets/community/x86-linux.cmake")

# onnxruntime/core/common/denormal.cc uses _MM_SET_FLUSH_ZERO_MODE (SSE) and
# _MM_SET_DENORMALS_ZERO_MODE (SSE3), both always_inline intrinsics. The
# community x86-linux triplet builds with -m32 and no SSE flags, so GCC fails
# with "target specific option mismatch". Enable SSE2/SSE3 to satisfy them.
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} -msse2 -msse3")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -msse2 -msse3")