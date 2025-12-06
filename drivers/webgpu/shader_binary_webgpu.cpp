#ifdef WEBGPU_ENABLED

#include "core/io/compression.h"
#include "core/io/marshalls.h"
#include "servers/rendering/rendering_device_commons.h"

#include "shader_binary_webgpu.h"

ShaderBinaryWebGpu::ShaderBinaryWebGpu(ShaderBinaryWebGpu::DataInput input) : input(input) {
}

ShaderBinaryWebGpu::~ShaderBinaryWebGpu() {
}

Vector<uint8_t> ShaderBinaryWebGpu::to_byte_array() {
	Vector<uint8_t> bytes;

	ERR_FAIL_COND_V(input.shader_name.length() != input.data.shader_name_len, Vector<uint8_t>());

	uint32_t shader_name_encoded_size = STEPIFY(input.shader_name.length(), 4);
	uint32_t bindings_encoded_size = 0;
	for (int set_idx = 0; set_idx < input.sets.size(); set_idx++) {
		const Vector<DataBindingInput> &bindings = input.sets[set_idx];
		bindings_encoded_size += sizeof(uint32_t);
		for (int binding_idx = 0; binding_idx < bindings.size(); binding_idx++) {
			const DataBindingInput &binding = bindings[binding_idx];
			bindings_encoded_size += sizeof(DataBinding);
			bindings_encoded_size += sizeof(uint32_t) * binding.corrections.size();
		}
	}

	uint32_t stages_encoded_size = 0;
	for (int i = 0; i < input.stages.size(); i++) {
		const ShaderStageInput &stage = input.stages[i];
		stages_encoded_size += 4 + 4 + 4;
		stages_encoded_size += STEPIFY(stage.source.size(), 4);
	}

	uint32_t expected_byte_count =
			4 + 4 + // HEADER and VERSION
			sizeof(Data) +
			shader_name_encoded_size +
			bindings_encoded_size +
			stages_encoded_size;
	bytes.resize_zeroed(expected_byte_count);

	uint8_t *binptr = bytes.ptrw();
	uint32_t offset = 0;
	// Good tool for checking
	uint32_t last_offset = 0;

	binptr[0] = 'G';
	binptr[1] = 'W';
	binptr[2] = 'G';
	binptr[3] = 'B';
	offset += 4;
	encode_uint32(VERSION, binptr + offset);
	offset += 4;

	memcpy(binptr + offset, &input.data, sizeof(Data));
	offset += sizeof(Data);

	memcpy(binptr + offset, input.shader_name.ptr(), input.data.shader_name_len);
	offset += shader_name_encoded_size;

	last_offset = offset;
	for (int i = 0; i < input.sets.size(); i++) {
		const Vector<DataBindingInput> &bindings = input.sets[i];
		encode_uint32(bindings.size(), binptr + offset);
		offset += sizeof(uint32_t);
		for (int i = 0; i < bindings.size(); i++) {
			const DataBindingInput &binding = bindings[i];
			uint32_t binding_size = sizeof(DataBinding);
			memcpy(binptr + offset, &binding.binding, binding_size);
			offset += binding_size;

			uint32_t corrections_size = sizeof(uint32_t) * binding.corrections.size();
			memcpy(binptr + offset, binding.corrections.ptr(), corrections_size);
			offset += corrections_size;
		}
	}
	ERR_FAIL_COND_V(offset - last_offset != bindings_encoded_size, Vector<uint8_t>());

	last_offset = offset;
	for (int i = 0; i < input.stages.size(); i++) {
		const ShaderStageInput &stage = input.stages[i];
		encode_uint32(stage.shader_stage, binptr + offset);
		offset += sizeof(uint32_t);
		encode_uint32(stage.original_source_size, binptr + offset);
		offset += sizeof(uint32_t);
		encode_uint32(stage.zstd_size, binptr + offset);
		offset += sizeof(uint32_t);
		memcpy(binptr + offset, stage.source.ptr(), stage.source.size());
		offset += STEPIFY(stage.source.size(), 4);
	}
	ERR_FAIL_COND_V(offset - last_offset != stages_encoded_size, Vector<uint8_t>());

	ERR_FAIL_COND_V(expected_byte_count != offset, Vector<uint8_t>());
	return bytes;
}

ShaderBinaryWebGpu::DataOutput ShaderBinaryWebGpu::parse_input_from_bytes(const Vector<uint8_t> &p_bytes) {
	DataInput result;

	const uint8_t *binptr = p_bytes.ptr();
	uint32_t binsize = p_bytes.size();
	uint32_t offset = 0;

	if (
			binsize < sizeof(uint32_t) * 2 + sizeof(Data) ||
			binptr[0] != 'G' ||
			binptr[1] != 'W' ||
			binptr[2] != 'G' ||
			binptr[3] != 'B' ||
			decode_uint32(binptr + 4) != VERSION) {
		return (DataOutput){
			.error = true
		};
	}
	offset += 4 + 4;

	memcpy(&result.data, binptr + offset, sizeof(Data));
	offset += sizeof(Data);

	if (result.data.shader_name_len != 0) {
		char *shader_name = (char *)memalloc(result.data.shader_name_len + 1);
		memcpy(shader_name, binptr + offset, result.data.shader_name_len);
		shader_name[result.data.shader_name_len] = '\0';
		offset += STEPIFY(result.data.shader_name_len, 4);
		result.shader_name = CharString(shader_name);
		memfree(shader_name);
	} else {
		result.shader_name = CharString("");
	}

	for (int set_idx = 0; set_idx < result.data.set_count; set_idx++) {
		Vector<DataBindingInput> bindings;
		uint32_t binding_count = decode_uint32(binptr + offset);
		offset += sizeof(uint32_t);

		for (int binding_idx = 0; binding_idx < binding_count; binding_idx++) {
			DataBindingInput binding;
			memcpy(&binding.binding, binptr + offset, sizeof(DataBinding));
			offset += sizeof(DataBinding);

			uint32_t corrections_size = binding.binding.correction_count * sizeof(uint32_t);
			binding.corrections.resize_zeroed(binding.binding.correction_count);
			memcpy(binding.corrections.ptrw(), binptr + offset, corrections_size);
			offset += corrections_size;

			bindings.push_back(binding);
		}
		result.sets.push_back(bindings);
	}

	for (int i = 0; i < result.data.stages_count; i++) {
		ShaderStageInput stage;
		stage.shader_stage = decode_uint32(binptr + offset);
		offset += sizeof(uint32_t);
		stage.original_source_size = decode_uint32(binptr + offset);
		offset += sizeof(uint32_t);
		stage.zstd_size = decode_uint32(binptr + offset);
		offset += sizeof(uint32_t);

		uint32_t source_size = stage.zstd_size ? stage.zstd_size : stage.original_source_size;
		stage.source.resize_zeroed(source_size);
		memcpy(stage.source.ptrw(), binptr + offset, source_size);
		offset += STEPIFY(source_size, 4);

		result.stages.push_back(stage);
	}

	ERR_FAIL_COND_V(offset != p_bytes.size(), (DataOutput){ .error = true });

	return (DataOutput){
		.error = false,
		.data = result,
	};
}

ShaderBinaryWebGpu::ShaderStageInput ShaderBinaryWebGpu::compress_source_into_input(const CharString &p_source, uint32_t shader_stages) {
	ShaderStageInput stage;
	stage.shader_stage = shader_stages;
	stage.original_source_size = p_source.size();
	Vector<uint8_t> zstd_bytes;
	zstd_bytes.resize(Compression::get_max_compressed_buffer_size(p_source.size(), Compression::MODE_ZSTD));
	int dst_size = Compression::compress(zstd_bytes.ptrw(), (uint8_t *)p_source.ptr(), p_source.size(), Compression::MODE_ZSTD);

	if (dst_size > 0 && (uint32_t)dst_size < p_source.size()) {
		stage.zstd_size = dst_size;
		stage.source = zstd_bytes;
		stage.source.resize(dst_size);
	} else {
		// Not using zstd.
		stage.zstd_size = 0;
		stage.source.resize(p_source.size());
		memcpy(stage.source.ptrw(), p_source.ptr(), p_source.size());
	}
	return stage;
}

Vector<uint8_t> ShaderBinaryWebGpu::decompress_source_with_input(const ShaderStageInput &input) {
	if (input.zstd_size > 0) {
		Vector<uint8_t> dec_bytes;
		dec_bytes.resize(input.original_source_size);
		int dec_size = Compression::decompress(dec_bytes.ptrw(), dec_bytes.size(), input.source.ptr(), input.zstd_size, Compression::MODE_ZSTD);
		ERR_FAIL_COND_V(dec_size != (int32_t)input.original_source_size, Vector<uint8_t>());
		return dec_bytes;
	} else {
		return input.source;
	}
}

#endif
