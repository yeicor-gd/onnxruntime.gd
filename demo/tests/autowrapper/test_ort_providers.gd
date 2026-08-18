extends Node

func test_available_providers() -> void:
	var providers = OrtAdapters.get_available_providers()
	assert(providers.size() > 0, "At least one provider must be available")
	assert(providers.has("CPUExecutionProvider"), "CPUExecutionProvider must be present")
	print("Available ONNX Runtime execution providers on this system: ", providers)
