// ort_adapters.cpp - High-level Godot adapters and convenience helpers for ONNX Runtime.

#include "ort_adapters.hpp"
#include "ort_guard.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>
#include <string>
#include <cstring>
#include <memory>

#include "autowrapper/OrtValue.hpp"
#include "autowrapper/OrtSession.hpp"
#include "autowrapper/OrtEnv.hpp"
#include "autowrapper/OrtSessionOptions.hpp"
#include "autowrapper/OrtMemoryInfo.hpp"
#include "autowrapper/OrtRunOptions.hpp"

namespace godot {

void OrtAdapters::_bind_methods() {
    // Session creation
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_session", "env", "model_path", "options"), &OrtAdapters::create_session, DEFVAL(Ref<OrtSessionOptions>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_session_from_memory", "env", "model_data", "options"), &OrtAdapters::create_session_from_memory, DEFVAL(Ref<OrtSessionOptions>()));

    // Tensor creation
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_float32", "data", "shape", "mem_info"), &OrtAdapters::create_tensor_float32, DEFVAL(Ref<OrtMemoryInfo>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_float64", "data", "shape", "mem_info"), &OrtAdapters::create_tensor_float64, DEFVAL(Ref<OrtMemoryInfo>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_int32", "data", "shape", "mem_info"), &OrtAdapters::create_tensor_int32, DEFVAL(Ref<OrtMemoryInfo>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_int64", "data", "shape", "mem_info"), &OrtAdapters::create_tensor_int64, DEFVAL(Ref<OrtMemoryInfo>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_bytes", "data", "shape", "element_type", "mem_info"), &OrtAdapters::create_tensor_bytes, DEFVAL(2 /* UINT8 */), DEFVAL(Ref<OrtMemoryInfo>()));
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("create_tensor_strings", "data", "shape"), &OrtAdapters::create_tensor_strings);

    // Tensor inspection
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_shape", "value"), &OrtAdapters::get_tensor_shape);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_element_type", "value"), &OrtAdapters::get_tensor_element_type);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_element_count", "value"), &OrtAdapters::get_tensor_element_count);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("is_tensor", "value"), &OrtAdapters::is_tensor);

    // Tensor data extraction
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_float32", "value"), &OrtAdapters::get_tensor_data_float32);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_float64", "value"), &OrtAdapters::get_tensor_data_float64);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_int32", "value"), &OrtAdapters::get_tensor_data_int32);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_int64", "value"), &OrtAdapters::get_tensor_data_int64);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_bytes", "value"), &OrtAdapters::get_tensor_data_bytes);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_tensor_data_strings", "value"), &OrtAdapters::get_tensor_data_strings);

    // Inference helper
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("run_inference", "session", "inputs", "output_names", "run_options"), &OrtAdapters::run_inference, DEFVAL(PackedStringArray()), DEFVAL(Ref<OrtRunOptions>()));

    // Metadata
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_input_names", "session"), &OrtAdapters::get_input_names);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_output_names", "session"), &OrtAdapters::get_output_names);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_input_count", "session"), &OrtAdapters::get_input_count);
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_output_count", "session"), &OrtAdapters::get_output_count);

    // Available providers
    ClassDB::bind_static_method("OrtAdapters", D_METHOD("get_available_providers"), &OrtAdapters::get_available_providers);
}

Ref<OrtSession> OrtAdapters::create_session(const Ref<OrtEnv> &p_env, const String &p_model_path, const Ref<OrtSessionOptions> &p_options) {
    ERR_FAIL_COND_V_MSG(p_env.is_null(), Ref<OrtSession>(), "OrtEnv is null");
    try {
        Ort::SessionOptions default_opts;
        const Ort::SessionOptions &opts = p_options.is_valid() ? p_options->_native : default_opts;
#ifdef _WIN32
        std::wstring path_str(p_model_path.wide_string().get_data());
        auto native_sess = std::make_unique<Ort::Session>(p_env->_native, path_str.c_str(), opts);
#else
        std::string path_str = p_model_path.utf8().get_data();
        auto native_sess = std::make_unique<Ort::Session>(p_env->_native, path_str.c_str(), opts);
#endif

        Ref<OrtSession> res;
        res.instantiate();
        res->_native = std::move(native_sess);
        return res;
    } ORT_GUARD_CATCH(Ref<OrtSession>())
}

Ref<OrtSession> OrtAdapters::create_session_from_memory(const Ref<OrtEnv> &p_env, const PackedByteArray &p_model_data, const Ref<OrtSessionOptions> &p_options) {
    ERR_FAIL_COND_V_MSG(p_env.is_null(), Ref<OrtSession>(), "OrtEnv is null");
    ERR_FAIL_COND_V_MSG(p_model_data.is_empty(), Ref<OrtSession>(), "Model data is empty");
    try {
        Ort::SessionOptions default_opts;
        const Ort::SessionOptions &opts = p_options.is_valid() ? p_options->_native : default_opts;
        auto native_sess = std::make_unique<Ort::Session>(p_env->_native, p_model_data.ptr(), p_model_data.size(), opts);

        Ref<OrtSession> res;
        res.instantiate();
        res->_native = std::move(native_sess);
        return res;
    } ORT_GUARD_CATCH(Ref<OrtSession>())
}

Ref<OrtValue> OrtAdapters::create_tensor_float32(const PackedFloat32Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info) {
    try {
        (void)p_mem_info;
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());
        int64_t total_elements = 1;
        for (int64_t d : shape) {
            total_elements *= d;
        }
        ERR_FAIL_COND_V_MSG(total_elements != p_data.size(), Ref<OrtValue>(), "Data size does not match tensor shape dimensions.");

        Ort::Value val = Ort::Value::CreateTensor<float>(allocator, shape.data(), shape.size());
        float *dst = val.GetTensorMutableData<float>();
        std::memcpy(dst, p_data.ptr(), p_data.size() * sizeof(float));

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

Ref<OrtValue> OrtAdapters::create_tensor_float64(const PackedFloat64Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info) {
    try {
        (void)p_mem_info;
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());
        int64_t total_elements = 1;
        for (int64_t d : shape) {
            total_elements *= d;
        }
        ERR_FAIL_COND_V_MSG(total_elements != p_data.size(), Ref<OrtValue>(), "Data size does not match tensor shape dimensions.");

        Ort::Value val = Ort::Value::CreateTensor<double>(allocator, shape.data(), shape.size());
        double *dst = val.GetTensorMutableData<double>();
        std::memcpy(dst, p_data.ptr(), p_data.size() * sizeof(double));

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

Ref<OrtValue> OrtAdapters::create_tensor_int32(const PackedInt32Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info) {
    try {
        (void)p_mem_info;
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());
        int64_t total_elements = 1;
        for (int64_t d : shape) {
            total_elements *= d;
        }
        ERR_FAIL_COND_V_MSG(total_elements != p_data.size(), Ref<OrtValue>(), "Data size does not match tensor shape dimensions.");

        Ort::Value val = Ort::Value::CreateTensor<int32_t>(allocator, shape.data(), shape.size());
        int32_t *dst = val.GetTensorMutableData<int32_t>();
        std::memcpy(dst, p_data.ptr(), p_data.size() * sizeof(int32_t));

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

Ref<OrtValue> OrtAdapters::create_tensor_int64(const PackedInt64Array &p_data, const PackedInt64Array &p_shape, const Ref<OrtMemoryInfo> &p_mem_info) {
    try {
        (void)p_mem_info;
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());
        int64_t total_elements = 1;
        for (int64_t d : shape) {
            total_elements *= d;
        }
        ERR_FAIL_COND_V_MSG(total_elements != p_data.size(), Ref<OrtValue>(), "Data size does not match tensor shape dimensions.");

        Ort::Value val = Ort::Value::CreateTensor<int64_t>(allocator, shape.data(), shape.size());
        int64_t *dst = val.GetTensorMutableData<int64_t>();
        std::memcpy(dst, p_data.ptr(), p_data.size() * sizeof(int64_t));

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

Ref<OrtValue> OrtAdapters::create_tensor_bytes(const PackedByteArray &p_data, const PackedInt64Array &p_shape, int p_element_type, const Ref<OrtMemoryInfo> &p_mem_info) {
    try {
        (void)p_mem_info;
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());

        ONNXTensorElementDataType ort_type = static_cast<ONNXTensorElementDataType>(p_element_type);
        Ort::Value val = Ort::Value::CreateTensor(allocator, shape.data(), shape.size(), ort_type);
        void *dst = val.GetTensorMutableRawData();
        std::memcpy(dst, p_data.ptr(), p_data.size());

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

Ref<OrtValue> OrtAdapters::create_tensor_strings(const PackedStringArray &p_data, const PackedInt64Array &p_shape) {
    try {
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<int64_t> shape(p_shape.ptr(), p_shape.ptr() + p_shape.size());
        std::vector<std::string> utf8_strings;
        std::vector<const char*> c_strings;
        utf8_strings.reserve(p_data.size());
        c_strings.reserve(p_data.size());
        for (int i = 0; i < p_data.size(); ++i) {
            utf8_strings.push_back(p_data[i].utf8().get_data());
            c_strings.push_back(utf8_strings.back().c_str());
        }

        Ort::Value val = Ort::Value::CreateTensor(allocator, shape.data(), shape.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
        val.FillStringTensor(c_strings.data(), c_strings.size());

        Ref<OrtValue> res;
        res.instantiate();
        res->_native = std::make_unique<Ort::Value>(std::move(val));
        return res;
    } ORT_GUARD_CATCH(Ref<OrtValue>())
}

bool OrtAdapters::is_tensor(const Ref<OrtValue> &p_value) {
    if (p_value.is_null() || !p_value->_native) return false;
    try {
        return p_value->_native->IsTensor();
    } catch (...) {
        return false;
    }
}

PackedInt64Array OrtAdapters::get_tensor_shape(const Ref<OrtValue> &p_value) {
    PackedInt64Array res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();
        res.resize(shape.size());
        for (size_t i = 0; i < shape.size(); ++i) {
            res[i] = shape[i];
        }
        return res;
    } ORT_GUARD_CATCH(res)
}

int OrtAdapters::get_tensor_element_type(const Ref<OrtValue> &p_value) {
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, 0, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        return static_cast<int>(type_info.GetElementType());
    } ORT_GUARD_CATCH(0)
}

int64_t OrtAdapters::get_tensor_element_count(const Ref<OrtValue> &p_value) {
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, 0, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        return static_cast<int64_t>(type_info.GetElementCount());
    } ORT_GUARD_CATCH(0)
}

PackedFloat32Array OrtAdapters::get_tensor_data_float32(const Ref<OrtValue> &p_value) {
    PackedFloat32Array res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        const float *src = p_value->_native->GetTensorData<float>();
        res.resize(count);
        std::memcpy(res.ptrw(), src, count * sizeof(float));
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedFloat64Array OrtAdapters::get_tensor_data_float64(const Ref<OrtValue> &p_value) {
    PackedFloat64Array res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        const double *src = p_value->_native->GetTensorData<double>();
        res.resize(count);
        std::memcpy(res.ptrw(), src, count * sizeof(double));
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedInt32Array OrtAdapters::get_tensor_data_int32(const Ref<OrtValue> &p_value) {
    PackedInt32Array res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        const int32_t *src = p_value->_native->GetTensorData<int32_t>();
        res.resize(count);
        std::memcpy(res.ptrw(), src, count * sizeof(int32_t));
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedInt64Array OrtAdapters::get_tensor_data_int64(const Ref<OrtValue> &p_value) {
    PackedInt64Array res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        const int64_t *src = p_value->_native->GetTensorData<int64_t>();
        res.resize(count);
        std::memcpy(res.ptrw(), src, count * sizeof(int64_t));
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedByteArray OrtAdapters::get_tensor_data_bytes(const Ref<OrtValue> &p_value) {
    PackedByteArray res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        const uint8_t *src = reinterpret_cast<const uint8_t*>(p_value->_native->GetTensorRawData());
        res.resize(count);
        std::memcpy(res.ptrw(), src, count);
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedStringArray OrtAdapters::get_tensor_data_strings(const Ref<OrtValue> &p_value) {
    PackedStringArray res;
    ERR_FAIL_COND_V_MSG(p_value.is_null() || !p_value->_native, res, "OrtValue is null or uninitialized");
    try {
        auto type_info = p_value->_native->GetTensorTypeAndShapeInfo();
        size_t count = type_info.GetElementCount();
        size_t total_len = p_value->_native->GetStringTensorDataLength();
        std::string buffer(total_len, '\0');
        std::vector<size_t> offsets(count);
        p_value->_native->GetStringTensorContent(buffer.data(), total_len, offsets.data(), offsets.size());
        res.resize(count);
        for (size_t i = 0; i < count; ++i) {
            size_t start = offsets[i];
            size_t end = (i + 1 < count) ? offsets[i + 1] : total_len;
            std::string str_val = buffer.substr(start, end - start);
            res[i] = String::utf8(str_val.c_str(), str_val.size());
        }
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedStringArray OrtAdapters::get_input_names(const Ref<OrtSession> &p_session) {
    PackedStringArray res;
    ERR_FAIL_COND_V_MSG(p_session.is_null() || !p_session->_native, res, "OrtSession is null or uninitialized");
    try {
        Ort::AllocatorWithDefaultOptions allocator;
        size_t count = p_session->_native->GetInputCount();
        res.resize(count);
        for (size_t i = 0; i < count; ++i) {
            auto name_ptr = p_session->_native->GetInputNameAllocated(i, allocator);
            res[i] = String::utf8(name_ptr.get());
        }
        return res;
    } ORT_GUARD_CATCH(res)
}

PackedStringArray OrtAdapters::get_output_names(const Ref<OrtSession> &p_session) {
    PackedStringArray res;
    ERR_FAIL_COND_V_MSG(p_session.is_null() || !p_session->_native, res, "OrtSession is null or uninitialized");
    try {
        Ort::AllocatorWithDefaultOptions allocator;
        size_t count = p_session->_native->GetOutputCount();
        res.resize(count);
        for (size_t i = 0; i < count; ++i) {
            auto name_ptr = p_session->_native->GetOutputNameAllocated(i, allocator);
            res[i] = String::utf8(name_ptr.get());
        }
        return res;
    } ORT_GUARD_CATCH(res)
}

int64_t OrtAdapters::get_input_count(const Ref<OrtSession> &p_session) {
    ERR_FAIL_COND_V_MSG(p_session.is_null() || !p_session->_native, 0, "OrtSession is null or uninitialized");
    try {
        return static_cast<int64_t>(p_session->_native->GetInputCount());
    } ORT_GUARD_CATCH(0)
}

int64_t OrtAdapters::get_output_count(const Ref<OrtSession> &p_session) {
    ERR_FAIL_COND_V_MSG(p_session.is_null() || !p_session->_native, 0, "OrtSession is null or uninitialized");
    try {
        return static_cast<int64_t>(p_session->_native->GetOutputCount());
    } ORT_GUARD_CATCH(0)
}

Dictionary OrtAdapters::run_inference(const Ref<OrtSession> &p_session, const Dictionary &p_inputs, const PackedStringArray &p_output_names, const Ref<OrtRunOptions> &p_run_options) {
    Dictionary results;
    ERR_FAIL_COND_V_MSG(p_session.is_null() || !p_session->_native, results, "OrtSession is null or uninitialized");
    try {
        // Prepare inputs
        Array input_keys = p_inputs.keys();
        std::vector<std::string> input_name_strings;
        std::vector<const char*> input_names;
        std::vector<Ort::Value> input_values;

        input_name_strings.reserve(input_keys.size());
        input_names.reserve(input_keys.size());
        input_values.reserve(input_keys.size());

        for (int i = 0; i < input_keys.size(); ++i) {
            String name = input_keys[i];
            input_name_strings.push_back(name.utf8().get_data());
            input_names.push_back(input_name_strings.back().c_str());

            Ref<OrtValue> val_ref = p_inputs[name];
            ERR_FAIL_COND_V_MSG(val_ref.is_null() || !val_ref->_native, results, String("Input tensor '{0}' is null or uninitialized").format(Array::make(name)));
            input_values.push_back(std::move(*val_ref->_native));
        }

        // Prepare outputs
        PackedStringArray out_names = p_output_names;
        if (out_names.is_empty()) {
            out_names = get_output_names(p_session);
        }

        std::vector<std::string> output_name_strings;
        std::vector<const char*> output_names;
        output_name_strings.reserve(out_names.size());
        output_names.reserve(out_names.size());

        for (int i = 0; i < out_names.size(); ++i) {
            output_name_strings.push_back(out_names[i].utf8().get_data());
            output_names.push_back(output_name_strings.back().c_str());
        }

        const Ort::RunOptions *opt = p_run_options.is_valid() ? &p_run_options->_native : nullptr;
        std::vector<Ort::Value> output_tensors;
        if (opt) {
            output_tensors = p_session->_native->Run(*opt, input_names.data(), input_values.data(), input_values.size(), output_names.data(), output_names.size());
        } else {
            Ort::RunOptions default_opts;
            output_tensors = p_session->_native->Run(default_opts, input_names.data(), input_values.data(), input_values.size(), output_names.data(), output_names.size());
        }

        // Re-assign moved input values back to their OrtValue wrappers so they remain valid
        for (int i = 0; i < input_keys.size(); ++i) {
            String name = input_keys[i];
            Ref<OrtValue> val_ref = p_inputs[name];
            val_ref->_native = std::make_unique<Ort::Value>(std::move(input_values[i]));
        }

        // Wrap output tensors into Godot OrtValue objects
        for (size_t i = 0; i < output_tensors.size(); ++i) {
            Ref<OrtValue> out_val;
            out_val.instantiate();
            out_val->_native = std::make_unique<Ort::Value>(std::move(output_tensors[i]));
            results[out_names[i]] = out_val;
        }

        return results;
    } ORT_GUARD_CATCH(results)
}

PackedStringArray OrtAdapters::get_available_providers() {
    PackedStringArray res;
    try {
        auto providers = Ort::GetAvailableProviders();
        res.resize(providers.size());
        for (size_t i = 0; i < providers.size(); ++i) {
            res[i] = String::utf8(providers[i].c_str());
        }
        return res;
    } catch (...) {
        return res;
    }
}

} // namespace godot
