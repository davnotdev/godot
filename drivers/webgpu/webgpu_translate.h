#ifndef WEBGPU_TRANSLATE_H
#define WEBGPU_TRANSLATE_H

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"

#include "webgpu.h"

#include <cstdint>

enum class WebGpuTranslateFailureStage {
	NONE = 0,
	FRONT = 1,
	VALID = 2,
	COMPACT = 3,
	BACK = 4,
};

struct WebGpuTranslateBindingSamplerLayout {
	WGPUSamplerBindingType sampler_type;
};

struct WebGpuTranslateBindingTextureLayout {
	WGPUTextureSampleType sample_type;
	bool multisampled;
};

enum class WebGpuTranslateBindingType {
	UNUSED,
	SAMPLER,
	TEXTURE,
};

struct WebGpuTranslateBindingLayout {
	WebGpuTranslateBindingType type;
	union {
		WebGpuTranslateBindingSamplerLayout sampler;
		WebGpuTranslateBindingTextureLayout texture;
	} _data;
};

struct ConvertResult {
	CharString wgsl_string;
	HashMap<uint32_t, HashMap<uint32_t, WebGpuTranslateBindingLayout>> binding_hints;

	CharString error_string;
	WebGpuTranslateFailureStage failure_stage;
};

ConvertResult webgpu_translate_spirv_to_wgsl(const uint32_t *spv, uint32_t spv_count);
bool webgpu_translate_compare_binding_layout(const WebGpuTranslateBindingLayout &a, const WebGpuTranslateBindingLayout &b);

#endif
