#include "webgpu_trans.h"
#include "naga.h"

inline WebGpuTranslateBindingLayout interpret_binding_type(const NagaType *types, const NagaType &type) {
	switch (type.inner.tag) {
		case NagaTypeInnerTag_Sampler: {
			WGPUSamplerBindingType sampler_type;
			if (type.inner.data.sampler.comparison) {
				sampler_type = WGPUSamplerBindingType_Comparison;
			} else {
				sampler_type = WGPUSamplerBindingType_Filtering;
			}

			return (WebGpuTranslateBindingLayout){
				.type = WebGpuTranslateBindingType::SAMPLER,
				._data = {
						.sampler = (WebGpuTranslateBindingSamplerLayout){
								.sampler_type = sampler_type },

				}
			};
		} break;
		case NagaTypeInnerTag_Image: {
			NagaImageClass class_ = type.inner.data.image.class_;
			switch (class_.tag) {
				case NagaImageClassTag_Sampled: {
					WGPUTextureSampleType sample_type;
					switch (class_.data.sampled.kind) {
						case NagaScalarKind_Sint:
							sample_type = WGPUTextureSampleType_Sint;
							break;
						case NagaScalarKind_Uint:
							sample_type = WGPUTextureSampleType_Uint;
							break;
						case NagaScalarKind_Float:
							sample_type = WGPUTextureSampleType_Float;
							break;
						// TODO: Investigate whether other scalars and what to do if not.
						default:
							sample_type = WGPUTextureSampleType_Sint;
							break;
					}
					return (WebGpuTranslateBindingLayout){
						.type = WebGpuTranslateBindingType::TEXTURE,
						._data = {
								.texture = (WebGpuTranslateBindingTextureLayout){
										.sample_type =
												sample_type,
										.multisampled =
												(bool)class_.data.sampled.multi },
						}
					};
				} break;
				case NagaImageClassTag_Depth:
					return (WebGpuTranslateBindingLayout){
						.type = WebGpuTranslateBindingType::TEXTURE,
						._data = {
								.texture = (WebGpuTranslateBindingTextureLayout){
										.sample_type =
												WGPUTextureSampleType_Depth,
										.multisampled =
												(bool)class_.data.depth.multi },

						}
					};
					break;
				default:
					break;
			}
		} break;
		case NagaTypeInnerTag_BindingArray:
			return interpret_binding_type(types, types[type.inner.data.binding_array.base]);
			break;
		default:
			break;
	}
	return (WebGpuTranslateBindingLayout){
		.type = WebGpuTranslateBindingType::UNUSED
	};
}

ConvertResult webgpu_translate_spirv_to_wgsl(const uint32_t *spv, uint32_t spv_count) {
	uint8_t success;
	WebGpuTranslateFailureStage stage = WebGpuTranslateFailureStage::FRONT;

	NagaCapabilitiesFlags caps =
			NagaCapabilities::NagaCapabilities_MULTISAMPLED_SHADING |
			NagaCapabilities::NagaCapabilities_CUBE_ARRAY_TEXTURES |
			NagaCapabilities::NagaCapabilities_IMMEDIATES |
			NagaCapabilities::NagaCapabilities_STORAGE_TEXTURE_16BIT_NORM_FORMATS |
			NagaCapabilities::NagaCapabilities_SHADER_FLOAT16_IN_FLOAT32 |
			NagaCapabilities::NagaCapabilities_TEXTURE_AND_SAMPLER_BINDING_ARRAY |
			NagaCapabilities::NagaCapabilities_TEXTURE_AND_SAMPLER_BINDING_ARRAY_NON_UNIFORM_INDEXING |
			NagaCapabilities::NagaCapabilities_STORAGE_TEXTURE_BINDING_ARRAY |
			NagaCapabilities::NagaCapabilities_STORAGE_TEXTURE_BINDING_ARRAY_NON_UNIFORM_INDEXING |
			NagaCapabilities::NagaCapabilities_SUBGROUP;
	NagaModuleFillFlags fill_flags = NAGA_FLAGS_ALL(NagaModuleFillFlags);

	NagaSPVFrontOptions options = (NagaSPVFrontOptions){
		.adjust_coordinate_space = true,
		.strict_capabilities = true,
		.block_ctx_dump_prefix = nullptr,
	};
	NagaSPVFrontResult front_result;
	front_result.flags = NagaFrontResultOption_FormattedErrorOnly;
	success = naga_front_spv_parse(options, spv, spv_count, fill_flags, &front_result);
	if (!success) {
		return (ConvertResult){
			.wgsl_string = nullptr,
			.error_string = strdup(front_result.fmt_error),
		};
	}

	stage = WebGpuTranslateFailureStage::VALID;
	NagaValidator validator = naga_valid_validator_new(NAGA_FLAGS_ALL(NagaValidationFlagsFlags), caps);
	NagaValidateResult valid_result;
	valid_result.flags = NagaValidateResultOption_FormattedErrorOnly;
	success = naga_valid_validator_validate(&validator, &front_result.module, &valid_result);
	if (!success) {
		return (ConvertResult){
			.wgsl_string = nullptr,
			.error_string = strdup(valid_result.fmt_error),
		};
	}

	HashMap<uint32_t, HashMap<uint32_t, WebGpuTranslateBindingLayout>> binding_hints;
	for (uint32_t i = 0; i < front_result.module.global_variables_len; i++) {
		NagaGlobalVariable &variable = front_result.module.global_variables[i];
		if (variable.binding.some) {
			uint32_t type_idx = variable.ty;
			NagaType &ty = front_result.module.types[type_idx];
			uint32_t set = variable.binding.value.group;
			uint32_t binding = variable.binding.value.binding;
			WebGpuTranslateBindingLayout hint = interpret_binding_type(front_result.module.types, ty);

			if (hint.type != WebGpuTranslateBindingType::UNUSED) {
				if (!binding_hints.has(set)) {
					binding_hints.insert(set, HashMap<uint32_t, WebGpuTranslateBindingLayout>());
				}
				HashMap<uint32_t, WebGpuTranslateBindingLayout> &bindings = binding_hints.get(set);
				bindings.insert(binding, hint);
			}
		}
	}

	// Compaction does not work for many of our shaders?
	//
	// stage = ConvertFailureStage::COMPACT;
	// naga_compact_compact(&front_result.module, KeepUnused_No);

	stage = WebGpuTranslateFailureStage::BACK;
	NagaWGSLBackWriterFlagsFlags writer_flags = NAGA_FLAGS_EMPTY(NagaWGSLBackWriterFlagsFlags);
	NagaWGSLWriteResult back_result;
	back_result.flags = NagaWriteResultOption_FormattedErrorOnly;
	success = naga_back_wgsl_write(&front_result.module, &valid_result.module_info, writer_flags, &back_result);
	if (!success) {
		return (ConvertResult){
			.wgsl_string = nullptr,
			.error_string = back_result.fmt_error,
			.failure_stage = stage,
		};
	}
	// TODO: Free Memory

	return (ConvertResult){
		.wgsl_string = back_result.output,
		.binding_hints = binding_hints,
		.error_string = nullptr,
		.failure_stage = WebGpuTranslateFailureStage::NONE
	};
}

bool webgpu_translate_compare_binding_layout(const WebGpuTranslateBindingLayout &a, const WebGpuTranslateBindingLayout &b) {
	if (a.type == b.type) {
		switch (a.type) {
			case WebGpuTranslateBindingType::UNUSED:
				return true;
			case WebGpuTranslateBindingType::SAMPLER:
				return a._data.sampler.sampler_type == b._data.sampler.sampler_type;
			case WebGpuTranslateBindingType::TEXTURE:
				return a._data.texture.sample_type == b._data.texture.sample_type && a._data.texture.multisampled == b._data.texture.multisampled;
		}
	} else {
		return false;
	}
}
