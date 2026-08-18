// ort_errors.cpp - GDScript-facing error diagnostics implementation.

#include "ort_errors.h"
#include "ort_guard.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void OrtErrors::_bind_methods() {
    ClassDB::bind_static_method("OrtErrors", D_METHOD("get_last_error_message"), &OrtErrors::get_last_error_message);
    ClassDB::bind_static_method("OrtErrors", D_METHOD("get_last_error_code"), &OrtErrors::get_last_error_code);
    ClassDB::bind_static_method("OrtErrors", D_METHOD("clear_last_error"), &OrtErrors::clear_last_error);
    ClassDB::bind_static_method("OrtErrors", D_METHOD("are_errors_pushed"), &OrtErrors::are_errors_pushed);
    ClassDB::bind_static_method("OrtErrors", D_METHOD("set_push_errors", "enabled"), &OrtErrors::set_push_errors);
}

String OrtErrors::get_last_error_message() {
    return ort_gd::get_last_error_message();
}

int OrtErrors::get_last_error_code() {
    return ort_gd::get_last_error_code();
}

void OrtErrors::clear_last_error() {
    ort_gd::clear_last_error();
}

bool OrtErrors::are_errors_pushed() {
    return ort_gd::errors_pushed_on_exception();
}

void OrtErrors::set_push_errors(bool p_enabled) {
    ort_gd::set_errors_pushed_on_exception(p_enabled);
}

} // namespace godot
