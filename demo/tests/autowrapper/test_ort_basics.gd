extends Node

func test_ort_env() -> void:
	var env = OrtEnv.new()
	assert(env != null, "OrtEnv must instantiate")

func test_ort_session_options() -> void:
	var opts = OrtSessionOptions.new()
	assert(opts != null, "OrtSessionOptions must instantiate")

func test_ort_run_options() -> void:
	var opts = OrtRunOptions.new()
	assert(opts != null, "OrtRunOptions must instantiate")

func test_ort_memory_info() -> void:
	var mem_info = OrtMemoryInfo.new()
	assert(mem_info != null, "OrtMemoryInfo must instantiate")
