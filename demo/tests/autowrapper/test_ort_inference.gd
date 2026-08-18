extends Node

func test_model_inference() -> void:
	var model_path := ProjectSettings.globalize_path("res://models/test_model.onnx")
	assert(FileAccess.file_exists(model_path), "test_model.onnx must exist at path: " + model_path)

	var env = OrtEnv.new()
	var session_options = OrtSessionOptions.new()
	var session = OrtAdapters.create_session(env, model_path, session_options)
	assert(session != null, "OrtSession must load model")

	var input_names = OrtAdapters.get_input_names(session)
	var output_names = OrtAdapters.get_output_names(session)
	assert(input_names.size() >= 1, "Model must have at least 1 input")
	assert(output_names.size() >= 1, "Model must have at least 1 output")
	assert(input_names[0] == "X", "Input name must be 'X'")
	assert(output_names[0] == "Y", "Output name must be 'Y'")

	# Create input tensor X = [10.0, 20.0, 30.0]
	var in_data := PackedFloat32Array([10.0, 20.0, 30.0])
	var in_shape := PackedInt64Array([1, 3])
	var in_tensor = OrtAdapters.create_tensor_float32(in_data, in_shape)

	var inputs := { "X": in_tensor }
	var outputs = OrtAdapters.run_inference(session, inputs, PackedStringArray(["Y"]))
	assert(outputs.has("Y"), "Outputs dictionary must contain 'Y'")

	var out_tensor = outputs["Y"]
	assert(out_tensor != null, "Output tensor Y must not be null")

	var out_data = OrtAdapters.get_tensor_data_float32(out_tensor)
	assert(out_data.size() == 3, "Output data size must be 3")

	# Expected Y = X = [10.0, 20.0, 30.0]
	assert(abs(out_data[0] - 10.0) < 1e-4, "Output[0] must be ~10.0, got: " + str(out_data[0]))
	assert(abs(out_data[1] - 20.0) < 1e-4, "Output[1] must be ~20.0, got: " + str(out_data[1]))
	assert(abs(out_data[2] - 30.0) < 1e-4, "Output[2] must be ~30.0, got: " + str(out_data[2]))
