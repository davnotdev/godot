#include "webgpu_conv.h"
#include "webgpu.h"

// TODO: This is currently only used for R16_UNORM and R16_SNORM from https://github.com/davnotdev/wgpu-native/tree/godot-webgpu
#include <wgpu.h>

WGPUBufferUsage webgpu_buffer_usage_from_rd(BitField<RDD::BufferUsageBits> p_buffer_usage) {
	uint32_t ret = 0;
	// Only use MapWrite or MapRead if these are CPU buffers.
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_FROM_BIT) {
		ret |= WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_TO_BIT) {
		ret |= WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TEXEL_BIT) {
		ret |= WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_UNIFORM_BIT) {
		ret |= WGPUBufferUsage_Uniform;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_STORAGE_BIT) {
		ret |= WGPUBufferUsage_Storage;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDEX_BIT) {
		ret |= WGPUBufferUsage_Index;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_VERTEX_BIT) {
		ret |= WGPUBufferUsage_Vertex;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDIRECT_BIT) {
		ret |= WGPUBufferUsage_Indirect;
	}
	return (WGPUBufferUsage)ret;
}

WGPUTextureFormat webgpu_texture_format_from_rd(RDD::DataFormat p_data_format) {
	WGPUTextureFormat ret = WGPUTextureFormat_Undefined;

	// See https://www.w3.org/TR/webgpu/#enumdef-gputextureformat
	// TODO: The BC, ETC2, and ASTC compressed formats have been left out.
	// NOTE: Please also update `webgpu_bytes_per_row_from_format` alongside this.
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
		case RDD::DataFormat::DATA_FORMAT_R16_UNORM:
			ret = (WGPUTextureFormat)WGPUTextureFormatExtras_R16Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SNORM:
			ret = (WGPUTextureFormat)WGPUTextureFormatExtras_R16Snorm;
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

static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST, WGPUFilterMode_Nearest));
static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR, WGPUFilterMode_Linear));
WGPUFilterMode webgpu_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
	return (WGPUFilterMode)p_sampler_filter;
}

static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST, WGPUMipmapFilterMode_Nearest));
static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR, WGPUMipmapFilterMode_Linear));
WGPUMipmapFilterMode webgpu_mipmap_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
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

WGPUVertexFormat webgpu_vertex_format_from_rd(RDD::DataFormat p_data_format) {
	WGPUVertexFormat ret = WGPUVertexFormat_Undefined;

	switch (p_data_format) {
		case RDD::DataFormat::DATA_FORMAT_R32_UINT:
			ret = WGPUVertexFormat_Uint32;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SINT:
			ret = WGPUVertexFormat_Sint32;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SFLOAT:
			ret = WGPUVertexFormat_Float32;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_UINT:
			ret = WGPUVertexFormat_Uint8x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_SINT:
			ret = WGPUVertexFormat_Sint8x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_UINT:
			ret = WGPUVertexFormat_Uint16x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SINT:
			ret = WGPUVertexFormat_Sint16x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SFLOAT:
			ret = WGPUVertexFormat_Float16x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_UINT:
			ret = WGPUVertexFormat_Uint8x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SINT:
			ret = WGPUVertexFormat_Sint8x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_UINT:
			ret = WGPUVertexFormat_Uint32x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SINT:
			ret = WGPUVertexFormat_Sint32x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SFLOAT:
			ret = WGPUVertexFormat_Float32x2;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_UINT:
			ret = WGPUVertexFormat_Uint16x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SINT:
			ret = WGPUVertexFormat_Sint16x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SFLOAT:
			ret = WGPUVertexFormat_Float16x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_UINT:
			ret = WGPUVertexFormat_Uint32x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SINT:
			ret = WGPUVertexFormat_Sint32x4;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SFLOAT:
			ret = WGPUVertexFormat_Float32x4;
			break;
		default:
			break;
	}

	return ret;
}

WGPULoadOp webgpu_load_op_from_rd(RDD::AttachmentLoadOp p_load_op) {
	switch (p_load_op) {
		case RenderingDeviceDriver::ATTACHMENT_LOAD_OP_LOAD:
			return WGPULoadOp_Load;
		case RenderingDeviceDriver::ATTACHMENT_LOAD_OP_CLEAR:
			return WGPULoadOp_Clear;
		case RenderingDeviceDriver::ATTACHMENT_LOAD_OP_DONT_CARE:
			return WGPULoadOp_Undefined;
	}
}

WGPUStoreOp webgpu_store_op_from_rd(RDD::AttachmentStoreOp p_store_op) {
	switch (p_store_op) {
		case RenderingDeviceDriver::ATTACHMENT_STORE_OP_STORE:
			return WGPUStoreOp_Store;
		case RenderingDeviceDriver::ATTACHMENT_STORE_OP_DONT_CARE:
			return WGPUStoreOp_Undefined;
	}
}

WGPUTextureViewDimension webgpu_texture_view_dimension_from_rd(RDD::TextureType p_texture_type) {
	switch (p_texture_type) {
		case RenderingDeviceCommons::TEXTURE_TYPE_1D:
			return WGPUTextureViewDimension_1D;
		case RenderingDeviceCommons::TEXTURE_TYPE_2D:
			return WGPUTextureViewDimension_2D;
		case RenderingDeviceCommons::TEXTURE_TYPE_3D:
			return WGPUTextureViewDimension_3D;
		case RenderingDeviceCommons::TEXTURE_TYPE_CUBE:
			return WGPUTextureViewDimension_Cube;
		case RenderingDeviceCommons::TEXTURE_TYPE_1D_ARRAY:
			return WGPUTextureViewDimension_Undefined;
		case RenderingDeviceCommons::TEXTURE_TYPE_2D_ARRAY:
			return WGPUTextureViewDimension_2DArray;
		case RenderingDeviceCommons::TEXTURE_TYPE_CUBE_ARRAY:
			return WGPUTextureViewDimension_CubeArray;
		case RenderingDeviceCommons::TEXTURE_TYPE_MAX:
			return WGPUTextureViewDimension_Undefined;
	}
}

WGPUShaderStage webgpu_shader_stage_from_rd(RDD::ShaderStage p_shader_stage) {
	switch (p_shader_stage) {
		case RenderingDeviceCommons::SHADER_STAGE_VERTEX:
			return WGPUShaderStage_Vertex;
		case RenderingDeviceCommons::SHADER_STAGE_FRAGMENT:
			return WGPUShaderStage_Fragment;
		case RenderingDeviceCommons::SHADER_STAGE_COMPUTE:
		case RenderingDeviceCommons::SHADER_STAGE_COMPUTE_BIT:
			return WGPUShaderStage_Compute;
		case RenderingDeviceCommons::SHADER_STAGE_TESSELATION_CONTROL:
		case RenderingDeviceCommons::SHADER_STAGE_TESSELATION_EVALUATION:
		case RenderingDeviceCommons::SHADER_STAGE_TESSELATION_EVALUATION_BIT:
		case RenderingDeviceCommons::SHADER_STAGE_MAX:
			return WGPUShaderStage_None;
	}
}

WGPUTextureAspect webgpu_texture_aspect_from_rd(BitField<RDD::TextureAspectBits> p_texture_aspect_bits) {
	// NOTE: WGPUTextureAspect_All will be preferred if we cannot match a specific aspect.
	if (p_texture_aspect_bits == RDD::TextureAspectBits::TEXTURE_ASPECT_DEPTH_BIT) {
		return WGPUTextureAspect_DepthOnly;
	} else if (p_texture_aspect_bits == RDD::TextureAspectBits::TEXTURE_ASPECT_STENCIL_BIT) {
		return WGPUTextureAspect_StencilOnly;
	} else {
		return WGPUTextureAspect_All;
	}
}

uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPUSupportedLimits p_limits) {
	WGPULimits limits = p_limits.limits;
	// NOTE: For limits that aren't supported, I've put the max uint64 value. This may cause issues.
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

ImageBufferLayoutInfo webgpu_image_buffer_layout_from_format(WGPUTextureFormat p_format) {
	// NOTE: Please also update `webgpu_texture_format_from_rd` alongside this.
	switch ((uint32_t)p_format) {
		case WGPUTextureFormat_R8Unorm:
		case WGPUTextureFormat_R8Snorm:
		case WGPUTextureFormat_R8Uint:
		case WGPUTextureFormat_R8Sint:
			return { 1, 1, 1 };
		case WGPUTextureFormat_R16Uint:
		case WGPUTextureFormat_R16Sint:
		case WGPUTextureFormat_R16Float:
		case WGPUTextureFormatExtras_R16Unorm:
		case WGPUTextureFormatExtras_R16Snorm:
			return { 2, 1, 1 };
		case WGPUTextureFormat_R32Uint:
		case WGPUTextureFormat_R32Sint:
		case WGPUTextureFormat_R32Float:
			return { 4, 1, 1 };
		case WGPUTextureFormat_RG8Unorm:
		case WGPUTextureFormat_RG8Snorm:
		case WGPUTextureFormat_RG8Uint:
		case WGPUTextureFormat_RG8Sint:
			return { 2, 1, 1 };
		case WGPUTextureFormat_RG16Uint:
		case WGPUTextureFormat_RG16Sint:
		case WGPUTextureFormat_RG16Float:
			return { 4, 1, 1 };
		case WGPUTextureFormat_RGBA8Unorm:
		case WGPUTextureFormat_RGBA8UnormSrgb:
		case WGPUTextureFormat_RGBA8Snorm:
		case WGPUTextureFormat_RGBA8Uint:
		case WGPUTextureFormat_RGBA8Sint:
		case WGPUTextureFormat_BGRA8Unorm:
		case WGPUTextureFormat_BGRA8UnormSrgb:
			return { 4, 1, 1 };
		case WGPUTextureFormat_RG32Uint:
		case WGPUTextureFormat_RG32Sint:
		case WGPUTextureFormat_RG32Float:
		case WGPUTextureFormat_RGBA16Uint:
		case WGPUTextureFormat_RGBA16Sint:
		case WGPUTextureFormat_RGBA16Float:
			return { 8, 1, 1 };
		case WGPUTextureFormat_RGBA32Uint:
		case WGPUTextureFormat_RGBA32Sint:
		case WGPUTextureFormat_RGBA32Float:
			return { 16, 1, 1 };
		case WGPUTextureFormat_Stencil8:
			return { 1, 1, 1 };
		case WGPUTextureFormat_Depth16Unorm:
			return { 2, 1, 1 };
		case WGPUTextureFormat_Depth24PlusStencil8:
		case WGPUTextureFormat_Depth32Float:
			return { 4, 1, 1 };
		case WGPUTextureFormat_Depth32FloatStencil8:
			return { 5, 1, 1 };
		default:
			return { 0, 1, 1 };
	}
}
