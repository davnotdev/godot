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
			ret = (WGPUTextureFormat)WGPUNativeTextureFormat_R16Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SNORM:
			ret = (WGPUTextureFormat)WGPUNativeTextureFormat_R16Snorm;
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
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_UNORM:
			/* ret = WGPUTextureFormat_RGBA16Unorm; */
			// HACK: Put this here to bypass errors for now.
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

static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST + 1, WGPUFilterMode_Nearest));
static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR + 1, WGPUFilterMode_Linear));
WGPUFilterMode webgpu_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
	return (WGPUFilterMode)(p_sampler_filter + 1);
}

static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_NEAREST + 1, WGPUMipmapFilterMode_Nearest));
static_assert(ENUM_MEMBERS_EQUAL(RDD::SAMPLER_FILTER_LINEAR + 1, WGPUMipmapFilterMode_Linear));
WGPUMipmapFilterMode webgpu_mipmap_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter) {
	return (WGPUMipmapFilterMode)(p_sampler_filter + 1);
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
	// There is no longer a `WGPUVertexFormat_Undefined` value.
	WGPUVertexFormat ret = WGPUVertexFormat_Uint8;

	switch (p_data_format) {
		case RDD::DATA_FORMAT_R8G8_UINT:
			ret = WGPUVertexFormat_Uint8x2;
			break;
		case RDD::DATA_FORMAT_R8G8B8A8_UINT:
			ret = WGPUVertexFormat_Uint8x4;
			break;
		case RDD::DATA_FORMAT_R8G8_SINT:
			ret = WGPUVertexFormat_Sint8x2;
			break;
		case RDD::DATA_FORMAT_R8G8B8A8_SINT:
			ret = WGPUVertexFormat_Sint8x4;
			break;
		case RDD::DATA_FORMAT_R8G8_UNORM:
			ret = WGPUVertexFormat_Unorm8x2;
			break;
		case RDD::DATA_FORMAT_R8G8B8A8_UNORM:
			ret = WGPUVertexFormat_Unorm8x4;
			break;
		case RDD::DATA_FORMAT_R8G8_SNORM:
			ret = WGPUVertexFormat_Snorm8x2;
			break;
		case RDD::DATA_FORMAT_R8G8B8A8_SNORM:
			ret = WGPUVertexFormat_Snorm8x4;
			break;
		case RDD::DATA_FORMAT_R16G16_UINT:
			ret = WGPUVertexFormat_Uint16x2;
			break;
		case RDD::DATA_FORMAT_R16G16B16A16_UINT:
			ret = WGPUVertexFormat_Uint16x4;
			break;
		case RDD::DATA_FORMAT_R16G16_SINT:
			ret = WGPUVertexFormat_Sint16x2;
			break;
		case RDD::DATA_FORMAT_R16G16B16A16_SINT:
			ret = WGPUVertexFormat_Sint16x4;
			break;
		case RDD::DATA_FORMAT_R16G16_UNORM:
			ret = WGPUVertexFormat_Unorm16x2;
			break;
		case RDD::DATA_FORMAT_R16G16B16A16_UNORM:
			ret = WGPUVertexFormat_Unorm16x4;
			break;
		case RDD::DATA_FORMAT_R16G16_SNORM:
			ret = WGPUVertexFormat_Snorm16x2;
			break;
		case RDD::DATA_FORMAT_R16G16B16A16_SNORM:
			ret = WGPUVertexFormat_Snorm16x4;
			break;
		case RDD::DATA_FORMAT_R16G16_SFLOAT:
			ret = WGPUVertexFormat_Float16x2;
			break;
		case RDD::DATA_FORMAT_R16G16B16A16_SFLOAT:
			ret = WGPUVertexFormat_Float16x4;
			break;
		case RDD::DATA_FORMAT_R32_SFLOAT:
			ret = WGPUVertexFormat_Float32;
			break;
		case RDD::DATA_FORMAT_R32G32_SFLOAT:
			ret = WGPUVertexFormat_Float32x2;
			break;
		case RDD::DATA_FORMAT_R32G32B32_SFLOAT:
			ret = WGPUVertexFormat_Float32x3;
			break;
		case RDD::DATA_FORMAT_R32G32B32A32_SFLOAT:
			ret = WGPUVertexFormat_Float32x4;
			break;
		case RDD::DATA_FORMAT_R32_UINT:
			ret = WGPUVertexFormat_Uint32;
			break;
		case RDD::DATA_FORMAT_R32G32_UINT:
			ret = WGPUVertexFormat_Uint32x2;
			break;
		case RDD::DATA_FORMAT_R32G32B32_UINT:
			ret = WGPUVertexFormat_Uint32x3;
			break;
		case RDD::DATA_FORMAT_R32G32B32A32_UINT:
			ret = WGPUVertexFormat_Uint32x4;
			break;
		case RDD::DATA_FORMAT_R32_SINT:
			ret = WGPUVertexFormat_Sint32;
			break;
		case RDD::DATA_FORMAT_R32G32_SINT:
			ret = WGPUVertexFormat_Sint32x2;
			break;
		case RDD::DATA_FORMAT_R32G32B32_SINT:
			ret = WGPUVertexFormat_Sint32x3;
			break;
		case RDD::DATA_FORMAT_R32G32B32A32_SINT:
			ret = WGPUVertexFormat_Sint32x4;
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
			return WGPULoadOp_Load;
	}
}

WGPUStoreOp webgpu_store_op_from_rd(RDD::AttachmentStoreOp p_store_op) {
	switch (p_store_op) {
		case RenderingDeviceDriver::ATTACHMENT_STORE_OP_STORE:
			return WGPUStoreOp_Store;
		case RenderingDeviceDriver::ATTACHMENT_STORE_OP_DONT_CARE:
			return WGPUStoreOp_Discard;
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

// See https://github.com/gfx-rs/wgpu/blob/03ab3a27d8a2b35cc0afd7cfe418f74a2498ff20/wgpu-hal/src/lib.rs#L1689
WGPUTextureAspect webgpu_texture_aspect_from_rd_format(RDD::DataFormat p_data_format) {
	switch (p_data_format) {
		case RDD::DataFormat::DATA_FORMAT_S8_UINT:
			return WGPUTextureAspect_StencilOnly;
		case RDD::DataFormat::DATA_FORMAT_D16_UNORM:
		case RDD::DataFormat::DATA_FORMAT_D32_SFLOAT:
			return WGPUTextureAspect_DepthOnly;
		default:
			return WGPUTextureAspect_All;
	}
}


WGPUBlendOperation webgpu_blend_operation_from_rd(RDD::BlendOperation p_blend_operation) {
	switch (p_blend_operation) {
		case RenderingDeviceCommons::BLEND_OP_ADD:
			return WGPUBlendOperation_Add;
		case RenderingDeviceCommons::BLEND_OP_SUBTRACT:
			return WGPUBlendOperation_Subtract;
		case RenderingDeviceCommons::BLEND_OP_REVERSE_SUBTRACT:
			return WGPUBlendOperation_ReverseSubtract;
		case RenderingDeviceCommons::BLEND_OP_MINIMUM:
			return WGPUBlendOperation_Min;
		case RenderingDeviceCommons::BLEND_OP_MAXIMUM:
			return WGPUBlendOperation_Max;
		default:
			return WGPUBlendOperation_Add;
	}
}

WGPUBlendFactor webgpu_blend_factor_from_rd(RDD::BlendFactor p_blend_factor) {
	// NOTE: Some of these blend factors are not supported, so we default to `WGPUBlendFactor_Zero`
	switch (p_blend_factor) {
		case RenderingDeviceCommons::BLEND_FACTOR_ZERO:
			return WGPUBlendFactor_Zero;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE:
			return WGPUBlendFactor_One;
		case RenderingDeviceCommons::BLEND_FACTOR_SRC_COLOR:
			return WGPUBlendFactor_Src;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
			return WGPUBlendFactor_OneMinusSrc;
		case RenderingDeviceCommons::BLEND_FACTOR_DST_COLOR:
			return WGPUBlendFactor_Dst;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_DST_COLOR:
			return WGPUBlendFactor_OneMinusDst;
		case RenderingDeviceCommons::BLEND_FACTOR_SRC_ALPHA:
			return WGPUBlendFactor_SrcAlpha;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
			return WGPUBlendFactor_OneMinusSrcAlpha;
		case RenderingDeviceCommons::BLEND_FACTOR_DST_ALPHA:
			return WGPUBlendFactor_DstAlpha;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
			return WGPUBlendFactor_OneMinusDstAlpha;
		case RenderingDeviceCommons::BLEND_FACTOR_CONSTANT_COLOR:
			return WGPUBlendFactor_Constant;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
			return WGPUBlendFactor_OneMinusConstant;
		case RenderingDeviceCommons::BLEND_FACTOR_CONSTANT_ALPHA:
			return WGPUBlendFactor_Constant;
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
			return WGPUBlendFactor_OneMinusConstant;
		case RenderingDeviceCommons::BLEND_FACTOR_SRC_ALPHA_SATURATE:
			return WGPUBlendFactor_SrcAlphaSaturated;
		case RenderingDeviceCommons::BLEND_FACTOR_SRC1_COLOR:
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
		case RenderingDeviceCommons::BLEND_FACTOR_SRC1_ALPHA:
		case RenderingDeviceCommons::BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
		case RenderingDeviceCommons::BLEND_FACTOR_MAX:
			return WGPUBlendFactor_Zero;
	}
}

WGPUStencilOperation webgpu_stencil_operation_from_rd(RDD::StencilOperation p_stencil_operation) {
	switch (p_stencil_operation) {
		case RenderingDeviceCommons::STENCIL_OP_KEEP:
			return WGPUStencilOperation_Keep;
		case RenderingDeviceCommons::STENCIL_OP_ZERO:
			return WGPUStencilOperation_Zero;
		case RenderingDeviceCommons::STENCIL_OP_REPLACE:
			return WGPUStencilOperation_Replace;
		case RenderingDeviceCommons::STENCIL_OP_INCREMENT_AND_CLAMP:
			return WGPUStencilOperation_IncrementClamp;
		case RenderingDeviceCommons::STENCIL_OP_DECREMENT_AND_CLAMP:
			return WGPUStencilOperation_DecrementClamp;
		case RenderingDeviceCommons::STENCIL_OP_INVERT:
			return WGPUStencilOperation_Invert;
		case RenderingDeviceCommons::STENCIL_OP_INCREMENT_AND_WRAP:
			return WGPUStencilOperation_IncrementWrap;
		case RenderingDeviceCommons::STENCIL_OP_DECREMENT_AND_WRAP:
			return WGPUStencilOperation_DecrementWrap;
		case RenderingDeviceCommons::STENCIL_OP_MAX:
			return WGPUStencilOperation_Keep;
	}
}

uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPULimits p_limits) {
	// NOTE: For limits that aren't supported, I've put the max uint64 value. This may cause issues.
	switch (p_selected_limit) {
		case RenderingDeviceCommons::LIMIT_MAX_BOUND_UNIFORM_SETS:
			return p_limits.maxBindGroups;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS:
			return p_limits.maxColorAttachments;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURES_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_SAMPLERS_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_BUFFERS_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_IMAGES_PER_UNIFORM_SET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFERS_PER_UNIFORM_SET:
			return p_limits.maxUniformBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_DRAW_INDEXED_INDEX:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_HEIGHT:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_FRAMEBUFFER_WIDTH:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_ARRAY_LAYERS:
			return p_limits.maxTextureArrayLayers;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_1D:
			return p_limits.maxTextureDimension1D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_2D:
			return p_limits.maxTextureDimension2D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_3D:
			return p_limits.maxTextureDimension3D;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURE_SIZE_CUBE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_TEXTURES_PER_SHADER_STAGE:
			return p_limits.maxSampledTexturesPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_SAMPLERS_PER_SHADER_STAGE:
			return p_limits.maxSamplersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE:
			return p_limits.maxStorageBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_STORAGE_IMAGES_PER_SHADER_STAGE:
			return p_limits.maxStorageTexturesPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE:
			return p_limits.maxUniformBuffersPerShaderStage;
		case RenderingDeviceCommons::LIMIT_MAX_PUSH_CONSTANT_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_UNIFORM_BUFFER_SIZE:
			return p_limits.maxUniformBufferBindingSize;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_ATTRIBUTE_OFFSET:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_ATTRIBUTES:
			return p_limits.maxVertexAttributes;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_BINDINGS:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_VERTEX_INPUT_BINDING_STRIDE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
			return p_limits.minUniformBufferOffsetAlignment;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_SHARED_MEMORY_SIZE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_INVOCATIONS:
			return p_limits.maxComputeInvocationsPerWorkgroup;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_X:
			return p_limits.maxComputeWorkgroupSizeX;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Y:
			return p_limits.maxComputeWorkgroupSizeY;
		case RenderingDeviceCommons::LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Z:
			return p_limits.maxComputeWorkgroupSizeZ;
		// HACK: These need to be defined to get a working window.
		// I'm not sure what this number should be.
		case RenderingDeviceCommons::LIMIT_MAX_VIEWPORT_DIMENSIONS_X:
			return 99999;
		case RenderingDeviceCommons::LIMIT_MAX_VIEWPORT_DIMENSIONS_Y:
			return 99999;
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
		case RenderingDeviceCommons::LIMIT_METALFX_TEMPORAL_SCALER_MIN_SCALE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_METALFX_TEMPORAL_SCALER_MAX_SCALE:
			return UINT64_MAX;
		case RenderingDeviceCommons::LIMIT_MAX_SHADER_VARYINGS:
			return UINT64_MAX;
	}
}

WGPUTextureComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle) {
	switch (p_texture_swizzle) {
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_ZERO:
			return WGPUTextureComponentSwizzle_Zero;
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_ONE:
			return WGPUTextureComponentSwizzle_One;
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_R:
			return WGPUTextureComponentSwizzle_R;
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_G:
			return WGPUTextureComponentSwizzle_G;
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_B:
			return WGPUTextureComponentSwizzle_B;
		case RDD::TextureSwizzle::TEXTURE_SWIZZLE_A:
			return WGPUTextureComponentSwizzle_A;
		default:
			return WGPUTextureComponentSwizzle_Identity;
	}
}

uint32_t webgpu_texture_format_block_copy_size(WGPUTextureFormat format, WGPUTextureAspect aspect) {
	switch ((WGPUNativeTextureFormat)format) {
		case WGPUNativeTextureFormat_R16Unorm:
		case WGPUNativeTextureFormat_R16Snorm:
			return 2;
		case WGPUNativeTextureFormat_Rg16Unorm:
		case WGPUNativeTextureFormat_Rg16Snorm:
			return 4;
		case WGPUNativeTextureFormat_Rgba16Unorm:
		case WGPUNativeTextureFormat_Rgba16Snorm:
			return 8;
		case WGPUNativeTextureFormat_NV12:
			/*
			if (aspect == TextureAspect_Plane0) {
				return 1;
			}
			if (aspect == TextureAspect_Plane1) {
				return 2;
			}
			*/
			return UINT32_MAX;
	}

	switch (format) {
		case WGPUTextureFormat_R8Unorm:
		case WGPUTextureFormat_R8Snorm:
		case WGPUTextureFormat_R8Uint:
		case WGPUTextureFormat_R8Sint:
		case WGPUTextureFormat_Stencil8:
			return 1;

		case WGPUTextureFormat_RG8Unorm:
		case WGPUTextureFormat_RG8Snorm:
		case WGPUTextureFormat_RG8Uint:
		case WGPUTextureFormat_RG8Sint:
		// case (WGPUTextureFormat)WGPUNativeTextureFormat_R16Unorm:
		// case (WGPUTextureFormat)WGPUNativeTextureFormat_R16Snorm:
		case WGPUTextureFormat_R16Uint:
		case WGPUTextureFormat_R16Sint:
		case WGPUTextureFormat_R16Float:
		case WGPUTextureFormat_Depth16Unorm:
			return 2;

		case WGPUTextureFormat_RGBA8Unorm:
		case WGPUTextureFormat_RGBA8UnormSrgb:
		case WGPUTextureFormat_RGBA8Snorm:
		case WGPUTextureFormat_RGBA8Uint:
		case WGPUTextureFormat_RGBA8Sint:
		case WGPUTextureFormat_BGRA8Unorm:
		case WGPUTextureFormat_BGRA8UnormSrgb:
		// case (WGPUTextureFormat)WGPUNativeTextureFormat_Rg16Unorm:
		// case (WGPUTextureFormat)WGPUNativeTextureFormat_Rg16Snorm:
		case WGPUTextureFormat_RG16Uint:
		case WGPUTextureFormat_RG16Sint:
		case WGPUTextureFormat_RG16Float:
		case WGPUTextureFormat_R32Uint:
		case WGPUTextureFormat_R32Sint:
		case WGPUTextureFormat_R32Float:
		case WGPUTextureFormat_RGB9E5Ufloat:
		case WGPUTextureFormat_RGB10A2Uint:
		case WGPUTextureFormat_RGB10A2Unorm:
		case WGPUTextureFormat_RG11B10Ufloat:
		case WGPUTextureFormat_Depth32Float:
			return 4;

		// case (WGPUTextureFormat)WGPUNativeTextureFormat_Rgba16Unorm:
		// case (WGPUTextureFormat)WGPUNativeTextureFormat_Rgba16Snorm:
		case WGPUTextureFormat_RGBA16Uint:
		case WGPUTextureFormat_RGBA16Sint:
		case WGPUTextureFormat_RGBA16Float:
		// case WGPUTextureFormat_R64Uint:
		case WGPUTextureFormat_RG32Uint:
		case WGPUTextureFormat_RG32Sint:
		case WGPUTextureFormat_RG32Float:
			return 8;

		case WGPUTextureFormat_RGBA32Uint:
		case WGPUTextureFormat_RGBA32Sint:
		case WGPUTextureFormat_RGBA32Float:
			return 16;

		case WGPUTextureFormat_Depth24Plus:
			return -1;

		case WGPUTextureFormat_Depth24PlusStencil8:
			if (aspect == WGPUTextureAspect_StencilOnly) {
				return 1;
			}
			return -1;

		case WGPUTextureFormat_Depth32FloatStencil8:
			if (aspect == WGPUTextureAspect_DepthOnly) {
				return 4;
			}
			if (aspect == WGPUTextureAspect_StencilOnly) {
				return 1;
			}
			return UINT32_MAX;

		case WGPUTextureFormat_BC1RGBAUnorm:
		case WGPUTextureFormat_BC1RGBAUnormSrgb:
		case WGPUTextureFormat_BC4RUnorm:
		case WGPUTextureFormat_BC4RSnorm:
		case WGPUTextureFormat_ETC2RGB8Unorm:
		case WGPUTextureFormat_ETC2RGB8UnormSrgb:
		case WGPUTextureFormat_ETC2RGB8A1Unorm:
		case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
		case WGPUTextureFormat_EACR11Unorm:
		case WGPUTextureFormat_EACR11Snorm:
			return 8;

		case WGPUTextureFormat_BC2RGBAUnorm:
		case WGPUTextureFormat_BC2RGBAUnormSrgb:
		case WGPUTextureFormat_BC3RGBAUnorm:
		case WGPUTextureFormat_BC3RGBAUnormSrgb:
		case WGPUTextureFormat_BC5RGUnorm:
		case WGPUTextureFormat_BC5RGSnorm:
		case WGPUTextureFormat_BC6HRGBUfloat:
		case WGPUTextureFormat_BC6HRGBFloat:
		case WGPUTextureFormat_BC7RGBAUnorm:
		case WGPUTextureFormat_BC7RGBAUnormSrgb:
		case WGPUTextureFormat_ETC2RGBA8Unorm:
		case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
		case WGPUTextureFormat_EACRG11Unorm:
		case WGPUTextureFormat_EACRG11Snorm:
		case WGPUTextureFormat_ASTC4x4Unorm:
		case WGPUTextureFormat_ASTC4x4UnormSrgb:
		case WGPUTextureFormat_ASTC5x4Unorm:
		case WGPUTextureFormat_ASTC5x4UnormSrgb:
		case WGPUTextureFormat_ASTC5x5Unorm:
		case WGPUTextureFormat_ASTC5x5UnormSrgb:
		case WGPUTextureFormat_ASTC6x5Unorm:
		case WGPUTextureFormat_ASTC6x5UnormSrgb:
		case WGPUTextureFormat_ASTC6x6Unorm:
		case WGPUTextureFormat_ASTC6x6UnormSrgb:
		case WGPUTextureFormat_ASTC8x5Unorm:
		case WGPUTextureFormat_ASTC8x5UnormSrgb:
		case WGPUTextureFormat_ASTC8x6Unorm:
		case WGPUTextureFormat_ASTC8x6UnormSrgb:
		case WGPUTextureFormat_ASTC8x8Unorm:
		case WGPUTextureFormat_ASTC8x8UnormSrgb:
		case WGPUTextureFormat_ASTC10x5Unorm:
		case WGPUTextureFormat_ASTC10x5UnormSrgb:
		case WGPUTextureFormat_ASTC10x6Unorm:
		case WGPUTextureFormat_ASTC10x6UnormSrgb:
		case WGPUTextureFormat_ASTC10x8Unorm:
		case WGPUTextureFormat_ASTC10x8UnormSrgb:
		case WGPUTextureFormat_ASTC10x10Unorm:
		case WGPUTextureFormat_ASTC10x10UnormSrgb:
		case WGPUTextureFormat_ASTC12x10Unorm:
		case WGPUTextureFormat_ASTC12x10UnormSrgb:
		case WGPUTextureFormat_ASTC12x12Unorm:
		case WGPUTextureFormat_ASTC12x12UnormSrgb:
			return 16;

		default:
			return -1; // Unknown format
	}
}

FormatBlockDimension webgpu_texture_format_block_dimensions(WGPUTextureFormat format) {
	switch ((WGPUNativeTextureFormat)format) {
		case WGPUNativeTextureFormat_R16Unorm:
		case WGPUNativeTextureFormat_R16Snorm:
		case WGPUNativeTextureFormat_Rg16Unorm:
		case WGPUNativeTextureFormat_Rg16Snorm:
		case WGPUNativeTextureFormat_Rgba16Unorm:
		case WGPUNativeTextureFormat_Rgba16Snorm:
		case WGPUNativeTextureFormat_NV12:
			return (FormatBlockDimension){ 1, 1 };
	}

	switch (format) {
		case WGPUTextureFormat_R8Unorm:
		case WGPUTextureFormat_R8Snorm:
		case WGPUTextureFormat_R8Uint:
		case WGPUTextureFormat_R8Sint:
		case WGPUTextureFormat_R16Uint:
		case WGPUTextureFormat_R16Sint:
		// case WGPUTextureFormat_R16Unorm:
		// case WGPUTextureFormat_R16Snorm:
		case WGPUTextureFormat_R16Float:
		case WGPUTextureFormat_RG8Unorm:
		case WGPUTextureFormat_RG8Snorm:
		case WGPUTextureFormat_RG8Uint:
		case WGPUTextureFormat_RG8Sint:
		case WGPUTextureFormat_R32Uint:
		case WGPUTextureFormat_R32Sint:
		case WGPUTextureFormat_R32Float:
		case WGPUTextureFormat_RG16Uint:
		case WGPUTextureFormat_RG16Sint:
		// case WGPUTextureFormat_Rg16Unorm:
		// case WGPUTextureFormat_Rg16Snorm:
		case WGPUTextureFormat_RG16Float:
		case WGPUTextureFormat_RGBA8Unorm:
		case WGPUTextureFormat_RGBA8UnormSrgb:
		case WGPUTextureFormat_RGBA8Snorm:
		case WGPUTextureFormat_RGBA8Uint:
		case WGPUTextureFormat_RGBA8Sint:
		case WGPUTextureFormat_BGRA8Unorm:
		case WGPUTextureFormat_BGRA8UnormSrgb:
		case WGPUTextureFormat_RGB9E5Ufloat:
		case WGPUTextureFormat_RGB10A2Uint:
		case WGPUTextureFormat_RGB10A2Unorm:
		case WGPUTextureFormat_RG11B10Ufloat:
		// case WGPUTextureFormat_R64Uint:
		case WGPUTextureFormat_RG32Uint:
		case WGPUTextureFormat_RG32Sint:
		case WGPUTextureFormat_RG32Float:
		case WGPUTextureFormat_RGBA16Uint:
		case WGPUTextureFormat_RGBA16Sint:
		// case WGPUTextureFormat_RGBA16Unorm:
		// case WGPUTextureFormat_RGBA16Snorm:
		case WGPUTextureFormat_RGBA16Float:
		case WGPUTextureFormat_RGBA32Uint:
		case WGPUTextureFormat_RGBA32Sint:
		case WGPUTextureFormat_RGBA32Float:
		case WGPUTextureFormat_Stencil8:
		case WGPUTextureFormat_Depth16Unorm:
		case WGPUTextureFormat_Depth24Plus:
		case WGPUTextureFormat_Depth24PlusStencil8:
		// case WGPUTextureFormat_NV12:
		case WGPUTextureFormat_Depth32Float:
		case WGPUTextureFormat_Depth32FloatStencil8:
			return (FormatBlockDimension){ 1, 1 };

		case WGPUTextureFormat_BC1RGBAUnorm:
		case WGPUTextureFormat_BC1RGBAUnormSrgb:
		case WGPUTextureFormat_BC2RGBAUnorm:
		case WGPUTextureFormat_BC2RGBAUnormSrgb:
		case WGPUTextureFormat_BC3RGBAUnorm:
		case WGPUTextureFormat_BC3RGBAUnormSrgb:
		case WGPUTextureFormat_BC4RUnorm:
		case WGPUTextureFormat_BC4RSnorm:
		case WGPUTextureFormat_BC5RGUnorm:
		case WGPUTextureFormat_BC5RGSnorm:
		case WGPUTextureFormat_BC6HRGBUfloat:
		case WGPUTextureFormat_BC6HRGBFloat:
		case WGPUTextureFormat_BC7RGBAUnorm:
		case WGPUTextureFormat_BC7RGBAUnormSrgb:
		case WGPUTextureFormat_ETC2RGB8Unorm:
		case WGPUTextureFormat_ETC2RGB8UnormSrgb:
		case WGPUTextureFormat_ETC2RGB8A1Unorm:
		case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
		case WGPUTextureFormat_ETC2RGBA8Unorm:
		case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
		case WGPUTextureFormat_EACR11Unorm:
		case WGPUTextureFormat_EACR11Snorm:
		case WGPUTextureFormat_EACRG11Unorm:
		case WGPUTextureFormat_EACRG11Snorm:
			return (FormatBlockDimension){ 4, 4 };

		case WGPUTextureFormat_ASTC4x4Unorm:
		case WGPUTextureFormat_ASTC4x4UnormSrgb:
			return (FormatBlockDimension){ 4, 4 };
		case WGPUTextureFormat_ASTC5x4Unorm:
		case WGPUTextureFormat_ASTC5x4UnormSrgb:
			return (FormatBlockDimension){ 5, 4 };
		case WGPUTextureFormat_ASTC5x5Unorm:
		case WGPUTextureFormat_ASTC5x5UnormSrgb:
			return (FormatBlockDimension){ 5, 5 };
		case WGPUTextureFormat_ASTC6x5Unorm:
		case WGPUTextureFormat_ASTC6x5UnormSrgb:
			return (FormatBlockDimension){ 6, 5 };
		case WGPUTextureFormat_ASTC6x6Unorm:
		case WGPUTextureFormat_ASTC6x6UnormSrgb:
			return (FormatBlockDimension){ 6, 6 };
		case WGPUTextureFormat_ASTC8x5Unorm:
		case WGPUTextureFormat_ASTC8x5UnormSrgb:
			return (FormatBlockDimension){ 8, 5 };
		case WGPUTextureFormat_ASTC8x6Unorm:
		case WGPUTextureFormat_ASTC8x6UnormSrgb:
			return (FormatBlockDimension){ 8, 6 };
		case WGPUTextureFormat_ASTC8x8Unorm:
		case WGPUTextureFormat_ASTC8x8UnormSrgb:
			return (FormatBlockDimension){ 8, 8 };
		case WGPUTextureFormat_ASTC10x5Unorm:
		case WGPUTextureFormat_ASTC10x5UnormSrgb:
			return (FormatBlockDimension){ 10, 5 };
		case WGPUTextureFormat_ASTC10x6Unorm:
		case WGPUTextureFormat_ASTC10x6UnormSrgb:
			return (FormatBlockDimension){ 10, 6 };
		case WGPUTextureFormat_ASTC10x8Unorm:
		case WGPUTextureFormat_ASTC10x8UnormSrgb:
			return (FormatBlockDimension){ 10, 8 };
		case WGPUTextureFormat_ASTC10x10Unorm:
		case WGPUTextureFormat_ASTC10x10UnormSrgb:
			return (FormatBlockDimension){ 10, 10 };
		case WGPUTextureFormat_ASTC12x10Unorm:
		case WGPUTextureFormat_ASTC12x10UnormSrgb:
			return (FormatBlockDimension){ 12, 10 };
		case WGPUTextureFormat_ASTC12x12Unorm:
		case WGPUTextureFormat_ASTC12x12UnormSrgb:
			return (FormatBlockDimension){ 12, 12 };
		default:
			return (FormatBlockDimension){ 1, 1 };
	}
}
