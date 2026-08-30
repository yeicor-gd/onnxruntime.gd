extends Node

func test_ort_env() -> void:
	var env = OrtEnv.new()
	assert(env != null, "OrtEnv must instantiate")

func test_ort_session_options() -> void:
	var opts = OrtSessionOptions.new()
	assert(opts != null, "OrtSessionOptions must instantiate")
	
	# Test method chaining and setters
	var chained = opts.set_intra_op_num_threads(2).set_inter_op_num_threads(1).set_graph_optimization_level(1)
	assert(chained != null, "Method chaining should return valid Ref")
	
	opts.enable_cpu_mem_arena()
	opts.disable_cpu_mem_arena()
	opts.enable_mem_pattern()
	opts.disable_mem_pattern()
	opts.set_log_id("ort_test_logger")
	
	# Test config entries
	opts.add_config_entry("session.load_model_format", "ONNX")
	assert(opts.has_config_entry("session.load_model_format"), "Config entry should exist")
	assert(opts.get_config_entry("session.load_model_format") == "ONNX", "Config entry should match")
	assert(opts.get_config_entry_or_default("session.non_existent", "DEFAULT") == "DEFAULT", "Default config entry should match")
	
	# Test clone
	var cloned = opts.clone()
	assert(cloned != null, "Cloned session options must be valid")

func test_ort_run_options() -> void:
	var opts = OrtRunOptions.new()
	assert(opts != null, "OrtRunOptions must instantiate")
	
	opts.set_run_log_severity_level(2)
	opts.set_run_tag("unit_test_run")
	assert(opts.get_run_tag() == "unit_test_run", "Run tag should match")

func test_ort_memory_info() -> void:
	var mem_info = OrtMemoryInfo.new()
	assert(mem_info != null, "OrtMemoryInfo must instantiate")

func test_ort_session_direct_methods() -> void:
	var resource_path := "res://models/test_model.onnx"
	if not FileAccess.file_exists(resource_path):
		return
	var model_file := FileAccess.open(resource_path, FileAccess.READ)
	if model_file == null:
		return
	var model_data: PackedByteArray = model_file.get_buffer(model_file.get_length())
	model_file.close()
	if model_data.is_empty():
		return

	var env = OrtEnv.new()
	var opts = OrtSessionOptions.new()
	var session = OrtAdapters.create_session_from_memory(env, model_data, opts)
	assert(session != null, "OrtSession must be created")

	# Test direct autowrapped methods on OrtSession
	assert(session.get_input_count() == 1, "Input count should be 1")
	assert(session.get_output_count() == 1, "Output count should be 1")
	assert(session.get_overridable_initializer_count() == 0, "Initializer count should be 0")
	
	var in_names = session.get_input_names()
	var out_names = session.get_output_names()
	assert(in_names.size() == 1 and in_names[0] == "X", "Direct get_input_names should match")
	assert(out_names.size() == 1 and out_names[0] == "Y", "Direct get_output_names should match")
