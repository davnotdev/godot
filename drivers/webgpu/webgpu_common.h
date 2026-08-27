#ifndef WEBGPU_COMMON_H
#define WEBGPU_COMMON_H

#include <cstdint>

enum class WebGpuBindingHintType {
	UNUSED = 0,
	SAMPLER = 2,
	TEXTURE = 3,
};

struct WebGpuBindingSamplerHint {
	uint32_t sampler_type = 0;
};

struct WebGpuBindingTextureHint {
	uint32_t sample_type = 0;
	uint32_t multisampled = 0;
};

// NOTE: This doesn't need to be here if it turns out that we don't need to store these in `ShaderInfo`
// I don't think we do, but we do store them right now for debugging.
struct WebGpuBindingHint {
	WebGpuBindingHintType type = WebGpuBindingHintType::UNUSED;
	union {
		WebGpuBindingSamplerHint sampler;
		WebGpuBindingTextureHint texture;
	};
	WebGpuBindingHint() {}
};

#define WEBGPU_MAX_BIND_GROUPS 4
// This should just be 128 up to minUniformBufferOffsetAlignment.
#define WEBGPU_MAX_IMMEDIATE_SIZE 256

#define WEBGPU_PUSH_CONSTANT_EMULATION_BUFFER_ENTRIES 1028

#endif
