#ifdef WEBGPU_ENABLED

#include "rendering_shader_container_webgpu.h"

#include "core/error/error_macros.h"
#include "core/io/marshalls.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "drivers/webgpu/webgpu_translate.h"

#include <thirdparty/spirv-reflect/spirv_reflect.h>

#include <spirv_webgpu_transform.h>

#define DEBUG_SHADERS

#ifdef DEBUG_SHADERS
#define DEBUG_SHADERS_RAW_LOCATION "/tmp/shader"
#define DEBUG_SHADERS_POST_PATCH_LOCATION "/tmp/shader2"
#define DEBUG_SHADERS_POST_TRANSLATION_LOCATION "/tmp/wgsl"

#include <cstdio>

static void _debug_dump_shader(const char *p_dir, const String &p_shader_name, RenderingDeviceCommons::ShaderStage p_stage, const uint8_t *p_data, size_t p_size) {
	String safe = p_shader_name.is_empty() ? String("unnamed") : p_shader_name;
	safe = safe.replace_char(':', '_').replace_char('/', '_').replace_char(' ', '_');
	String stage_suffix;
	switch (p_stage) {
		case RenderingDeviceCommons::SHADER_STAGE_VERTEX:
			stage_suffix = "vert";
			break;
		case RenderingDeviceCommons::SHADER_STAGE_FRAGMENT:
			stage_suffix = "frag";
			break;
		case RenderingDeviceCommons::SHADER_STAGE_COMPUTE:
			stage_suffix = "comp";
			break;
		default:
			stage_suffix = String("stage") + itos((int)p_stage);
			break;
	}
	String path = String(p_dir) + "/" + safe + "_" + stage_suffix + ".spv";
	FILE *f = fopen(path.utf8().get_data(), "wb");
	if (!f) {
		print_line("[WGPU][DEBUG_SHADERS] failed to open ", path);
		return;
	}
	if (p_size > 0) {
		fwrite(p_data, 1, p_size, f);
	}
	fclose(f);
}
#endif

static WebGpuBindingHint webgpu_binding_hint_from_trans(const WebGpuTranslateBindingLayout &p_in) {
	WebGpuBindingHint out = {};
	switch (p_in.type) {
		case WebGpuTranslateBindingType::UNUSED:
			out.type = WebGpuBindingHintType::UNUSED;
			break;
		case WebGpuTranslateBindingType::SAMPLER:
			out.type = WebGpuBindingHintType::SAMPLER;
			out.sampler.sampler_type = (uint32_t)p_in._data.sampler.sampler_type;
			break;
		case WebGpuTranslateBindingType::TEXTURE:
			out.type = WebGpuBindingHintType::TEXTURE;
			out.texture.sample_type = (uint32_t)p_in._data.texture.sample_type;
			out.texture.multisampled = p_in._data.texture.multisampled ? 1u : 0u;
			break;
	}
	return out;
}

const uint32_t RenderingShaderContainerWebGpu::FORMAT_VERSION = 1;

uint32_t RenderingShaderContainerWebGpu::_format() const {
	// 'WGPU' LE
	return 0x55504757;
}

uint32_t RenderingShaderContainerWebGpu::_format_version() const {
	return FORMAT_VERSION;
}

static RenderingDeviceCommons::TextureType _texture_type_from_spv(SpvDim p_dim, bool p_arrayed) {
	using RDC = RenderingDeviceCommons;
	switch (p_dim) {
		case SpvDim1D:
			return p_arrayed ? RDC::TEXTURE_TYPE_1D_ARRAY : RDC::TEXTURE_TYPE_1D;
		case SpvDim2D:
		case SpvDimRect:
		case SpvDimSubpassData:
			return p_arrayed ? RDC::TEXTURE_TYPE_2D_ARRAY : RDC::TEXTURE_TYPE_2D;
		case SpvDim3D:
			return RDC::TEXTURE_TYPE_3D;
		case SpvDimCube:
			return p_arrayed ? RDC::TEXTURE_TYPE_CUBE_ARRAY : RDC::TEXTURE_TYPE_CUBE;
		case SpvDimBuffer:
			return RDC::TEXTURE_TYPE_2D;
		default:
			return RDC::TEXTURE_TYPE_2D;
	}
}

void RenderingShaderContainerWebGpu::_set_from_shader_reflection_post(const ReflectShader &p_shader) {
	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());
}

bool RenderingShaderContainerWebGpu::_set_code_from_spirv(const ReflectShader &p_shader) {
	const LocalVector<ReflectShaderStage> &p_spirv = p_shader.shader_stages;

	String shader_name_str = String::utf8(shader_name.ptr(), shader_name.length());

	// HACK: I will ignore these shaders until a better workaround is found.
	if (shader_name_str.contains("GiShader")) {
		ERR_FAIL_V_MSG(false, "Refusing to compile GiShader*");
	}
	// HACK: There is no way to create a binding layout for the `depth_buffer` uniform using reflection data.
	if (shader_name_str.contains("ClusterDebugShaderRD:0")) {
		ERR_FAIL_V_MSG(false, "Refusing to compile ClusterDebugShaderRD*");
	}
	// HACK: The hall of bad shaders:
	// if (shader_name_str.contains("CopyToFbShaderRD:0")) {
	// 	ERR_FAIL_V_MSG(false, "Refusing to compile CopyToFbShaderRD:0");
	// }
	if (shader_name_str.contains("BokehDofRasterShaderRD:0")) {
		ERR_FAIL_V_MSG(false, "Refusing to compile BokehDofRasterShaderRD:0");
	}
	if (shader_name_str.contains("CubeToDpShaderRD:0")) {
		ERR_FAIL_V_MSG(false, "Refusing to compile CubeToDpShaderRD:0");
	}

	struct PatchedStage {
		RDC::ShaderStage shader_stage;
		Vector<uint8_t> spirv;
	};
	Vector<PatchedStage> patched;
	Vector<SpvTransformCorrectionMap> correction_maps;
	patched.resize(p_spirv.size());
	correction_maps.resize(p_spirv.size());

	// The max_set + 1 is where we want to put the push constant emulation uniform.
	uint32_t max_set = 0;
	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		max_set = MAX(p_shader.uniform_sets.size() - 1, max_set);
	}

	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		Span<uint32_t> stage_spirv = p_spirv[i].spirv();
		Vector<uint32_t> in_spirv;
		in_spirv.resize(stage_spirv.size());
		memcpy(in_spirv.ptrw(), stage_spirv.ptr(), stage_spirv.size() * sizeof(uint32_t));

#ifdef DEBUG_SHADERS
		_debug_dump_shader(DEBUG_SHADERS_RAW_LOCATION, shader_name_str, p_spirv[i].shader_stage,
				(const uint8_t *)stage_spirv.ptr(), stage_spirv.size() * sizeof(uint32_t));
#endif

		SpvTransformCorrectionMap map = (SpvTransformCorrectionMap)SPIRV_WEBGPU_TRANSFORM_CORRECTION_MAP_NULL;

		// Ensure our push constant emulation uniform is at the max_set + 1, rather than detecting for each shader.
		spirv_webgpu_transform_correction_write_immediates_set(&map, max_set + 1);

		uint32_t *combimg_out_spv = nullptr;
		uint32_t combimg_out_count = 0;
		spirv_webgpu_transform_combimgsampsplitter_alloc(in_spirv.ptrw(), in_spirv.size(), &combimg_out_spv, &combimg_out_count, &map);

		uint32_t *dref_out_spv = nullptr;
		uint32_t dref_out_count = 0;
		spirv_webgpu_transform_drefsplitter_alloc(combimg_out_spv, combimg_out_count, &dref_out_spv, &dref_out_count, &map);

		uint32_t *isnanisinf_out_spv = nullptr;
		uint32_t isnanisinf_out_count = 0;
		spirv_webgpu_transform_isnanisinfpatch_alloc(dref_out_spv, dref_out_count, &isnanisinf_out_spv, &isnanisinf_out_count);

		uint32_t *storagecube_out_spv = nullptr;
		uint32_t storagecube_out_count = 0;
		spirv_webgpu_transform_storagecubepatch_alloc(isnanisinf_out_spv, isnanisinf_out_count, &storagecube_out_spv, &storagecube_out_count, &map);

		uint32_t *immediates_out_spv = nullptr;
		uint32_t immediates_out_count = 0;
		spirv_webgpu_transform_immediatespatch_alloc(storagecube_out_spv, storagecube_out_count, &immediates_out_spv, &immediates_out_count, &map);

		uint32_t *bindingarray_out_spv = nullptr;
		uint32_t bindingarray_out_count = 0;
		spirv_webgpu_transform_splitbindingarray_alloc(immediates_out_spv, immediates_out_count, &bindingarray_out_spv, &bindingarray_out_count, &map);

		uint32_t *pruneunuseddref_out_spv = nullptr;
		uint32_t pruneunuseddref_out_count = 0;
		spirv_webgpu_transform_pruneunuseddref_alloc(bindingarray_out_spv, bindingarray_out_count, &pruneunuseddref_out_spv, &pruneunuseddref_out_count);

		Vector<uint8_t> out_spirv;
		out_spirv.resize(pruneunuseddref_out_count * sizeof(uint32_t));
		memcpy(out_spirv.ptrw(), pruneunuseddref_out_spv, out_spirv.size());

		patched.write[i].shader_stage = p_spirv[i].shader_stage;
		patched.write[i].spirv = out_spirv;
		correction_maps.write[i] = map;

		spirv_webgpu_transform_combimgsampsplitter_free(combimg_out_spv);
		spirv_webgpu_transform_drefsplitter_free(dref_out_spv);
		spirv_webgpu_transform_isnanisinfpatch_free(isnanisinf_out_spv);
		spirv_webgpu_transform_storagecubepatch_free(storagecube_out_spv);
		spirv_webgpu_transform_immediatespatch_free(immediates_out_spv);
		spirv_webgpu_transform_splitbindingarray_free(bindingarray_out_spv);
		spirv_webgpu_transform_pruneunuseddref_free(pruneunuseddref_out_spv);
	}

	ERR_FAIL_COND_V_MSG(correction_maps.size() > 2, false, "Unexpected, more than 2 stages in one shader");
	SpvTransformCorrectionMap correction_map = (SpvTransformCorrectionMap)SPIRV_WEBGPU_TRANSFORM_CORRECTION_MAP_NULL;
	if (correction_maps.size() == 1) {
		correction_map = correction_maps[0];
	} else if (correction_maps.size() == 2) {
		SpvTransformCorrectionMap left_map = correction_maps[0];
		SpvTransformCorrectionMap right_map = correction_maps[1];

		Vector<uint8_t> &left_spirv = patched.write[0].spirv;
		Vector<uint8_t> &right_spirv = patched.write[1].spirv;

		uint32_t *out_left_spirv = nullptr;
		uint32_t out_left_spirv_count = 0;
		uint32_t *out_right_spirv = nullptr;
		uint32_t out_right_spirv_count = 0;

		spirv_webgpu_transform_mirrorpatch_alloc(
				(uint32_t *)left_spirv.ptr(), left_spirv.size() / sizeof(uint32_t), &left_map,
				(uint32_t *)right_spirv.ptr(), right_spirv.size() / sizeof(uint32_t), &right_map,
				&out_left_spirv, &out_left_spirv_count,
				&out_right_spirv, &out_right_spirv_count);

		left_spirv.resize(out_left_spirv_count * sizeof(uint32_t));
		memcpy(left_spirv.ptrw(), out_left_spirv, left_spirv.size());

		right_spirv.resize(out_right_spirv_count * sizeof(uint32_t));
		memcpy(right_spirv.ptrw(), out_right_spirv, right_spirv.size());

		// Right is always right! (Right is the fragment shader which is typically chonkier but this doesn't matter.)
		correction_map = right_map;

		spirv_webgpu_transform_mirrorpatch_free(out_left_spirv, out_right_spirv);
	}

	Vector<CharString> wgsl_sources;
	wgsl_sources.resize(patched.size());
	HashMap<uint32_t, HashMap<uint32_t, WebGpuTranslateBindingLayout>> merged_binding_hints;
	for (int i = 0; i < patched.size(); i++) {
		const Vector<uint8_t> &spv_bytes = patched[i].spirv;

#ifdef DEBUG_SHADERS
		_debug_dump_shader(DEBUG_SHADERS_POST_PATCH_LOCATION, shader_name_str, patched[i].shader_stage,
				spv_bytes.ptr(), (size_t)spv_bytes.size());
#endif

		ConvertResult result = webgpu_translate_spirv_to_wgsl((const uint32_t *)spv_bytes.ptr(), spv_bytes.size() / sizeof(uint32_t));
		if (result.error_string != nullptr) {
			print_line("[WGPU] WGSL compilation ", shader_name_str, "on step", (int)result.failure_stage, ":", result.error_string.ptr());
			return false;
		}

		wgsl_sources.write[i] = result.wgsl_string;

#ifdef DEBUG_SHADERS
		_debug_dump_shader(DEBUG_SHADERS_POST_TRANSLATION_LOCATION, shader_name_str, patched[i].shader_stage,
				(const uint8_t *)result.wgsl_string.ptr(), (size_t)result.wgsl_string.length());
#endif

		for (KeyValue<uint32_t, HashMap<uint32_t, WebGpuTranslateBindingLayout>> &set_kv : result.binding_hints) {
			if (!merged_binding_hints.has(set_kv.key)) {
				merged_binding_hints.insert(set_kv.key, HashMap<uint32_t, WebGpuTranslateBindingLayout>());
			}
			HashMap<uint32_t, WebGpuTranslateBindingLayout> &bindings = merged_binding_hints.get(set_kv.key);
			for (KeyValue<uint32_t, WebGpuTranslateBindingLayout> &binding_kv : set_kv.value) {
				if (bindings.has(binding_kv.key)) {
					const WebGpuTranslateBindingLayout &existing_binding = bindings.get(binding_kv.key);
					if (!webgpu_translate_compare_binding_layout(existing_binding, binding_kv.value)) {
						ERR_FAIL_V_MSG(false, vformat("Mismatched shader binding hints from binding (%d, %d) %s", set_kv.key, binding_kv.key, shader_name_str));
					}
				} else {
					bindings.insert(binding_kv.key, binding_kv.value);
				}
			}
		}
	}

	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());

	uint32_t global_idx = 0;
	for (uint32_t set_idx = 0; set_idx < p_shader.uniform_sets.size(); set_idx++) {
		const ReflectDescriptorSet &set_refl = p_shader.uniform_sets[set_idx];
		const HashMap<uint32_t, WebGpuTranslateBindingLayout> *hint_set = nullptr;
		if (merged_binding_hints.has(set_idx)) {
			hint_set = &merged_binding_hints.get(set_idx);
		}

		uint32_t binding_offset = 0;
		for (uint32_t binding_idx = 0; binding_idx < set_refl.size(); binding_idx++) {
			const ReflectUniform &uniform_refl = set_refl[binding_idx];
			UniformData &u = webgpu_uniform_data.write[global_idx];

			const SpvReflectDescriptorBinding &binding = uniform_refl.get_spv_reflect();
			u.image_format = (uint32_t)uniform_refl.image.format;
			u.texture_image_type = (uint32_t)_texture_type_from_spv((SpvDim)binding.image.dim, binding.image.arrayed != 0);
			u.texture_is_multisample = binding.image.ms != 0 ? 1u : 0u;
			u.texture_sample_type = (uint32_t)RDC::ShaderUniform::TextureSampleType::Float;

			bool non_writable = (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) ||
					(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
			bool non_readable = (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE) ||
					(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE);
			if (!non_writable && !non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadWrite;
			} else if (non_writable && !non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadOnly;
			} else if (!non_writable && non_readable) {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::WriteOnly;
			} else {
				u.image_access = (uint32_t)RDC::ShaderUniform::ImageAccess::ReadOnly;
			}

			uint32_t base_post = uniform_refl.binding + binding_offset;
			if (hint_set && hint_set->has(base_post)) {
				u.base_hint = webgpu_binding_hint_from_trans(hint_set->get(base_post));
			} else {
				u.base_hint = WebGpuBindingHint();
			}

			u.corrections.clear();
			u.binding_hints.clear();
			if (correction_map != (SpvTransformCorrectionMap)SPIRV_WEBGPU_TRANSFORM_CORRECTION_MAP_NULL) {
				uint16_t *corrections_raw = nullptr;
				uint32_t correction_count = 0;
				uint8_t status = spirv_webgpu_transform_correction_sets_index(
						correction_map, set_idx, uniform_refl.binding, &corrections_raw, &correction_count);
				if (status != 0) {
					for (uint32_t k = 0; k < correction_count; k++) {
						u.corrections.push_back((uint32_t)corrections_raw[k]);
					}
				}
			}
			for (int k = 0; k < u.corrections.size(); k++) {
				binding_offset += 1;
				uint32_t corr_post = uniform_refl.binding + binding_offset;
				WebGpuBindingHint hint;
				if (hint_set && hint_set->has(corr_post)) {
					hint = webgpu_binding_hint_from_trans(hint_set->get(corr_post));
				}
				u.binding_hints.push_back(hint);
			}

			global_idx++;
		}
	}

	for (int i = 0; i < correction_maps.size(); i++) {
		spirv_webgpu_transform_correction_map_free(correction_maps[i]);
	}

	shaders.resize(wgsl_sources.size());
	for (int i = 0; i < wgsl_sources.size(); i++) {
		const CharString &source = wgsl_sources[i];
		Shader &shader = shaders.write[i];
		shader.shader_stage = patched[i].shader_stage;

		uint32_t source_size = (uint32_t)source.length();
		shader.code_decompressed_size = source_size;
		shader.code_compressed_bytes.resize(source_size);

		uint32_t compressed_size = 0;
		bool compressed = compress_code(
				(const uint8_t *)source.ptr(), source_size,
				shader.code_compressed_bytes.ptrw(), &compressed_size, &shader.code_compression_flags);
		ERR_FAIL_COND_V_MSG(!compressed, false, vformat("Failed to compress WGSL for SPIR-V stage #%d.", i));

		shader.code_compressed_bytes.resize(compressed_size);
	}

	return true;
}

// Size of the non-arrayed items we store in container extra data.
static constexpr uint32_t WEBGPU_UNIFORM_EXTRA_BASE_SIZE =
		sizeof(WebGpuBindingHint) + 5 * sizeof(uint32_t);

uint32_t RenderingShaderContainerWebGpu::_to_bytes_reflection_binding_uniform_extra_data(uint8_t *p_bytes, uint32_t p_index) const {
	const UniformData &u = webgpu_uniform_data[p_index];
	uint32_t correction_count = (uint32_t)u.corrections.size();
	uint32_t total = WEBGPU_UNIFORM_EXTRA_BASE_SIZE + sizeof(uint32_t) + correction_count * sizeof(uint32_t) + correction_count * sizeof(WebGpuBindingHint);
	if (p_bytes == nullptr) {
		return total;
	}

	uint8_t *p = p_bytes;
	memcpy(p, &u.base_hint, sizeof(WebGpuBindingHint));
	p += sizeof(WebGpuBindingHint);
	encode_uint32(u.image_format, p);
	p += sizeof(uint32_t);
	encode_uint32(u.image_access, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_image_type, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_sample_type, p);
	p += sizeof(uint32_t);
	encode_uint32(u.texture_is_multisample, p);
	p += sizeof(uint32_t);
	encode_uint32(correction_count, p);
	p += sizeof(uint32_t);

	for (uint32_t k = 0; k < correction_count; k++) {
		encode_uint32(u.corrections[k], p);
		p += sizeof(uint32_t);
	}
	for (uint32_t k = 0; k < correction_count; k++) {
		memcpy(p, &u.binding_hints[k], sizeof(WebGpuBindingHint));
		p += sizeof(WebGpuBindingHint);
	}

	return total;
}

uint32_t RenderingShaderContainerWebGpu::_from_bytes_reflection_binding_uniform_extra_data_start(const uint8_t *p_bytes) {
	webgpu_uniform_data.resize(reflection_binding_set_uniforms_data.size());
	return 0;
}

uint32_t RenderingShaderContainerWebGpu::_from_bytes_reflection_binding_uniform_extra_data(const uint8_t *p_bytes, uint32_t p_index) {
	UniformData &u = webgpu_uniform_data.write[p_index];

	const uint8_t *p = p_bytes;
	memcpy(&u.base_hint, p, sizeof(WebGpuBindingHint));
	p += sizeof(WebGpuBindingHint);
	u.image_format = decode_uint32(p);
	p += sizeof(uint32_t);
	u.image_access = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_image_type = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_sample_type = decode_uint32(p);
	p += sizeof(uint32_t);
	u.texture_is_multisample = decode_uint32(p);
	p += sizeof(uint32_t);
	uint32_t correction_count = decode_uint32(p);
	p += sizeof(uint32_t);

	u.corrections.resize(correction_count);
	for (uint32_t k = 0; k < correction_count; k++) {
		u.corrections.write[k] = decode_uint32(p);
		p += sizeof(uint32_t);
	}
	u.binding_hints.resize(correction_count);
	for (uint32_t k = 0; k < correction_count; k++) {
		memcpy(&u.binding_hints.write[k], p, sizeof(WebGpuBindingHint));
		p += sizeof(WebGpuBindingHint);
	}

	return WEBGPU_UNIFORM_EXTRA_BASE_SIZE + sizeof(uint32_t) + correction_count * (sizeof(uint32_t) + sizeof(WebGpuBindingHint));
}

RenderingShaderContainerWebGpu::RenderingShaderContainerWebGpu() {}

Ref<RenderingShaderContainer> RenderingShaderContainerFormatWebGpu::create_container() const {
	return memnew(RenderingShaderContainerWebGpu);
}

RenderingDeviceCommons::ShaderLanguageVersion RenderingShaderContainerFormatWebGpu::get_shader_language_version() const {
	return SHADER_LANGUAGE_VULKAN_VERSION_1_1;
}

RenderingDeviceCommons::ShaderSpirvVersion RenderingShaderContainerFormatWebGpu::get_shader_spirv_version() const {
	return SHADER_SPIRV_VERSION_1_3;
}

RenderingShaderContainerFormatWebGpu::RenderingShaderContainerFormatWebGpu() {}
RenderingShaderContainerFormatWebGpu::~RenderingShaderContainerFormatWebGpu() {}

#endif // WEBGPU_ENABLED
