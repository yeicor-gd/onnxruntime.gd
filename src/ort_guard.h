// ort_guard.h - Exception guard and error handling for ONNX Runtime Godot bindings.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32) || defined(_MSC_VER)
#undef min
#undef max
#endif

#include <exception>
#include <string>

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string.hpp>

// Include ONNX Runtime C++ headers
#include <onnxruntime_cxx_api.h>

namespace ort_gd {

struct LastError {
    ::godot::String message;
    int error_code = 0; // OrtErrorCode (0 = ORT_OK)
};

inline LastError &last_error_ref() {
    thread_local LastError err;
    return err;
}

inline void record_last_exception(const char *p_message, int p_code = -1) {
    LastError &err = last_error_ref();
    err.message = p_message ? ::godot::String(p_message) : ::godot::String("Unknown ONNX Runtime exception");
    err.error_code = p_code;
}

inline void record_last_exception(const ::Ort::Exception &e) {
    record_last_exception(e.what(), static_cast<int>(e.GetOrtErrorCode()));
}

inline void record_last_exception(const std::exception &e) {
    record_last_exception(e.what(), -1);
}

inline ::godot::String get_last_error_message() {
    return last_error_ref().message;
}

inline int get_last_error_code() {
    return last_error_ref().error_code;
}

inline void clear_last_error() {
    last_error_ref().message = ::godot::String();
    last_error_ref().error_code = 0;
}

inline bool &push_errors_ref() {
    static bool enabled = true;
    return enabled;
}

inline bool errors_pushed_on_exception() {
    return push_errors_ref();
}

inline void set_errors_pushed_on_exception(bool p_enabled) {
    push_errors_ref() = p_enabled;
}

} // namespace ort_gd

#define ORT_GUARD_FAIL_RETURN(m_default, ort_msg)                                            \
    if (ort_gd::errors_pushed_on_exception()) {                                              \
        ERR_FAIL_V_MSG(m_default, ort_msg);                                                  \
    }                                                                                        \
    return m_default;

#define ORT_GUARD_FAIL_VOID_RETURN(ort_msg)                                                  \
    if (ort_gd::errors_pushed_on_exception()) {                                              \
        ERR_FAIL_MSG(ort_msg);                                                               \
    }                                                                                        \
    return;

#define ORT_GUARD_CATCH_CTOR()                                                               \
    catch (const ::Ort::Exception &ort_e) {                                                  \
        ort_gd::record_last_exception(ort_e);                                                \
    } catch (const std::exception &e) {                                                      \
        ort_gd::record_last_exception(e);                                                    \
    } catch (...) {                                                                          \
        ort_gd::record_last_exception("Unknown ONNX Runtime / GDExtension exception", -1);   \
    }

#define ORT_GUARD_CATCH(m_default)                                                           \
    catch (const ::Ort::Exception &ort_e) {                                                  \
        ort_gd::record_last_exception(ort_e);                                                \
        ORT_GUARD_FAIL_RETURN(m_default, ort_e.what())                                       \
    } catch (const std::exception &e) {                                                      \
        ort_gd::record_last_exception(e);                                                    \
        ORT_GUARD_FAIL_RETURN(m_default, e.what())                                           \
    } catch (...) {                                                                          \
        ort_gd::record_last_exception("Unknown ONNX Runtime / GDExtension exception", -1);   \
        ORT_GUARD_FAIL_RETURN(m_default, "Unknown ONNX Runtime / GDExtension exception")     \
    }

#define ORT_GUARD_CATCH_VOID()                                                               \
    catch (const ::Ort::Exception &ort_e) {                                                  \
        ort_gd::record_last_exception(ort_e);                                                \
        ORT_GUARD_FAIL_VOID_RETURN(ort_e.what())                                             \
    } catch (const std::exception &e) {                                                      \
        ort_gd::record_last_exception(e);                                                    \
        ORT_GUARD_FAIL_VOID_RETURN(e.what())                                                 \
    } catch (...) {                                                                          \
        ort_gd::record_last_exception("Unknown ONNX Runtime / GDExtension exception", -1);   \
        ORT_GUARD_FAIL_VOID_RETURN("Unknown ONNX Runtime / GDExtension exception")           \
    }

#define OCC_CATCH_SIGNALS /* no-op in ONNX Runtime */
#define OCCT_GUARD_CATCH(m_default) ORT_GUARD_CATCH(m_default)
#define OCCT_GUARD_CATCH_VOID() ORT_GUARD_CATCH_VOID()
#define OCCT_GUARD_CATCH_CTOR() ORT_GUARD_CATCH_CTOR()


