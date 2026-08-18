// ort_adapters.hpp - High-level Godot adapters and convenience helpers for ONNX Runtime.
#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <onnxruntime_cxx_api.h>

namespace godot {

class OrtValue;
class OrtSession;
class OrtEnv;
class OrtSessionOptions;
class OrtMemoryInfo;
class OrtRunOptions;

class OrtAdapters : public RefCounted {
    GDCLASS(OrtAdapters, RefCounted);

protected:
    static void _bind_methods();

public:
    // Session creation helpers
    static Ref<OrtSession> create_session(const Ref<OrtEnv> &p_env, const String &p_model_path, const Ref<OrtSessionOptions> &p_options = nullptr);
    static Ref<OrtSession> create_session_from_memory(const Ref<OrtEnv> &p_env, const PackedByteArray &p_model_data, const Ref<OrtSessionOptions> &p_options = nullptr);

    // Tensor creation helpers
    static Ref<OrtValue> create_tensor_float32(const PackedFloat32Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info = nullptr);
    static Ref<OrtValue> create_tensor_float64(const PackedFloat64Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info = nullptr);
    static Ref<OrtValue> create_tensor_int32(const PackedInt32Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info = nullptr);
    static Ref<OrtValue> create_tensor_int64(const PackedInt64Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info = nullptr);
    static Ref<OrtValue> create_tensor_bytes(const PackedByteArray &p_data, const PackedInt64Array &p_shape, int p_element_type = 2 /* UINT8 */, const Ref<OrtMemoryInfo> &p_mem_info = nullptr);
    static Ref<OrtValue> create_tensor_strings(const PackedStringArray &p_data, const PackedInt64Array &p_shape);

    // Tensor inspection and data extraction
    static PackedInt64Array get_tensor_shape(const Ref<OrtValue> &p_value);
    static int get_tensor_element_type(const Ref<OrtValue> &p_value);
    static int64_t get_tensor_element_count(const Ref<OrtValue> &p_value);
    static bool is_tensor(const Ref<OrtValue> &p_value);

    static PackedFloat32Array get_tensor_data_float32(const Ref<OrtValue> &p_value);
    static PackedFloat64Array get_tensor_data_float64(const Ref<OrtValue> &p_value);
    static PackedInt32Array get_tensor_data_int32(const Ref<OrtValue> &p_value);
    static PackedInt64Array get_tensor_data_int64(const Ref<OrtValue> &p_value);
    static PackedByteArray get_tensor_data_bytes(const Ref<OrtValue> &p_value);
    static PackedStringArray get_tensor_data_strings(const Ref<OrtValue> &p_value);

    // Session inference helper
    static Dictionary run_inference(const Ref<OrtSession> &p_session, const Dictionary &p_inputs, const PackedStringArray &p_output_names = PackedStringArray(), const Ref<OrtRunOptions> &p_run_options = nullptr);

    // Session metadata / IO inspection
    static PackedStringArray get_input_names(const Ref<OrtSession> &p_session);
    static PackedStringArray get_output_names(const Ref<OrtSession> &p_session);
    static int64_t get_input_count(const Ref<OrtSession> &p_session);
    static int64_t get_output_count(const Ref<OrtSession> &p_session);

    // Available execution providers list
    static PackedStringArray get_available_providers();
};

} // namespace godot
