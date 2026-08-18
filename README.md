# ONNXRuntime.gd

This project provides an automatic high-performance GDExtension wrapper for **Microsoft ONNX Runtime** in the Godot Engine.

## Features

- **Full ONNX Runtime C++ API Support**: Automatically generated bindings exposing environments (`OrtEnv`), session options (`OrtSessionOptions`), run options (`OrtRunOptions`), memory info (`OrtMemoryInfo`), model metadata, tensor info, and more.
- **High-Level Adapters (`OrtAdapters`)**: Idiomatic GDScript helpers for:
  - Creating and loading sessions from file paths or in-memory buffers
  - Inspecting input and output tensor names, types, and shapes
  - Creating tensor `OrtValue` objects from GDScript `PackedFloat32Array`, `PackedInt32Array`, `PackedInt64Array`, and `PackedByteArray`
  - Running model inference with dictionary input/output mapping
  - Querying available execution providers (CPU, CUDA, TensorRT, CoreML, DirectML, etc.)
- **Error Handling (`OrtErrors`)**: Diagnostic helpers for inspecting last error messages, error codes, and clearing error state.
- **Cross-Platform Support**: Desktop (Linux, Windows, macOS), mobile (Android, iOS), and web.
- **Demo & Tests**: Interactive demo scene (`demo/demo/demo.tscn`) and automated test suites (`demo/tests/`).
- **Dependency Management**: Integrated with [vcpkg](https://github.com/microsoft/vcpkg) for reproducible builds.

## Quick Start

### 1. Basic Instantiation & Session Creation

```gdscript
# Create environment and session options
var env = OrtEnv.new()
var session_opts = OrtSessionOptions.new()
session_opts.set_intra_op_num_threads(2)

# Load ONNX model
var model_path = ProjectSettings.globalize_path("res://models/test_model.onnx")
var session = OrtAdapters.create_session(env, model_path, session_opts)

# Inspect inputs & outputs
var input_names = OrtAdapters.get_input_names(session)   # ["X"]
var output_names = OrtAdapters.get_output_names(session) # ["Y"]
```

### 2. Tensor Creation & Inference

```gdscript
# Prepare input tensor: shape [1, 3] with data [1.0, 2.0, 3.0]
var input_data := PackedFloat32Array([1.0, 2.0, 3.0])
var input_shape := PackedInt64Array([1, 3])
var in_val = OrtAdapters.create_tensor_float32(input_data, input_shape)

# Run inference
var inputs = { "X": in_val }
var outputs = OrtAdapters.run_inference(session, inputs, PackedStringArray(["Y"]))

# Retrieve output
if outputs.has("Y"):
    var out_tensor = outputs["Y"]
    var out_shape = OrtAdapters.get_tensor_shape(out_tensor)
    var out_data = OrtAdapters.get_tensor_data_float32(out_tensor)
    print("Inference result: ", out_data)
```

## Building from Source

```bash
# 1. Install dependencies via vcpkg
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGODOTCPP_TARGET=template_debug
cmake --build build -j$(nproc)
cmake --install build

# 2. Run automated validation tests
GODOT_VERSION=system ./validate.sh
```

## License

ONNX Runtime is licensed under the MIT License.
