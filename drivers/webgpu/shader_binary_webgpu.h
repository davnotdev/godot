#ifndef SHADER_BINARY_WEBGPU_H
#define SHADER_BINARY_WEBGPU_H

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"

class ShaderBinaryWebGpu {
	//	An overview of the shader binary layout:
	//	```C
	//	struct {
	//		uint32_t header = b'GWGB';
	//		uint32_t version;
	//		Data data;
	//		// (aligned)
	//		char shader_name[data.shader_name_len];
	//		struct {
	//			uint32_t binding_count;
	//			struct {
	//				DataBinding binding;
	//				struct {
	//					CorrectionStage stage;
	//					uint32_t corrections[stage.corrections_count];
	//				} correction_stage_map[bindings.correction_stages_count]
	//			} bindings[binding_count];
	//		} sets[data.set_count]
	//		struct {
	//			uint32_t shader_stage;
	//			uint32_t original_source_size;
	//			uint32_t zstd_size;
	//			// (aligned)
	//			char source[zstd_size ? zstd_size : original_source_size];
	//		} stages[data.stages_count];
	//	};
	//	```

public:
	// Version 1: initial.
	static const uint32_t VERSION = 1;

	struct Data {
		uint64_t vertex_input_mask = 0;
		uint32_t fragment_output_mask = 0;
		uint32_t is_compute = 0;
		uint32_t compute_local_size[3] = {};
		uint32_t push_constant_size = 0;
		uint32_t push_constant_stages = 0; // WebGPU stage flags

		uint32_t shader_name_len = 0;
		uint32_t set_count = 0;
		uint32_t stages_count = 0;
	};

	struct DataBinding {
		uint32_t type = 0;
		uint32_t binding = 0;
		uint32_t stages = 0;
		uint32_t length = 0; // Size of arrays (in total elements), or UBOs (in bytes * total elements).
		uint32_t writable = 0;

		uint32_t image_format = 0;
		uint32_t image_access = 0;
		uint32_t texture_image_type = 0;
		uint32_t texture_sample_type = 0;
		uint32_t texture_is_multisample = 0;

		uint32_t correction_stages_count;
	};

	struct CorrectionStage {
		uint32_t stage;
		uint32_t corrections_count;
	};

public:
	struct DataBindingInput {
		DataBinding binding;
		HashMap<uint32_t, Vector<uint32_t>> correction_stage_map;
	};

	struct ShaderStageInput {
		uint32_t shader_stage;
		uint32_t original_source_size;
		uint32_t zstd_size;
		Vector<uint8_t> source;
	};

	struct DataInput {
		Data data;
		CharString shader_name;
		Vector<Vector<DataBindingInput>> sets;
		Vector<ShaderStageInput> stages;
	};

	struct DataOutput {
		bool error = false;
		DataInput data;
	};

	ShaderBinaryWebGpu(DataInput input);
	~ShaderBinaryWebGpu();

	Vector<uint8_t> to_byte_array();
	static DataOutput parse_input_from_bytes(const Vector<uint8_t> &p_bytes);
	static ShaderStageInput compress_source_into_input(const CharString &p_source, uint32_t shader_stages);
	static Vector<uint8_t> decompress_source_with_input(const ShaderStageInput &input);

private:
	DataInput input;
};

#endif
