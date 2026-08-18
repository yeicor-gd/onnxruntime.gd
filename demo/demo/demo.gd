extends Control

@onready var status_label: Label = $VBox/StatusLabel
@onready var output_text: TextEdit = $VBox/OutputText
@onready var run_btn: Button = $VBox/HBox/RunButton
@onready var back_btn: Button = $VBox/HBox/BackButton

var session = null
var env = null

func _ready() -> void:
	back_btn.pressed.connect(_on_back_pressed)
	run_btn.pressed.connect(_on_run_pressed)
	_init_ort()

func _init_ort() -> void:
	var providers = OrtAdapters.get_available_providers()
	output_text.text = "=== ONNX Runtime for Godot ===\n"
	output_text.text += "Available Execution Providers:\n"
	for p in providers:
		output_text.text += "  - " + p + "\n"
	output_text.text += "\n"

	var model_path := ProjectSettings.globalize_path("res://models/test_model.onnx")
	if not FileAccess.file_exists(model_path):
		status_label.text = "Model file not found: " + model_path
		return

	env = OrtEnv.new()
	var session_opts = OrtSessionOptions.new()
	session = OrtAdapters.create_session(env, model_path, session_opts)
	if session != null:
		var input_names = OrtAdapters.get_input_names(session)
		var output_names = OrtAdapters.get_output_names(session)
		status_label.text = "Model loaded successfully! Inputs: " + str(input_names) + ", Outputs: " + str(output_names)
		output_text.text += "Model loaded: " + model_path + "\n"
		output_text.text += "Inputs: " + str(input_names) + "\n"
		output_text.text += "Outputs: " + str(output_names) + "\n\n"
	else:
		status_label.text = "Failed to load ONNX session."

func _on_run_pressed() -> void:
	if session == null:
		status_label.text = "Session is not loaded."
		return

	var x_data := PackedFloat32Array([1.0, 2.0, 3.0])
	var x_shape := PackedInt64Array([1, 3])
	var in_val = OrtAdapters.create_tensor_float32(x_data, x_shape)

	output_text.text += "Running inference with input X = [1.0, 2.0, 3.0] (shape [1, 3])...\n"

	var inputs = { "X": in_val }
	var outputs = OrtAdapters.run_inference(session, inputs, PackedStringArray(["Y"]))

	if outputs.has("Y"):
		var out_val = outputs["Y"]
		var out_shape = OrtAdapters.get_tensor_shape(out_val)
		var out_data = OrtAdapters.get_tensor_data_float32(out_val)
		output_text.text += "Output Y shape: " + str(out_shape) + "\n"
		output_text.text += "Output Y data: " + str(out_data) + "\n"
		output_text.text += "Inference successful!\n\n"
		status_label.text = "Inference completed! Result: " + str(out_data)
	else:
		status_label.text = "Inference failed to produce output Y."

func _on_back_pressed() -> void:
	get_tree().change_scene_to_file("res://main.tscn")
