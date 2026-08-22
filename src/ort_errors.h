// ort_errors.h - GDScript-facing error diagnostics class.
#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class OrtErrors : public RefCounted {
    GDCLASS(OrtErrors, RefCounted)

protected:
    static void _bind_methods();

public:
    static String get_last_error_message();
    static int get_last_error_code();
    static void clear_last_error();
    static bool are_errors_pushed();
    static void set_push_errors(bool p_enabled);
};

} // namespace godot
