#include "webgpu_conv.h"
#include "webgpu.h"

WGPUBufferUsage webgpu_buffer_usage_from_rd(BitField<RDD::BufferUsageBits> p_buffer_usage) {
	uint32_t ret = 0;
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_FROM_BIT) {
		ret |= WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_TO_BIT) {
		ret = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TEXEL_BIT) {
		ret = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_UNIFORM_BIT) {
		ret = WGPUBufferUsage_Uniform;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_STORAGE_BIT) {
		ret = WGPUBufferUsage_Storage;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDEX_BIT) {
		ret = WGPUBufferUsage_Index;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_VERTEX_BIT) {
		ret = WGPUBufferUsage_Vertex;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDIRECT_BIT) {
		ret = WGPUBufferUsage_Indirect;
	}
	return (WGPUBufferUsage)ret;
}

WGPUTextureFormat webgpu_texture_format_from_rd(RDD::DataFormat p_data_format) {
	WGPUTextureFormat ret = WGPUTextureFormat_Undefined;

	// See https://www.w3.org/TR/webgpu/#enumdef-gputextureformat
	// TODO: The BC, ETC2, and ASTC compressed formats have been left out.
	switch (p_data_format) {
		case RDD::DataFormat::DATA_FORMAT_R8_UNORM:
			ret = WGPUTextureFormat_R8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_SNORM:
			ret = WGPUTextureFormat_R8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_UINT:
			ret = WGPUTextureFormat_R8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_SINT:
			ret = WGPUTextureFormat_R8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_UINT:
			ret = WGPUTextureFormat_R16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SINT:
			ret = WGPUTextureFormat_R16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SFLOAT:
			ret = WGPUTextureFormat_R16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_UINT:
			ret = WGPUTextureFormat_R32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SINT:
			ret = WGPUTextureFormat_R32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SFLOAT:
			ret = WGPUTextureFormat_R32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_UNORM:
			ret = WGPUTextureFormat_RG8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_SNORM:
			ret = WGPUTextureFormat_RG8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_UINT:
			ret = WGPUTextureFormat_RG8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_SINT:
			ret = WGPUTextureFormat_RG8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_UINT:
			ret = WGPUTextureFormat_RG16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SINT:
			ret = WGPUTextureFormat_RG16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SFLOAT:
			ret = WGPUTextureFormat_RG16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_UNORM:
			ret = WGPUTextureFormat_RGBA8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SRGB:
			ret = WGPUTextureFormat_RGBA8UnormSrgb;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SNORM:
			ret = WGPUTextureFormat_RGBA8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_UINT:
			ret = WGPUTextureFormat_RGBA8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SINT:
			ret = WGPUTextureFormat_RGBA8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_B8G8R8A8_UNORM:
			ret = WGPUTextureFormat_BGRA8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_B8G8R8A8_SRGB:
			ret = WGPUTextureFormat_BGRA8UnormSrgb;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_UINT:
			ret = WGPUTextureFormat_RG32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SINT:
			ret = WGPUTextureFormat_RG32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SFLOAT:
			ret = WGPUTextureFormat_RG32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_UINT:
			ret = WGPUTextureFormat_RGBA16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SINT:
			ret = WGPUTextureFormat_RGBA16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SFLOAT:
			ret = WGPUTextureFormat_RGBA16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_UINT:
			ret = WGPUTextureFormat_RGBA32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SINT:
			ret = WGPUTextureFormat_RGBA32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SFLOAT:
			ret = WGPUTextureFormat_RGBA32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_S8_UINT:
			ret = WGPUTextureFormat_Stencil8;
			break;
		case RDD::DataFormat::DATA_FORMAT_D16_UNORM:
			ret = WGPUTextureFormat_Depth16Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_D24_UNORM_S8_UINT:
			ret = WGPUTextureFormat_Depth24PlusStencil8;
			break;
		case RDD::DataFormat::DATA_FORMAT_D32_SFLOAT:
			ret = WGPUTextureFormat_Depth32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_D32_SFLOAT_S8_UINT:
			ret = WGPUTextureFormat_Depth32FloatStencil8;
			break;
		default:
			break;
	}

	return ret;
}

WGPUFilterMode webgpu_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
	static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST, WGPUFilterMode_Nearest));
	static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR, WGPUFilterMode_Linear));
	return (WGPUFilterMode)p_sampler_filter;
}

WGPUMipmapFilterMode webgpu_mipmap_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
	static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST, WGPUMipmapFilterMode_Nearest));
	static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR, WGPUMipmapFilterMode_Linear));
	return (WGPUMipmapFilterMode)p_sampler_filter;
}

WGPUAddressMode webgpu_address_mode_from_rd(RDD::SamplerRepeatMode p_sampler_repeat_mode) {
	// Not all of these exist, so we'll default to clamp to edge.
	// See https://gpuweb.github.io/gpuweb/#enumdef-gpuaddressmode
	switch (p_sampler_repeat_mode) {
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_REPEAT:
			return WGPUAddressMode_Repeat;
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT:
			return WGPUAddressMode_MirrorRepeat;
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE:
			return WGPUAddressMode_ClampToEdge;
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_CLAMP_TO_BORDER:
			return WGPUAddressMode_ClampToEdge;
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_MIRROR_CLAMP_TO_EDGE:
			return WGPUAddressMode_ClampToEdge;
		case RenderingDeviceCommons::SAMPLER_REPEAT_MODE_MAX:
			return WGPUAddressMode_ClampToEdge;
	}
}

WGPUCompareFunction webgpu_compare_mode_from_rd(RDD::CompareOperator p_compare_operator) {
	switch (p_compare_operator) {
		case RenderingDeviceCommons::COMPARE_OP_NEVER:
			return WGPUCompareFunction_Never;
		case RenderingDeviceCommons::COMPARE_OP_LESS:
			return WGPUCompareFunction_Less;
		case RenderingDeviceCommons::COMPARE_OP_EQUAL:
			return WGPUCompareFunction_Equal;
		case RenderingDeviceCommons::COMPARE_OP_LESS_OR_EQUAL:
			return WGPUCompareFunction_LessEqual;
		case RenderingDeviceCommons::COMPARE_OP_GREATER:
			return WGPUCompareFunction_Greater;
		case RenderingDeviceCommons::COMPARE_OP_NOT_EQUAL:
			return WGPUCompareFunction_NotEqual;
		case RenderingDeviceCommons::COMPARE_OP_GREATER_OR_EQUAL:
			return WGPUCompareFunction_GreaterEqual;
		case RenderingDeviceCommons::COMPARE_OP_ALWAYS:
			return WGPUCompareFunction_Always;
		case RenderingDeviceCommons::COMPARE_OP_MAX:
			return WGPUCompareFunction_Undefined;
	}
}

uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPUSupportedLimits p_limits) {
	WGPULimits limits = p_limits.limits;
	// Note: For limits that aren't supported, I've put the max uint64 value. This may cause issues.
	switch (p_selected_limit) {
		case RenderingDeviceCommons::LIMIT_MAX_BOUND_UNIFORM_SETS:
			return limits.maxBindGroups;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS:
			return limits.maxColorAttachments;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURES_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_SAMPLERS_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_BUFFERS_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_IMAGES_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFERS_PER_UNIFORM_SET:
			return limits.maxUniformBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_DRAW_INDEXED_INDEX:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_HEIGHT:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_WIDTH:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_ARRAY_LAYERS:
			return limits.maxTextureArrayLayers;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_1D:
			return limits.maxTextureDimension1D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_2D:
			return limits.maxTextureDimension2D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_3D:
			return limits.maxTextureDimension3D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_CUBE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURES_PER_SHADER_STAGE:
			return limits.maxSampledTexturesPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_SAMPLERS_PER_SHADER_STAGE:
			return limits.maxSamplersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE:
			return limits.maxStorageBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_IMAGES_PER_SHADER_STAGE:
			return limits.maxStorageTexturesPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE:
			return limits.maxUniformBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_PUSH_CONSTANT_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFER_SIZE:
			return limits.maxUniformBufferBindingSize;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_ATTRIBUTE_OFFSET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_ATTRIBUTES:
			return limits.maxVertexAttributes;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_BINDINGS:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_BINDING_STRIDE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
			return limits.minUniformBufferOffsetAlignment;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_SHARED_MEMORY_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_INVOCATIONS:
			return limits.maxComputeInvocationsPerWorkgroup;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_X:
			return limits.maxComputeWorkgroupSizeX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Y:
			return limits.maxComputeWorkgroupSizeY;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Z:
			return limits.maxComputeWorkgroupSizeZ;
		case RenderingDeviceCommons::LIMIT_MAX_VIEWPORT_DIMENSIONS_X:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_VIEWPORT_DIMENSIONS_Y:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_SUBGROUP_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_SUBGROUP_MIN_SIZE:
			return 0;
		case RenderingDeviceCommons::LIMIT_SUBGROUP_MAX_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_SUBGROUP_IN_SHADERS:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_SUBGROUP_OPERATIONS:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_VRS_TEXEL_WIDTH:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_VRS_TEXEL_HEIGHT:
			return UINT64_MAX;
	}
}
