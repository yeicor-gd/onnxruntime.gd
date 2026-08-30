extends RefCounted


func test_model_inference() -> String:
	var resource_path := "res://models/test_model.onnx"

	assert(
		FileAccess.file_exists(resource_path),
		"Model file not found: " + resource_path
	)

	var model_file := FileAccess.open(resource_path, FileAccess.READ)
	assert(
		model_file != null,
		"Failed to read model: " + resource_path
	)

	var model_data: PackedByteArray = model_file.get_buffer(
		model_file.get_length()
	)
	model_file.close()

	assert(
		not model_data.is_empty(),
		"Model file is empty: " + resource_path
	)

	var env = OrtEnv.new()
	var session_opts := OrtSessionOptions.new()

	var session = OrtAdapters.create_session_from_memory(
		env,
		model_data,
		session_opts
	)

	assert(session != null, "OrtSession must load model")

	var input_names = OrtAdapters.get_input_names(session)
	var output_names = OrtAdapters.get_output_names(session)

	assert(input_names.size() >= 1)
	assert(output_names.size() >= 1)
	assert(input_names[0] == "X")
	assert(output_names[0] == "Y")

	var in_data := PackedFloat32Array([
		10.0,
		20.0,
		30.0
	])

	var in_shape := PackedInt64Array([1, 3])

	var in_tensor = OrtAdapters.create_tensor_float32(
		in_data,
		in_shape
	)

	var inputs := {
		"X": in_tensor
	}

	var outputs = OrtAdapters.run_inference(
		session,
		inputs,
		PackedStringArray(["Y"])
	)

	assert(outputs.has("Y"))

	var out_tensor = outputs["Y"]

	assert(out_tensor != null)

	var out_data = OrtAdapters.get_tensor_data_float32(out_tensor)

	assert(out_data.size() == 3)

	assert(abs(out_data[0] - 10.0) < 1e-4)
	assert(abs(out_data[1] - 20.0) < 1e-4)
	assert(abs(out_data[2] - 30.0) < 1e-4)
	
	return "OK"
