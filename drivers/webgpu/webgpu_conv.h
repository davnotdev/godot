#ifndef WEBGPU_CONV_H
#define WEBGPU_CONV_H

#include "servers/rendering/rendering_device.h"
#include <webgpu.h>
#include <wgpu.h>

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
WGPUTextureAspect webgpu_texture_aspect_from_rd(BitField<RDD::TextureAspectBits> p_texture_aspect_bits);
WGPUTextureAspect webgpu_texture_aspect_from_rd_format(RDD::DataFormat p_data_format);
WGPUBlendOperation webgpu_blend_operation_from_rd(RDD::BlendOperation p_blend_operation);
WGPUBlendFactor webgpu_blend_factor_from_rd(RDD::BlendFactor p_blend_factor);
WGPUStencilOperation webgpu_stencil_operation_from_rd(RDD::StencilOperation p_stencil_operation);
WGPUTextureComponentSwizzle webgpu_component_swizzle_from_rd(RDD::TextureSwizzle p_texture_swizzle);

uint64_t rd_limit_from_webgpu(RDD::Limit p_selected_limit, WGPULimits p_limits);

typedef struct {
	uint32_t block_dim_x;
	uint32_t block_dim_y;
} FormatBlockDimension;

uint32_t webgpu_texture_format_block_copy_size(WGPUTextureFormat format, WGPUTextureAspect aspect);
FormatBlockDimension webgpu_texture_format_block_dimensions(WGPUTextureFormat format);

#endif // WEBGPU_CONV_H
