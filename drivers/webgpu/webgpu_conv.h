#ifndef WEBGPU_CONV_H
#define WEBGPU_CONV_H

#include "servers/rendering/rendering_device.h"

#include <webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
#include <wgpu.h>
#endif

WGPUBufferUsage webgpu_buffer_usage_from_rd(BitField<RDD::BufferUsageBits> p_buffer_usage);
WGPUTextureFormat webgpu_texture_format_from_rd(RDD::DataFormat p_data_format);
WGPUFilterMode webgpu_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter);
WGPUMipmapFilterMode webgpu_mipmap_filter_mode_from_rd(RDD::SamplerFilter p_sampler_filter);
WGPUAddressMode webgpu_address_mode_from_rd(RDD::SamplerRepeatMode p_sampler_repeat_mode);
WGPUCompareFunction webgpu_compare_mode_from_rd(RDD::CompareOperator p_compare_operator);
WGPUVertexFormat webgpu_vertex_format_from_rd(RDD::DataFormat p_data_format);
WGPULoadOp webgpu_load_op_from_rd(RDD::AttachmentLoadOp p_load_op);
WGPUStoreOp webgpu_store_op_from_rd(RDD::AttachmentStoreOp p_store_op);
WGPUTextureViewDimension webgpu_texture_view_dimension_from_rd(RDD::TextureType p_texture_type);
WGPUShaderStage webgpu_shader_stage_from_rd(RDD::ShaderStage p_shader_stage);
WGPUTextureAspect webgpu_texture_aspect_from_rd(RDD::TextureAspect p_texture_aspect);
WGPUTextureAspect webgpu_texture_aspect_from_rd(BitField<RDD::TextureAspectBits> p_texture_aspect);
WGPUTextureAspect webgpu_texture_aspect_from_rd_format(RDD::DataFormat p_data_format);
WGPUBlendOperation webgpu_blend_operation_from_rd(RDD::BlendOperation p_blend_operation);
WGPUBlendFactor webgpu_blend_factor_from_rd(RDD::BlendFactor p_blend_factor);
WGPUStencilOperation webgpu_stencil_operation_from_rd(RDD::StencilOperation p_stencil_operation);

// HACK: `wgpu` does not support swizzle: https://github.com/gfx-rs/wgpu/issues/1028
// Currently, our fork has it patched in for Vulkan-only.
#ifdef WEBGPU_BACKEND_DAWN_DESKTOP
WGPUComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle);
#elif defined(WEBGPU_BACKEND_WGPU_DESKTOP)
WGPUNativeTextureComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle);
#endif

WGPUTextureSampleType webgpu_texture_sample_type_from_shader_uniform(RDD::ShaderUniform::TextureSampleType p_texture_sample_type);

RDD::DataFormat rd_texture_format_from_webgpu(WGPUTextureFormat p_format);
uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPULimits p_limits);

struct FormatBlockDimension {
	uint32_t block_dim_x;
	uint32_t block_dim_y;
};

bool webgpu_texture_format_is_depth_stencil(WGPUTextureFormat p_format);
bool webgpu_texture_format_has_depth_aspect(WGPUTextureFormat p_format);
bool webgpu_texture_format_has_stencil_aspect(WGPUTextureFormat p_format);
WGPUTextureFormat webgpu_texture_format_downgrade_depth_only(WGPUTextureFormat p_format);
uint32_t webgpu_texture_format_block_copy_size(WGPUTextureFormat format, WGPUTextureAspect aspect);
FormatBlockDimension webgpu_texture_format_block_dimensions(WGPUTextureFormat format);

// TODO: This needs to account for texture tiers.
// https://www.w3.org/TR/webgpu/#texture-formats
static const WGPUTextureFormat WEBGPU_CORE_SUPPORTED_FORMATS[] = {
	// 8-bit formats.
	WGPUTextureFormat_R8Unorm,
	WGPUTextureFormat_R8Snorm,
	WGPUTextureFormat_R8Uint,
	WGPUTextureFormat_R8Sint,

	// 16-bit formats.
	// WGPUTextureFormat_R16Unorm,
	// WGPUTextureFormat_R16Snorm,

	WGPUTextureFormat_R16Uint,
	WGPUTextureFormat_R16Sint,
	WGPUTextureFormat_R16Float,
	WGPUTextureFormat_RG8Unorm,
	WGPUTextureFormat_RG8Snorm,
	WGPUTextureFormat_RG8Uint,
	WGPUTextureFormat_RG8Sint,

	// 32-bit formats.
	WGPUTextureFormat_R32Uint,
	WGPUTextureFormat_R32Sint,
	WGPUTextureFormat_R32Float,

	// WGPUTextureFormat_RG16Unorm,
	// WGPUTextureFormat_RG16Snorm,

	WGPUTextureFormat_RG16Uint,
	WGPUTextureFormat_RG16Sint,
	WGPUTextureFormat_RG16Float,
	WGPUTextureFormat_RGBA8Unorm,
	WGPUTextureFormat_RGBA8UnormSrgb,
	WGPUTextureFormat_RGBA8Snorm,
	WGPUTextureFormat_RGBA8Uint,
	WGPUTextureFormat_RGBA8Sint,
	WGPUTextureFormat_BGRA8Unorm,
	WGPUTextureFormat_BGRA8UnormSrgb,
	// Packed 32-bit formats
	WGPUTextureFormat_RGB9E5Ufloat,
	WGPUTextureFormat_RGBA8Uint,
	WGPUTextureFormat_RGBA8Sint,
	WGPUTextureFormat_BGRA8Unorm,
	WGPUTextureFormat_BGRA8UnormSrgb,
	// Packed 32-bit formats.
	WGPUTextureFormat_RGB9E5Ufloat,
	WGPUTextureFormat_RGB10A2Uint,
	WGPUTextureFormat_RGB10A2Unorm,
	WGPUTextureFormat_RG11B10Ufloat,

	// 64-bit formats.
	WGPUTextureFormat_RG32Uint,
	WGPUTextureFormat_RG32Sint,
	WGPUTextureFormat_RG32Float,

	// WGPUTextureFormat_RGBA16Unorm,
	// WGPUTextureFormat_RGBA16Snorm,

	WGPUTextureFormat_RGBA16Uint,
	WGPUTextureFormat_RGBA16Sint,
	WGPUTextureFormat_RGBA16Float,

	// 128-bit formats.
	WGPUTextureFormat_RGBA32Uint,
	WGPUTextureFormat_RGBA32Sint,
	WGPUTextureFormat_RGBA32Float,

	// Depth/stencil formats.
	WGPUTextureFormat_Stencil8,
	WGPUTextureFormat_Depth16Unorm,
	WGPUTextureFormat_Depth24Plus,
	WGPUTextureFormat_Depth24PlusStencil8,
	WGPUTextureFormat_Depth32Float,
	WGPUTextureFormat_Depth32FloatStencil8,

	// BC compressed formats.
	// WGPUTextureFormat_BC1RGBAUnorm,
	// WGPUTextureFormat_BC1RGBAUnormSrgb,
	// WGPUTextureFormat_BC2RGBAUnorm,
	// WGPUTextureFormat_BC2RGBAUnormSrgb,
	// WGPUTextureFormat_BC3RGBAUnorm,
	// WGPUTextureFormat_BC3RGBAUnormSrgb,
	// WGPUTextureFormat_BC4RUnorm,
	// WGPUTextureFormat_BC4RSnorm,
	// WGPUTextureFormat_BC5RGUnorm,
	// WGPUTextureFormat_BC5RGSnorm,
	// WGPUTextureFormat_BC6HRGBUfloat,
	// WGPUTextureFormat_BC6HRGBFloat,
	// WGPUTextureFormat_BC7RGBAUnorm,
	// WGPUTextureFormat_BC7RGBAUnormSrgb,

	// ETC2 compressed formats.
	// WGPUTextureFormat_ETC2RGB8Unorm,
	// WGPUTextureFormat_ETC2RGB8UnormSrgb,
	// WGPUTextureFormat_ETC2RGB8A1Unorm,
	// WGPUTextureFormat_ETC2RGB8A1UnormSrgb,
	// WGPUTextureFormat_ETC2RGBA8Unorm,
	// WGPUTextureFormat_ETC2RGBA8UnormSrgb,
	// WGPUTextureFormat_EACR11Unorm,
	// WGPUTextureFormat_EACR11Snorm,
	// WGPUTextureFormat_EACRG11Unorm,
	// WGPUTextureFormat_EACRG11Snorm,

	// ASTC compressed formats.
	// WGPUTextureFormat_ASTC4x4Unorm,
	// WGPUTextureFormat_ASTC4x4UnormSrgb,
	// WGPUTextureFormat_ASTC5x4Unorm,
	// WGPUTextureFormat_ASTC5x4UnormSrgb,
	// WGPUTextureFormat_ASTC5x5Unorm,
	// WGPUTextureFormat_ASTC5x5UnormSrgb,
	// WGPUTextureFormat_ASTC6x5Unorm,
	// WGPUTextureFormat_ASTC6x5UnormSrgb,
	// WGPUTextureFormat_ASTC6x6Unorm,
	// WGPUTextureFormat_ASTC6x6UnormSrgb,
	// WGPUTextureFormat_ASTC8x5Unorm,
	// WGPUTextureFormat_ASTC8x5UnormSrgb,
	// WGPUTextureFormat_ASTC8x6Unorm,
	// WGPUTextureFormat_ASTC8x6UnormSrgb,
	// WGPUTextureFormat_ASTC8x8Unorm,
	// WGPUTextureFormat_ASTC8x8UnormSrgb,
	// WGPUTextureFormat_ASTC10x5Unorm,
	// WGPUTextureFormat_ASTC10x5UnormSrgb,
	// WGPUTextureFormat_ASTC10x6Unorm,
	// WGPUTextureFormat_ASTC10x6UnormSrgb,
	// WGPUTextureFormat_ASTC10x8Unorm,
	// WGPUTextureFormat_ASTC10x8UnormSrgb,
	// WGPUTextureFormat_ASTC10x10Unorm,
	// WGPUTextureFormat_ASTC10x10UnormSrgb,
	// WGPUTextureFormat_ASTC12x10Unorm,
	// WGPUTextureFormat_ASTC12x10UnormSrgb,
	// WGPUTextureFormat_ASTC12x12Unorm,
	// WGPUTextureFormat_ASTC12x12UnormSrgb,
};

#endif // WEBGPU_CONV_H
