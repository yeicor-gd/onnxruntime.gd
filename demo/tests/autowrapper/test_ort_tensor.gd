extends Node

func test_float32_tensor() -> void:
	var shape := PackedInt64Array([1, 4])
	var data := PackedFloat32Array([1.5, -2.5, 3.25, 4.0])
	var tensor = OrtAdapters.create_tensor_float32(data, shape)
	assert(tensor != null, "Tensor creation must succeed")
	assert(OrtAdapters.is_tensor(tensor), "is_tensor must return true")

	var out_shape = OrtAdapters.get_tensor_shape(tensor)
	assert(out_shape.size() == 2, "Shape rank must be 2")
	assert(out_shape[0] == 1 and out_shape[1] == 4, "Shape dimensions must match")

	var count = OrtAdapters.get_tensor_element_count(tensor)
	assert(count == 4, "Element count must be 4")

	var out_data = OrtAdapters.get_tensor_data_float32(tensor)
	assert(out_data.size() == 4, "Output data size must match")
	assert(abs(out_data[0] - 1.5) < 1e-5, "Data[0] must match")
	assert(abs(out_data[1] - (-2.5)) < 1e-5, "Data[1] must match")
	assert(abs(out_data[2] - 3.25) < 1e-5, "Data[2] must match")
	assert(abs(out_data[3] - 4.0) < 1e-5, "Data[3] must match")

func test_int32_tensor() -> void:
	var shape := PackedInt64Array([2, 2])
	var data := PackedInt32Array([10, 20, 30, 40])
	var tensor = OrtAdapters.create_tensor_int32(data, shape)
	assert(tensor != null, "Int32 tensor creation must succeed")

	var out_data = OrtAdapters.get_tensor_data_int32(tensor)
	assert(out_data.size() == 4, "Output size must be 4")
	assert(out_data[0] == 10 and out_data[1] == 20 and out_data[2] == 30 and out_data[3] == 40, "Int32 data must match")

func test_int64_tensor() -> void:
	var shape := PackedInt64Array([3])
	var data := PackedInt64Array([100000000000, 200000000000, 300000000000])
	var tensor = OrtAdapters.create_tensor_int64(data, shape)
	assert(tensor != null, "Int64 tensor creation must succeed")

	var out_data = OrtAdapters.get_tensor_data_int64(tensor)
	assert(out_data.size() == 3, "Output size must be 3")
	assert(out_data[0] == 100000000000 and out_data[2] == 300000000000, "Int64 data must match")

func test_bytes_tensor() -> void:
	var shape := PackedInt64Array([1, 4])
	var data := PackedByteArray([1, 2, 3, 255])
	var tensor = OrtAdapters.create_tensor_bytes(data, shape, 2) # UINT8
	assert(tensor != null, "Byte tensor creation must succeed")

	var out_data = OrtAdapters.get_tensor_data_bytes(tensor)
	assert(out_data.size() == 4, "Output size must be 4")
	assert(out_data[3] == 255, "Byte data must match")
