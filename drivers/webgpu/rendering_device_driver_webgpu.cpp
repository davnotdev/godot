#include "rendering_device_driver_webgpu.h"
#include "core/io/marshalls.h"
#include "thirdparty/misc/smolv.h"
#include "webgpu_conv.h"

#include <wgpu.h>
#include <cstring>

static void handle_request_device(WGPURequestDeviceStatus status,
		WGPUDevice device, char const *message,
		void *userdata) {
	*(WGPUDevice *)userdata = device;
}

Error RenderingDeviceDriverWebGpu::initialize(uint32_t p_device_index, uint32_t p_frame_count) {
	adapter = context_driver->adapter_get(p_device_index);
	context_device = context_driver->device_get(p_device_index);

	WGPUFeatureName required_features[] = {
		(WGPUFeatureName)WGPUNativeFeature_PushConstants,
		(WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm,
		(WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures,
		(WGPUFeatureName)WGPUNativeFeature_TextureBindingArray,
	};

	WGPUDeviceDescriptor device_desc = (WGPUDeviceDescriptor){
		.requiredFeatureCount = sizeof(required_features) / sizeof(WGPUFeatureName),
		.requiredFeatures = required_features,
	};
	wgpuAdapterRequestDevice(adapter, &device_desc, handle_request_device, &this->device);
	ERR_FAIL_COND_V(!this->device, FAILED);

	queue = wgpuDeviceGetQueue(device);
	ERR_FAIL_COND_V(!this->queue, FAILED);

	capabilties = (RenderingDeviceDriver::Capabilities){
		// TODO: This information is not accurate, see modules/glslang/register_types.cpp:78.
		.device_family = DEVICE_WEBGPU,
		.version_major = 0,
		.version_minor = 19,
	};

	return OK;
}

/*****************/
/**** BUFFERS ****/
/*****************/

RenderingDeviceDriverWebGpu::BufferID RenderingDeviceDriverWebGpu::buffer_create(uint64_t p_size, BitField<BufferUsageBits> p_usage, MemoryAllocationType _p_allocation_type) {
	WGPUBufferUsage usage = webgpu_buffer_usage_from_rd(p_usage);
	uint32_t map_mode = 0;
	bool is_transfer_buffer = false;

	if (usage & WGPUBufferUsage_MapRead) {
		map_mode |= WGPUMapMode_Read;
		is_transfer_buffer = true;
	}
	if (usage & WGPUBufferUsage_MapWrite) {
		map_mode |= WGPUMapMode_Write;
		is_transfer_buffer = true;
	}

	WGPUBufferDescriptor desc = (WGPUBufferDescriptor){
		.usage = usage,
		.size = p_size,
		.mappedAtCreation = is_transfer_buffer,
	};
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);

	BufferInfo *buffer_info = memnew(BufferInfo);
	buffer_info->size = p_size;
	buffer_info->buffer = buffer;
	buffer_info->map_mode = (WGPUMapMode)map_mode;
	buffer_info->is_transfer_first_map = is_transfer_buffer;

	return BufferID(buffer_info);
}

bool RenderingDeviceDriverWebGpu::buffer_set_texel_format(BufferID p_buffer, DataFormat p_format) {
	// TODO
	return true;
}

void RenderingDeviceDriverWebGpu::buffer_free(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	wgpuBufferRelease(buffer_info->buffer);
	memdelete(buffer_info);
}

uint64_t RenderingDeviceDriverWebGpu::buffer_get_allocation_size(BufferID p_buffer) {
	// TODO
	return 0;
}

static void handle_buffer_map(WGPUBufferMapAsyncStatus status, void *userdata) {
	ERR_FAIL_COND_V_MSG(
			status != WGPUBufferMapAsyncStatus_Success, (void)0,
			vformat("Failed to map buffer"));
}

uint8_t *RenderingDeviceDriverWebGpu::buffer_map(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	uint64_t offset = 0;
	uint64_t size = buffer_info->size;

	if (!buffer_info->is_transfer_first_map) {
		wgpuBufferMapAsync(
				buffer_info->buffer, buffer_info->map_mode, offset, size, handle_buffer_map, nullptr);
		wgpuDevicePoll(device, true, nullptr);
	} else {
		buffer_info->is_transfer_first_map = false;
	}
	const void *data = wgpuBufferGetConstMappedRange(buffer_info->buffer, offset, size);
	return (uint8_t *)data;
}

void RenderingDeviceDriverWebGpu::buffer_unmap(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	wgpuBufferUnmap(buffer_info->buffer);
}

/*****************/
/**** TEXTURE ****/
/*****************/

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create(const TextureFormat &p_format, const TextureView &p_view) {
	WGPUFlags usage_bits = WGPUTextureUsage_None;
	if ((p_format.usage_bits & TEXTURE_USAGE_SAMPLING_BIT)) {
		usage_bits |= WGPUTextureUsage_TextureBinding;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ||
			(p_format.usage_bits & TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) ||
			(p_format.usage_bits & TEXTURE_USAGE_INPUT_ATTACHMENT_BIT)) {
		usage_bits |= WGPUTextureUsage_RenderAttachment;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_CAN_UPDATE_BIT) || (p_format.usage_bits & TEXTURE_USAGE_CAN_COPY_TO_BIT)) {
		usage_bits |= WGPUTextureUsage_CopyDst;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_CAN_COPY_FROM_BIT)) {
		usage_bits |= WGPUTextureUsage_CopySrc;
	}
	if ((p_format.usage_bits & TEXTURE_USAGE_STORAGE_BIT)) {
		usage_bits |= WGPUTextureUsage_StorageBinding;
	}
	WGPUTextureUsage usage = (WGPUTextureUsage)usage_bits;

	WGPUExtent3D size;
	size.width = p_format.width;
	size.height = p_format.height;
	size.depthOrArrayLayers = 1;
	WGPUTextureDimension dimension;
	bool is_using_depth = false;

	if (p_format.texture_type == TEXTURE_TYPE_1D) {
		dimension = WGPUTextureDimension_1D;
	} else if (p_format.texture_type == TEXTURE_TYPE_2D) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_3D) {
		size.depthOrArrayLayers = p_format.depth;
		is_using_depth = true;
		dimension = WGPUTextureDimension_3D;
	} else if (p_format.texture_type == TEXTURE_TYPE_1D_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_1D;
	} else if (p_format.texture_type == TEXTURE_TYPE_2D_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_CUBE) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	} else if (p_format.texture_type == TEXTURE_TYPE_CUBE_ARRAY) {
		size.depthOrArrayLayers = p_format.array_layers;
		dimension = WGPUTextureDimension_2D;
	}

	if (p_format.samples > TextureSamples::TEXTURE_SAMPLES_1) {
		usage_bits |= WGPUTextureUsage_RenderAttachment;
	}

	uint32_t mip_level_count = p_format.mipmaps ? p_format.mipmaps : 1;

	// TODO: Assert that p_format.samples follows this behavior.
	uint32_t sample_count = pow(2, (uint32_t)p_format.samples);

	Vector<WGPUTextureFormat> view_formats;

	for (int i = 0; i < p_format.shareable_formats.size(); i++) {
		DataFormat format = p_format.shareable_formats[i];
		view_formats.push_back(webgpu_texture_format_from_rd(format));
	}

	WGPUTextureDescriptor texture_desc = (WGPUTextureDescriptor){
		.usage = usage,
		.dimension = dimension,
		.size = size,
		.format = webgpu_texture_format_from_rd(p_format.format),
		.mipLevelCount = mip_level_count,
		.sampleCount = sample_count,
		.viewFormatCount = (size_t)view_formats.size(),
		.viewFormats = view_formats.ptr(),
	};

	WGPUTexture texture = wgpuDeviceCreateTexture(device, &texture_desc);

	WGPUTextureViewDescriptor texture_view_desc = (WGPUTextureViewDescriptor){
		.format = webgpu_texture_format_from_rd(p_view.format),
		.mipLevelCount = texture_desc.mipLevelCount,
		.arrayLayerCount = is_using_depth ? 1 : texture_desc.size.depthOrArrayLayers,
	};

	WGPUTextureView view = wgpuTextureCreateView(texture, &texture_view_desc);

	TextureInfo *texture_info = memnew(TextureInfo);
	texture_info->texture = texture;
	texture_info->is_original_texture = true;
	texture_info->view = view;
	texture_info->width = size.width;
	texture_info->height = size.height;
	texture_info->depth_or_array = size.depthOrArrayLayers;
	texture_info->is_using_depth = is_using_depth;
	texture_info->mip_level_count = mip_level_count;

	return TextureID(texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil) {}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared(TextureID p_original_texture, const TextureView &p_view) {
	TextureInfo *texture_info = (TextureInfo *)p_original_texture.id;

	WGPUTextureViewDescriptor texture_view_desc = (WGPUTextureViewDescriptor){
		.format = webgpu_texture_format_from_rd(p_view.format),
		.mipLevelCount = texture_info->mip_level_count,
		.arrayLayerCount = texture_info->is_using_depth ? 1 : texture_info->depth_or_array,
	};

	WGPUTextureView view = wgpuTextureCreateView(texture_info->texture, &texture_view_desc);

	TextureInfo *new_texture_info = memnew(TextureInfo);
	*new_texture_info = *texture_info;
	new_texture_info->view = view;
	new_texture_info->is_original_texture = false;

	return TextureID(new_texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) {}

void RenderingDeviceDriverWebGpu::texture_free(TextureID p_texture) {
	TextureInfo *texture_info = (TextureInfo *)p_texture.id;
	if (texture_info->is_original_texture) {
		wgpuTextureRelease(texture_info->texture);
	}
	wgpuTextureViewRelease(texture_info->view);
	memdelete(texture_info);
}

uint64_t RenderingDeviceDriverWebGpu::texture_get_allocation_size(TextureID p_texture) {
	// TODO
	return 1;
}

void RenderingDeviceDriverWebGpu::texture_get_copyable_layout(TextureID p_texture, const TextureSubresource &p_subresource, TextureCopyableLayout *r_layout) {}
uint8_t *RenderingDeviceDriverWebGpu::texture_map(TextureID p_texture, const TextureSubresource &p_subresource) {}
void RenderingDeviceDriverWebGpu::texture_unmap(TextureID p_texture) {}

BitField<RenderingDeviceDriver::TextureUsageBits> RenderingDeviceDriverWebGpu::texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) {
	// TODO: Read this https://www.w3.org/TR/webgpu/#texture-format-caps
	BitField<RDD::TextureUsageBits> supported = INT64_MAX;
	return supported;
}

/*****************/
/**** SAMPLER ****/
/*****************/

RenderingDeviceDriver::SamplerID RenderingDeviceDriverWebGpu::sampler_create(const SamplerState &p_state) {
	// STUB: Samplers with anisotropy enabled cannot support nearest filtering.
	// See https://gpuweb.github.io/gpuweb/#sampler-creation
	WGPUSamplerDescriptor sampler_desc = (WGPUSamplerDescriptor){
		.addressModeU = webgpu_address_mode_from_rd(p_state.repeat_u),
		.addressModeV = webgpu_address_mode_from_rd(p_state.repeat_v),
		.addressModeW = webgpu_address_mode_from_rd(p_state.repeat_w),
		.magFilter = p_state.use_anisotropy ? WGPUFilterMode_Linear : webgpu_filter_mode_from_rd(p_state.mag_filter),
		.minFilter = p_state.use_anisotropy ? WGPUFilterMode_Linear : webgpu_filter_mode_from_rd(p_state.min_filter),
		.mipmapFilter = p_state.use_anisotropy ? WGPUMipmapFilterMode_Linear : webgpu_mipmap_filter_mode_from_rd(p_state.mip_filter),
		.lodMinClamp = p_state.min_lod,
		.lodMaxClamp = p_state.max_lod,
		.compare = p_state.enable_compare ? webgpu_compare_mode_from_rd(p_state.compare_op) : WGPUCompareFunction_Always,
		.maxAnisotropy = p_state.use_anisotropy ? (uint16_t)p_state.anisotropy_max : (uint16_t)1,
	};

	WGPUSampler sampler = wgpuDeviceCreateSampler(device, &sampler_desc);
	return SamplerID(sampler);
}

void RenderingDeviceDriverWebGpu::sampler_free(SamplerID p_sampler) {
	WGPUSampler sampler = (WGPUSampler)p_sampler.id;
	wgpuSamplerRelease(sampler);
}

bool RenderingDeviceDriverWebGpu::sampler_is_format_supported_for_filter(DataFormat p_format, SamplerFilter p_filter) {}

/**********************/
/**** VERTEX ARRAY ****/
/**********************/

// NOTE: The attributes in `p_vertex_attribs` must be in order.
RenderingDeviceDriver::VertexFormatID RenderingDeviceDriverWebGpu::vertex_format_create(VectorView<VertexAttribute> p_vertex_attribs) {
	WGPUVertexAttribute *vertex_attributes = memnew_arr(WGPUVertexAttribute, p_vertex_attribs.size());
	uint64_t array_stride = 0;
	for (int i = 0; i < p_vertex_attribs.size(); i++) {
		VertexAttribute attrib = p_vertex_attribs[i];
		vertex_attributes[i] = (WGPUVertexAttribute){
			.format = webgpu_vertex_format_from_rd(attrib.format),
			.offset = attrib.offset,
			.shaderLocation = attrib.location,
		};
		array_stride += attrib.stride;
	}

	WGPUVertexStepMode step_mode = p_vertex_attribs[0].frequency == VertexFrequency::VERTEX_FREQUENCY_VERTEX ? WGPUVertexStepMode_Vertex : WGPUVertexStepMode_Instance;

	WGPUVertexBufferLayout *layout = memnew(WGPUVertexBufferLayout);
	layout->attributes = vertex_attributes;
	layout->attributeCount = p_vertex_attribs.size();
	layout->stepMode = step_mode;
	layout->arrayStride = array_stride;
	return VertexFormatID(layout);
}

void RenderingDeviceDriverWebGpu::vertex_format_free(VertexFormatID p_vertex_format) {
	WGPUVertexBufferLayout *layout = (WGPUVertexBufferLayout *)p_vertex_format.id;
	memdelete_arr(layout->attributes);
	memdelete(layout);
}

/******************/
/**** BARRIERS ****/
/******************/

void RenderingDeviceDriverWebGpu::command_pipeline_barrier(
		CommandBufferID p_cmd_buffer,
		BitField<PipelineStageBits> p_src_stages,
		BitField<PipelineStageBits> p_dst_stages,
		VectorView<MemoryBarrier> p_memory_barriers,
		VectorView<BufferBarrier> p_buffer_barriers,
		VectorView<TextureBarrier> p_texture_barriers) {}

/****************/
/**** FENCES ****/
/****************/

RenderingDeviceDriver::FenceID RenderingDeviceDriverWebGpu::fence_create() {
	// The usage of fences in godot to sync frames is already handled by WebGpu.
	return FenceID(1);
}

Error RenderingDeviceDriverWebGpu::fence_wait(FenceID _p_fence) {
	return OK;
}

void RenderingDeviceDriverWebGpu::fence_free(FenceID p_fence) {
	// Empty.
}

/********************/
/**** SEMAPHORES ****/
/********************/

RenderingDeviceDriver::SemaphoreID RenderingDeviceDriverWebGpu::semaphore_create() {
	// The usage of fences in godot to sync frames is already handled by WebGpu.
	return SemaphoreID(1);
}

void RenderingDeviceDriverWebGpu::semaphore_free(SemaphoreID _p_semaphore) {
	// Empty.
}

/******************/
/**** COMMANDS ****/
/******************/

// ----- QUEUE FAMILY -----

RenderingDeviceDriver::CommandQueueFamilyID RenderingDeviceDriverWebGpu::command_queue_family_get(BitField<CommandQueueFamilyBits> _p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID _p_surface) {
	// WebGpu has no concept of queue families, so this value is unused.
	return CommandQueueFamilyID(1);
}

// ----- QUEUE -----

RenderingDeviceDriver::CommandQueueID RenderingDeviceDriverWebGpu::command_queue_create(CommandQueueFamilyID _p_cmd_queue_family, bool _p_identify_as_main_queue) {
	// WebGpu has only one queue, so this value is unused.
	return CommandQueueID(1);
}

Error RenderingDeviceDriverWebGpu::command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) {}

void RenderingDeviceDriverWebGpu::command_queue_free(CommandQueueID _p_cmd_queue) {
	// Empty.
}

// ----- POOL -----

RenderingDeviceDriver::CommandPoolID RenderingDeviceDriverWebGpu::command_pool_create(CommandQueueFamilyID _p_cmd_queue_family, CommandBufferType _p_cmd_buffer_type) {
	// WebGpu has no concept of command pools.
	// However, we will free command encoders with command_pool_free because command buffers are tied to the lifetime of command pools.
	return CommandPoolID(1);
}

void RenderingDeviceDriverWebGpu::command_pool_free(CommandPoolID _p_cmd_pool) {
	// Empty.
}

// ----- BUFFER -----

RenderingDeviceDriver::CommandBufferID RenderingDeviceDriverWebGpu::command_buffer_create(CommandPoolID _p_cmd_pool) {
	command_encoders.push_back(nullptr);
	return CommandBufferID(command_encoders.size() - 1 + 1);
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer - 1 < command_encoders.size());

	WGPUCommandEncoder &encoder_spot = command_encoders[p_cmd_buffer - 1];
	DEV_ASSERT(encoder_spot == nullptr);

	WGPUCommandEncoderDescriptor desc = (WGPUCommandEncoderDescriptor){};
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &desc);

	encoder_spot = encoder;

	return false;
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) {
}

void RenderingDeviceDriverWebGpu::command_buffer_end(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer - 1 < command_encoders.size());

	WGPUCommandEncoder &encoder = command_encoders[p_cmd_buffer - 1];
	DEV_ASSERT(encoder == nullptr);

	wgpuCommandEncoderRelease(encoder);
}

void RenderingDeviceDriverWebGpu::command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) {}

/********************/
/**** SWAP CHAIN ****/
/********************/

RenderingDeviceDriver::SwapChainID RenderingDeviceDriverWebGpu::swap_chain_create(RenderingContextDriver::SurfaceID p_surface) {
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)p_surface;

	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);

	// NOTE: This is not the best way of getting the format of the surface.
	WGPUTextureFormat surface_format = wgpuSurfaceGetPreferredFormat(surface->surface, adapter);

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>({ (RenderPassAttachmentInfo){
			.format = surface_format,
			.sample_count = 1,
			.load_op = WGPULoadOp_Clear,
			.store_op = WGPUStoreOp_Store,
			.stencil_load_op = WGPULoadOp_Undefined,
			.stencil_store_op = WGPUStoreOp_Undefined,
	} });
	// TODO: The multiview feature is currently disabled, so I will ignore this.
	render_pass_info->view_count = 1;

	SwapChainInfo *swapchain_info = memnew(SwapChainInfo);
	swapchain_info->surface = p_surface;
	swapchain_info->render_pass = RenderPassID(render_pass_info);

	return SwapChainID(swapchain_info);
}

Error RenderingDeviceDriverWebGpu::swap_chain_resize(CommandQueueID _p_cmd_queue, SwapChainID _p_swap_chain, uint32_t _p_desired_framebuffer_count) {
	return OK;
}

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) {
	return FramebufferID(1);
}

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::swap_chain_get_render_pass(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	return swapchain_info->render_pass;
}

// NOTE: In theory, this function's result doesn't matter.
// We take this to create a framebuffer attachment that we never end up using since WebGpu does not support framebuffers.
RenderingDeviceDriver::DataFormat RenderingDeviceDriverWebGpu::swap_chain_get_format(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;
	// NOTE: This is not the best way of getting the format of the surface.
	// return wgpuSurfaceGetPreferredFormat(surface->surface, adapter);
	// TODO: I don't want to write a conversion just for this one useless function.
	return DATA_FORMAT_ASTC_4x4_SRGB_BLOCK;
}

void RenderingDeviceDriverWebGpu::swap_chain_free(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;

	memdelete((RenderPassInfo *)swapchain_info->render_pass.id);
	memdelete(swapchain_info);
}

/*********************/
/**** FRAMEBUFFER ****/
/*********************/

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t p_width, uint32_t p_height) {
	// TODO: impl
	print_error("TODO --> framebuffer_create");
	exit(1);
}

void RenderingDeviceDriverWebGpu::framebuffer_free(FramebufferID p_framebuffer) {}

/****************/
/**** SHADER ****/
/****************/

String RenderingDeviceDriverWebGpu::shader_get_binary_cache_key() {
	// TODO
	return "";
}

Vector<uint8_t> RenderingDeviceDriverWebGpu::shader_compile_binary_from_spirv(VectorView<ShaderStageSPIRVData> p_spirv, const String &p_shader_name) {
	// This code is mostly taken from the vulkan device driver.
	ShaderReflection shader_refl;
	if (_reflect_spirv(p_spirv, shader_refl) != OK) {
		return Vector<uint8_t>();
	}

	ShaderBinary::Data binary_data;
	Vector<Vector<ShaderBinary::DataBinding>> uniforms; // Set bindings.
	Vector<ShaderBinary::SpecializationConstant> specialization_constants;
	{
		binary_data.specialization_constants_count = shader_refl.specialization_constants.size();
		binary_data.is_compute = shader_refl.is_compute;
		binary_data.compute_local_size[0] = shader_refl.compute_local_size[0];
		binary_data.compute_local_size[1] = shader_refl.compute_local_size[1];
		binary_data.compute_local_size[2] = shader_refl.compute_local_size[2];
		binary_data.set_count = shader_refl.uniform_sets.size();
		binary_data.push_constant_size = shader_refl.push_constant_size;
		for (uint32_t i = 0; i < SHADER_STAGE_MAX; i++) {
			if (shader_refl.push_constant_stages.has_flag((ShaderStage)(1 << i))) {
				binary_data.push_constant_stages |= webgpu_shader_stage_from_rd((ShaderStage)i);
			}
		}

		for (const Vector<ShaderUniform> &set_refl : shader_refl.uniform_sets) {
			Vector<ShaderBinary::DataBinding> set_bindings;
			for (const ShaderUniform &uniform_refl : set_refl) {
				ShaderBinary::DataBinding binding;
				binding.type = (uint32_t)uniform_refl.type;
				binding.binding = uniform_refl.binding;
				binding.stages = (uint32_t)uniform_refl.stages;
				binding.length = uniform_refl.length;
				binding.writable = (uint32_t)uniform_refl.writable;

				binding.texture_is_multisample = uniform_refl.texture_is_multisample;
				binding.image_format = uniform_refl.image_format;
				binding.image_access = uniform_refl.image_access;
				binding.texture_image_type = uniform_refl.texture_image_type;

				set_bindings.push_back(binding);
			}
			uniforms.push_back(set_bindings);
		}

		for (const ShaderSpecializationConstant &refl_sc : shader_refl.specialization_constants) {
			ShaderBinary::SpecializationConstant spec_constant;
			spec_constant.constant_id = refl_sc.constant_id;
			spec_constant.int_value = refl_sc.int_value;

			CharString ascii_name = refl_sc.name.ascii();
			ERR_FAIL_COND_V(ascii_name.size() < ShaderBinary::SpecializationConstant::OVERRIDE_CONSTANT_STRLEN, Vector<uint8_t>());
			strncpy(spec_constant.value_name, ascii_name.ptr(), ShaderBinary::SpecializationConstant::OVERRIDE_CONSTANT_STRLEN);

			specialization_constants.push_back(spec_constant);
		}
	}

	Vector<Vector<uint8_t>> compressed_stages;
	Vector<uint32_t> smolv_size;
	Vector<uint32_t> zstd_size;

	uint32_t stages_binary_size = 0;

	bool strip_debug = false;

	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		smolv::ByteArray smolv;
		if (!smolv::Encode(p_spirv[i].spirv.ptr(), p_spirv[i].spirv.size(), smolv, strip_debug ? smolv::kEncodeFlagStripDebugInfo : 0)) {
			ERR_FAIL_V_MSG(Vector<uint8_t>(), "Error compressing shader stage :" + String(SHADER_STAGE_NAMES[p_spirv[i].shader_stage]));
		} else {
			smolv_size.push_back(smolv.size());
			// zstd.
			{
				Vector<uint8_t> zstd;
				zstd.resize(Compression::get_max_compressed_buffer_size(smolv.size(), Compression::MODE_ZSTD));
				int dst_size = Compression::compress(zstd.ptrw(), &smolv[0], smolv.size(), Compression::MODE_ZSTD);

				if (dst_size > 0 && (uint32_t)dst_size < smolv.size()) {
					zstd_size.push_back(dst_size);
					zstd.resize(dst_size);
					compressed_stages.push_back(zstd);
				} else {
					Vector<uint8_t> smv;
					smv.resize(smolv.size());
					memcpy(smv.ptrw(), &smolv[0], smolv.size());
					// Not using zstd.
					zstd_size.push_back(0);
					compressed_stages.push_back(smv);
				}
			}
		}
		uint32_t s = compressed_stages[i].size();
		stages_binary_size += STEPIFY(s, 4);
	}

	binary_data.specialization_constants_count = specialization_constants.size();
	binary_data.set_count = uniforms.size();
	binary_data.stage_count = p_spirv.size();

	CharString shader_name_utf = p_shader_name.utf8();

	binary_data.shader_name_len = shader_name_utf.length();

	// Header + version + main datasize;.
	uint32_t total_size = sizeof(uint32_t) * 3;

	total_size += sizeof(ShaderBinary::Data);

	total_size += STEPIFY(binary_data.shader_name_len, 4);
	for (int i = 0; i < uniforms.size(); i++) {
		total_size += sizeof(uint32_t);
		total_size += uniforms[i].size() * sizeof(ShaderBinary::DataBinding);
	}

	total_size += sizeof(ShaderBinary::SpecializationConstant) * specialization_constants.size();
	total_size += compressed_stages.size() * sizeof(uint32_t) * 3; // Sizes.
	total_size += stages_binary_size;

	Vector<uint8_t> ret;
	ret.resize(total_size);
	{
		uint32_t offset = 0;
		uint8_t *binptr = ret.ptrw();
		binptr[0] = 'G';
		binptr[1] = 'S';
		binptr[2] = 'B';
		binptr[3] = 'D';
		offset += 4;
		encode_uint32(ShaderBinary::VERSION, binptr + offset);
		offset += sizeof(uint32_t);
		encode_uint32(sizeof(ShaderBinary::Data), binptr + offset);
		offset += sizeof(uint32_t);
		memcpy(binptr + offset, &binary_data, sizeof(ShaderBinary::Data));
		offset += sizeof(ShaderBinary::Data);

#define ADVANCE_OFFSET_WITH_ALIGNMENT(m_bytes)                         \
	{                                                                  \
		offset += m_bytes;                                             \
		uint32_t padding = STEPIFY(m_bytes, 4) - m_bytes;              \
		memset(binptr + offset, 0, padding); /* Avoid garbage data. */ \
		offset += padding;                                             \
	}

		if (binary_data.shader_name_len > 0) {
			memcpy(binptr + offset, shader_name_utf.ptr(), binary_data.shader_name_len);
			ADVANCE_OFFSET_WITH_ALIGNMENT(binary_data.shader_name_len);
		}

		for (int i = 0; i < uniforms.size(); i++) {
			int count = uniforms[i].size();
			encode_uint32(count, binptr + offset);
			offset += sizeof(uint32_t);
			if (count > 0) {
				memcpy(binptr + offset, uniforms[i].ptr(), sizeof(ShaderBinary::DataBinding) * count);
				offset += sizeof(ShaderBinary::DataBinding) * count;
			}
		}

		if (specialization_constants.size()) {
			memcpy(binptr + offset, specialization_constants.ptr(), sizeof(ShaderBinary::SpecializationConstant) * specialization_constants.size());
			offset += sizeof(ShaderBinary::SpecializationConstant) * specialization_constants.size();
		}

		for (int i = 0; i < compressed_stages.size(); i++) {
			encode_uint32(p_spirv[i].shader_stage, binptr + offset);
			offset += sizeof(uint32_t);
			encode_uint32(smolv_size[i], binptr + offset);
			offset += sizeof(uint32_t);
			encode_uint32(zstd_size[i], binptr + offset);
			offset += sizeof(uint32_t);
			memcpy(binptr + offset, compressed_stages[i].ptr(), compressed_stages[i].size());
			ADVANCE_OFFSET_WITH_ALIGNMENT(compressed_stages[i].size());
		}

		DEV_ASSERT(offset == (uint32_t)ret.size());
	}

	return ret;
}

RenderingDeviceDriver::ShaderID RenderingDeviceDriverWebGpu::shader_create_from_bytecode(const Vector<uint8_t> &p_shader_binary, ShaderDescription &r_shader_desc, String &r_name) {
	r_shader_desc = {};

	// TODO: We allocate memory and call wgpuDeviceCreate*. Perhaps, we should free that memory if we fail.
	ShaderInfo *shader_info = memnew(ShaderInfo);

	const uint8_t *binptr = p_shader_binary.ptr();
	uint32_t binsize = p_shader_binary.size();

	uint32_t read_offset = 0;

	// Consistency check.
	ERR_FAIL_COND_V(binsize < sizeof(uint32_t) * 3 + sizeof(ShaderBinary::Data), ShaderID());
	ERR_FAIL_COND_V(binptr[0] != 'G' || binptr[1] != 'S' || binptr[2] != 'B' || binptr[3] != 'D', ShaderID());
	uint32_t bin_version = decode_uint32(binptr + 4);
	ERR_FAIL_COND_V(bin_version != ShaderBinary::VERSION, ShaderID());

	uint32_t bin_data_size = decode_uint32(binptr + 8);
	const ShaderBinary::Data &binary_data = *(reinterpret_cast<const ShaderBinary::Data *>(binptr + 12));

	r_shader_desc.push_constant_size = binary_data.push_constant_size;

	r_shader_desc.vertex_input_mask = binary_data.vertex_input_mask;
	r_shader_desc.fragment_output_mask = binary_data.fragment_output_mask;

	r_shader_desc.is_compute = binary_data.is_compute;
	r_shader_desc.compute_local_size[0] = binary_data.compute_local_size[0];
	r_shader_desc.compute_local_size[1] = binary_data.compute_local_size[1];
	r_shader_desc.compute_local_size[2] = binary_data.compute_local_size[2];

	read_offset += sizeof(uint32_t) * 3 + bin_data_size;

	if (binary_data.shader_name_len) {
		r_name.parse_utf8((const char *)(binptr + read_offset), binary_data.shader_name_len);
		read_offset += STEPIFY(binary_data.shader_name_len, 4);
	}

	Vector<Vector<WGPUBindGroupLayoutEntry>> bind_group_layout_entries;

	r_shader_desc.uniform_sets.resize(binary_data.set_count);
	bind_group_layout_entries.resize(binary_data.set_count);

	for (uint32_t i = 0; i < binary_data.set_count; i++) {
		ERR_FAIL_COND_V(read_offset + sizeof(uint32_t) >= binsize, ShaderID());
		uint32_t binding_count = decode_uint32(binptr + read_offset);
		read_offset += sizeof(uint32_t);
		const ShaderBinary::DataBinding *set_ptr = reinterpret_cast<const ShaderBinary::DataBinding *>(binptr + read_offset);
		uint32_t binding_size = binding_count * sizeof(ShaderBinary::DataBinding);
		ERR_FAIL_COND_V(read_offset + binding_size >= binsize, ShaderID());

		for (uint32_t j = 0; j < binding_count; j++) {
			ShaderUniform info;
			info.type = UniformType(set_ptr[j].type);
			info.writable = set_ptr[j].writable;
			info.length = set_ptr[j].length;
			info.binding = set_ptr[j].binding;
			info.stages = set_ptr[j].stages;
			info.texture_is_multisample = set_ptr[j].texture_is_multisample;
			info.image_format = (DataFormat)set_ptr[j].image_format;
			info.image_access = (ShaderUniform::ImageAccess)set_ptr[j].image_access;
			info.texture_image_type = (TextureType)set_ptr[j].texture_image_type;

			WGPUBindGroupLayoutEntry layout_entry = {};
			layout_entry.binding = set_ptr[j].binding;
			for (uint32_t k = 0; k < SHADER_STAGE_MAX; k++) {
				if ((set_ptr[j].stages & (1 << k))) {
					layout_entry.visibility |= webgpu_shader_stage_from_rd((ShaderStage)k);
				}
			}

			switch (info.type) {
				case UNIFORM_TYPE_SAMPLER: {
					layout_entry.sampler = (WGPUSamplerBindingLayout){
						.type = WGPUSamplerBindingType_NonFiltering
					};
				} break;
				case UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
					// TODO: WebGpu doesn't appear to support combined image samplers?
					print_error("WebGpu UNIFORM_TYPE_SAMPLER_WITH_TEXTURE not supported");
					return ShaderID();
				} break;
				case UNIFORM_TYPE_TEXTURE: {
					layout_entry.texture = (WGPUTextureBindingLayout){
						// NOTE: Other texture types don't appear to be supported by spirv reflect, but utexture2D does appear once in godot.
						.sampleType = WGPUTextureSampleType_Float,
						.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type),
						.multisampled = info.texture_is_multisample,
					};
				} break;
				case UNIFORM_TYPE_IMAGE: {
					WGPUStorageTextureAccess access;
					switch (info.image_access) {
						case ShaderUniform::ImageAccess::ReadWrite:
							access = WGPUStorageTextureAccess_ReadWrite;
							break;
						case ShaderUniform::ImageAccess::ReadOnly:
							access = WGPUStorageTextureAccess_ReadOnly;
							break;
						case ShaderUniform::ImageAccess::WriteOnly:
							access = WGPUStorageTextureAccess_WriteOnly;
							break;
					}

					layout_entry.storageTexture = (WGPUStorageTextureBindingLayout){
						.access = access,
						.format = webgpu_texture_format_from_rd(info.image_format),
						.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type),
					};
				} break;
				case UNIFORM_TYPE_INPUT_ATTACHMENT: {
					layout_entry.texture = (WGPUTextureBindingLayout){
						// NOTE: Other texture types don't appear to be supported by spirv reflect, but utexture2D does appear once in godot.
						.sampleType = WGPUTextureSampleType_Float,
						.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type),
						.multisampled = info.texture_is_multisample,
					};
				} break;
				case UNIFORM_TYPE_UNIFORM_BUFFER: {
					layout_entry.buffer = (WGPUBufferBindingLayout){
						.type = WGPUBufferBindingType_Uniform,
						// Godot doesn't support dynamic offset
						.hasDynamicOffset = false,
					};
				} break;
				case UNIFORM_TYPE_STORAGE_BUFFER: {
					layout_entry.buffer = (WGPUBufferBindingLayout){
						.type = WGPUBufferBindingType_Storage,
						// Godot doesn't support dynamic offset
						.hasDynamicOffset = false,
					};
				} break;
				case UNIFORM_TYPE_TEXTURE_BUFFER:
				case UNIFORM_TYPE_IMAGE_BUFFER:
					print_error("WebGpu UNIFORM_TYPE_TEXTURE_BUFFER and UNIFORM_TYPE_IMAGE_BUFFER not supported.");
					return ShaderID();
					break;
				default: {
					DEV_ASSERT(false);
				}
			}

			r_shader_desc.uniform_sets.write[i].push_back(info);
			bind_group_layout_entries.write[i].push_back(layout_entry);
		}

		read_offset += binding_size;
	}

	ERR_FAIL_COND_V(read_offset + binary_data.specialization_constants_count * sizeof(ShaderBinary::SpecializationConstant) >= binsize, ShaderID());

	r_shader_desc.specialization_constants.resize(binary_data.specialization_constants_count);
	for (uint32_t i = 0; i < binary_data.specialization_constants_count; i++) {
		const ShaderBinary::SpecializationConstant &src_sc = *(reinterpret_cast<const ShaderBinary::SpecializationConstant *>(binptr + read_offset));
		ShaderSpecializationConstant sc;
		sc.constant_id = src_sc.constant_id;
		sc.int_value = src_sc.int_value;
		sc.name = String((char *)src_sc.value_name);
		r_shader_desc.specialization_constants.write[i] = sc;

		shader_info->override_keys[sc.constant_id] = sc.name;

		read_offset += sizeof(ShaderBinary::SpecializationConstant);
	}

	Vector<Vector<uint8_t>> stages_spirv;
	stages_spirv.resize(binary_data.stage_count);
	r_shader_desc.stages.resize(binary_data.stage_count);

	for (uint32_t i = 0; i < binary_data.stage_count; i++) {
		ERR_FAIL_COND_V(read_offset + sizeof(uint32_t) * 3 >= binsize, ShaderID());

		uint32_t stage = decode_uint32(binptr + read_offset);
		read_offset += sizeof(uint32_t);
		uint32_t smolv_size = decode_uint32(binptr + read_offset);
		read_offset += sizeof(uint32_t);
		uint32_t zstd_size = decode_uint32(binptr + read_offset);
		read_offset += sizeof(uint32_t);

		uint32_t buf_size = (zstd_size > 0) ? zstd_size : smolv_size;

		Vector<uint8_t> smolv;
		const uint8_t *src_smolv = nullptr;

		if (zstd_size > 0) {
			// Decompress to smolv.
			smolv.resize(smolv_size);
			int dec_smolv_size = Compression::decompress(smolv.ptrw(), smolv.size(), binptr + read_offset, zstd_size, Compression::MODE_ZSTD);
			ERR_FAIL_COND_V(dec_smolv_size != (int32_t)smolv_size, ShaderID());
			src_smolv = smolv.ptr();
		} else {
			src_smolv = binptr + read_offset;
		}

		Vector<uint8_t> &spirv = stages_spirv.ptrw()[i];
		uint32_t spirv_size = smolv::GetDecodedBufferSize(src_smolv, smolv_size);
		spirv.resize(spirv_size);
		if (!smolv::Decode(src_smolv, smolv_size, spirv.ptrw(), spirv_size)) {
			ERR_FAIL_V_MSG(ShaderID(), "Malformed smolv input uncompressing shader stage:" + String(SHADER_STAGE_NAMES[stage]));
		}

		r_shader_desc.stages.set(i, ShaderStage(stage));

		buf_size = STEPIFY(buf_size, 4);
		read_offset += buf_size;
		ERR_FAIL_COND_V(read_offset > binsize, ShaderID());
	}

	ERR_FAIL_COND_V(read_offset != binsize, ShaderID());

	for (int i = 0; i < r_shader_desc.stages.size(); i++) {
		WGPUShaderModuleSPIRVDescriptor shader_module_spirv_desc = (WGPUShaderModuleSPIRVDescriptor){
			.chain = (WGPUChainedStruct){
					.sType = WGPUSType_ShaderModuleSPIRVDescriptor,
			},
			.codeSize = (uint32_t)stages_spirv[i].size() / 4,
			.code = (const uint32_t *)stages_spirv[i].ptr(),
		};

		WGPUShaderModuleDescriptor shader_module_desc = (WGPUShaderModuleDescriptor){
			.nextInChain = (const WGPUChainedStruct *)&shader_module_spirv_desc,
			.hintCount = 0,
		};

		WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &shader_module_desc);

		ERR_FAIL_COND_V(!shader_module, ShaderID());

		shader_info->shader_modules.push_back(shader_module);
	}

	DEV_ASSERT((uint32_t)vk_set_bindings.size() == binary_data.set_count);
	for (uint32_t i = 0; i < binary_data.set_count; i++) {
		WGPUBindGroupLayoutDescriptor bind_group_layout_desc = (WGPUBindGroupLayoutDescriptor){
			.entryCount = (size_t)bind_group_layout_entries[i].size(),
			.entries = bind_group_layout_entries[i].ptr(),
		};

		WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);

		ERR_FAIL_COND_V(!bind_group_layout, ShaderID());

		shader_info->bind_group_layouts.push_back(bind_group_layout);
	}

	WGPUPushConstantRange push_constant_range;
	size_t push_constant_range_count = 0;

	if (binary_data.push_constant_size) {
		uint32_t stages = WGPUShaderStage_None;
		for (uint32_t k = 0; k < SHADER_STAGE_MAX; k++) {
			if ((binary_data.push_constant_stages & (1 << k))) {
				stages |= webgpu_shader_stage_from_rd((ShaderStage)k);
			}
		}

		push_constant_range = (WGPUPushConstantRange){
			.stages = (WGPUShaderStage)stages,
			.start = 0,
			.end = binary_data.push_constant_size,
		};
	}

	WGPUPipelineLayoutExtras wgpu_pipeline_extras = (WGPUPipelineLayoutExtras){
		.pushConstantRangeCount = push_constant_range_count,
		.pushConstantRanges = &push_constant_range,
	};

	WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = (WGPUPipelineLayoutDescriptor){
		.nextInChain = (WGPUChainedStruct *)&wgpu_pipeline_extras,
		.bindGroupLayoutCount = binary_data.set_count,
		.bindGroupLayouts = shader_info->bind_group_layouts.ptr(),
	};

	// TODO: Implement the usage of specialization constants.

	shader_info->pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_descriptor);
	ERR_FAIL_COND_V(!shader_info->pipeline_layout, ShaderID());

	return ShaderID(shader_info);
}

void RenderingDeviceDriverWebGpu::shader_free(ShaderID p_shader) {
}

/*********************/
/**** UNIFORM SET ****/
/*********************/

RenderingDeviceDriver::UniformSetID RenderingDeviceDriverWebGpu::uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index) {
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	Vector<WGPUBindGroupEntry> entries;

	// Used to allow data stored in entries to live long enough to be used.
	Vector<Vector<WGPUSampler>> owned_samplers;
	Vector<Vector<WGPUTextureView>> owned_texture_views;
	Vector<WGPUBindGroupEntryExtras> owned_chained_extras;

	for (int i = 0; i < p_uniforms.size(); i++) {
		const BoundUniform &uniform = p_uniforms[i];
		WGPUBindGroupEntry entry;
		entry.binding = uniform.binding;
		entry.nextInChain = nullptr;

		switch (uniform.type) {
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER:
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
				Vector<WGPUSampler> uniform_samplers;

				for (uint32_t j = 0; j < uniform.ids.size(); j++) {
					WGPUSampler sampler = (WGPUSampler)uniform.ids[j].id;
					uniform_samplers.push_back(sampler);
				}

				WGPUBindGroupEntryExtras entry_extras = (WGPUBindGroupEntryExtras){
					.samplers = uniform_samplers.ptr(),
					.samplerCount = (size_t)owned_texture_views.size(),
				};
				owned_chained_extras.push_back(entry_extras);
				entry.nextInChain = (WGPUChainedStruct *)&owned_chained_extras[owned_chained_extras.size() - 1];
				owned_samplers.push_back(uniform_samplers);

			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE:
			case RenderingDeviceCommons::UNIFORM_TYPE_INPUT_ATTACHMENT: {
				Vector<WGPUTextureView> uniform_texture_views;

				for (uint32_t j = 0; j < uniform.ids.size(); j++) {
					TextureInfo *texture_info = (TextureInfo *)uniform.ids[j].id;
					uniform_texture_views.push_back(texture_info->view);
				}

				WGPUBindGroupEntryExtras entry_extras = (WGPUBindGroupEntryExtras){
					.textureViews = uniform_texture_views.ptr(),
					.textureViewCount = (size_t)uniform_texture_views.size(),
				};
				owned_chained_extras.push_back(entry_extras);
				entry.nextInChain = (WGPUChainedStruct *)&owned_chained_extras[owned_chained_extras.size() - 1];
				owned_texture_views.push_back(uniform_texture_views);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE_BUFFER:
				CRASH_NOW_MSG("Unimplemented!"); // TODO.
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER: {
				BufferInfo *buffer_info = (BufferInfo *)uniform.ids[0].id;
				entry.buffer = buffer_info->buffer;
				entry.offset = 0;
				entry.size = buffer_info->size;
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_MAX:
				break;
		}
		entries.push_back(entry);
	}

	WGPUBindGroupDescriptor bind_group_desc = (WGPUBindGroupDescriptor){
		.layout = shader_info->bind_group_layouts[p_set_index],
		.entryCount = (size_t)entries.size(),
		.entries = entries.ptr(),
	};

	WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(device, &bind_group_desc);

	// TODO: impl
	/* print_error("TODO --> uniform_set_create"); */
	/* exit(1); */
	return UniformSetID(bind_group);
}

void RenderingDeviceDriverWebGpu::uniform_set_free(UniformSetID p_uniform_set) {
	WGPUBindGroup bind_group = (WGPUBindGroup)p_uniform_set.id;
	wgpuBindGroupRelease(bind_group);
}

// ----- COMMANDS -----

void RenderingDeviceDriverWebGpu::command_uniform_set_prepare_for_use(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {}

/******************/
/**** TRANSFER ****/
/******************/

void RenderingDeviceDriverWebGpu::command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) {}
void RenderingDeviceDriverWebGpu::command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) {}

void RenderingDeviceDriverWebGpu::command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) {}
void RenderingDeviceDriverWebGpu::command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) {}
void RenderingDeviceDriverWebGpu::command_clear_color_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) {}

void RenderingDeviceDriverWebGpu::command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) {}
void RenderingDeviceDriverWebGpu::command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) {}

/******************/
/**** PIPELINE ****/
/******************/

void RenderingDeviceDriverWebGpu::pipeline_free(PipelineID p_pipeline) {}

// ----- BINDING -----

void RenderingDeviceDriverWebGpu::command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_first_index, VectorView<uint32_t> p_data) {}

// ----- CACHE -----

bool RenderingDeviceDriverWebGpu::pipeline_cache_create(const Vector<uint8_t> &_p_data) {
	// WebGpu does not have pipeline caches.
	return false;
}
void RenderingDeviceDriverWebGpu::pipeline_cache_free() {
	// Empty.
}
size_t RenderingDeviceDriverWebGpu::pipeline_cache_query_size() {
	return 0;
}
Vector<uint8_t> RenderingDeviceDriverWebGpu::pipeline_cache_serialize() {
	return Vector<uint8_t>();
}

/*******************/
/**** RENDERING ****/
/*******************/

// ----- SUBPASS -----

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> _p_subpasses, VectorView<SubpassDependency> _p_subpass_dependencies, uint32_t p_view_count) {
	// WebGpu does not have subpasses so we will store this info until we create a render pipeline later.
	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>();
	for (int i = 0; i < p_attachments.size(); i++) {
		Attachment attachment = p_attachments[i];
		RenderPassAttachmentInfo attachment_info = (RenderPassAttachmentInfo){
			.format = webgpu_texture_format_from_rd(attachment.format),
			// TODO: Assert that p_format.samples follows this behavior.
			.sample_count = (uint32_t)pow(2, (uint32_t)attachment.samples),
			.load_op = webgpu_load_op_from_rd(attachment.load_op),
			.store_op = webgpu_store_op_from_rd(attachment.store_op),
			.stencil_load_op = webgpu_load_op_from_rd(attachment.stencil_load_op),
			.stencil_store_op = webgpu_store_op_from_rd(attachment.stencil_store_op),
		};
		render_pass_info->attachments.push_back(attachment_info);
	}

	render_pass_info->view_count = p_view_count;

	return RenderPassID(render_pass_info);
}
void RenderingDeviceDriverWebGpu::render_pass_free(RenderPassID p_render_pass) {
	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	memdelete(render_pass_info);
}

// ----- COMMANDS -----

void RenderingDeviceDriverWebGpu::command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) {}
void RenderingDeviceDriverWebGpu::command_end_render_pass(CommandBufferID p_cmd_buffer) {}
void RenderingDeviceDriverWebGpu::command_next_render_subpass(CommandBufferID p_cmd_buffer, CommandBufferType p_cmd_buffer_type) {}
void RenderingDeviceDriverWebGpu::command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) {}
void RenderingDeviceDriverWebGpu::command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) {}
void RenderingDeviceDriverWebGpu::command_render_clear_attachments(CommandBufferID p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) {}

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {}
void RenderingDeviceDriverWebGpu::command_bind_render_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {}

// Drawing.
void RenderingDeviceDriverWebGpu::command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) {}
void RenderingDeviceDriverWebGpu::command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) {}
void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) {}
void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) {}
void RenderingDeviceDriverWebGpu::command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) {}
void RenderingDeviceDriverWebGpu::command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) {}

// Buffer binding.
void RenderingDeviceDriverWebGpu::command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets) {}
void RenderingDeviceDriverWebGpu::command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) {}

// Dynamic state.
void RenderingDeviceDriverWebGpu::command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) {}
void RenderingDeviceDriverWebGpu::command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) {}

// ----- PIPELINE -----

RenderingDeviceDriver::PipelineID RenderingDeviceDriverWebGpu::render_pipeline_create(
		ShaderID p_shader,
		VertexFormatID p_vertex_format,
		RenderPrimitive p_render_primitive,
		PipelineRasterizationState p_rasterization_state,
		PipelineMultisampleState p_multisample_state,
		PipelineDepthStencilState p_depth_stencil_state,
		PipelineColorBlendState p_blend_state,
		VectorView<int32_t> p_color_attachments,
		BitField<PipelineDynamicStateFlags> p_dynamic_state,
		RenderPassID p_render_pass,
		uint32_t p_render_subpass,
		VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	// TODO: impl
	print_error("TODO --> render_pipeline_create");
	exit(1);
}

/*****************/
/**** COMPUTE ****/
/*****************/

// ----- COMMANDS -----

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {}
void RenderingDeviceDriverWebGpu::command_bind_compute_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {}

// Dispatching.
void RenderingDeviceDriverWebGpu::command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) {}
void RenderingDeviceDriverWebGpu::command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) {}

// ----- PIPELINE -----

RenderingDeviceDriver::PipelineID RenderingDeviceDriverWebGpu::compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	ERR_FAIL_COND_V(shader_info->shader_modules.size() != 1, PipelineID());

	Vector<WGPUConstantEntry> constants;
	for (int i = 0; i < p_specialization_constants.size(); i++) {
		PipelineSpecializationConstant constant = p_specialization_constants[i];
		union {
			int i;
			double d;
		} val;

		val.i = constant.int_value;

		CharString key_name = shader_info->override_keys[constant.constant_id].ascii();
		constants.push_back((WGPUConstantEntry){
				.key = key_name,
				.value = val.d,
		});
	}

	WGPUProgrammableStageDescriptor programmable_stage_desc = (WGPUProgrammableStageDescriptor){
		.module = shader_info->shader_modules[0],
		.entryPoint = "main",
		.constantCount = (size_t)constants.size(),
		.constants = constants.ptr(),
	};

	WGPUComputePipelineDescriptor compute_pipeline_desc = (WGPUComputePipelineDescriptor){
		.layout = shader_info->pipeline_layout,
		.compute = programmable_stage_desc,
	};

	WGPUComputePipeline compute_pipeline = wgpuDeviceCreateComputePipeline(device, &compute_pipeline_desc);

	return PipelineID(compute_pipeline);
}

/*****************/
/**** QUERIES ****/
/*****************/

// ----- TIMESTAMP -----

// Basic.
RenderingDeviceDriver::QueryPoolID RenderingDeviceDriverWebGpu::timestamp_query_pool_create(uint32_t p_query_count) {
	// TODO
	return QueryPoolID(1);
}

void RenderingDeviceDriverWebGpu::timestamp_query_pool_free(QueryPoolID p_pool_id) {
	// TODO
}

void RenderingDeviceDriverWebGpu::timestamp_query_pool_get_results(QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t *r_results) {
	// TODO
}

uint64_t RenderingDeviceDriverWebGpu::timestamp_query_result_to_time(uint64_t p_result) {
	// TODO
	return 1;
}

// Commands.
void RenderingDeviceDriverWebGpu::command_timestamp_query_pool_reset(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) {}
void RenderingDeviceDriverWebGpu::command_timestamp_write(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) {}

/****************/
/**** LABELS ****/
/****************/

void RenderingDeviceDriverWebGpu::command_begin_label(CommandBufferID p_cmd_buffer, const char *p_label_name, const Color &p_color) {}
void RenderingDeviceDriverWebGpu::command_end_label(CommandBufferID p_cmd_buffer) {}

/********************/
/**** SUBMISSION ****/
/********************/

void RenderingDeviceDriverWebGpu::begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) {}
void RenderingDeviceDriverWebGpu::end_segment() {}

/**************/
/**** MISC ****/
/**************/

void RenderingDeviceDriverWebGpu::set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) {}
uint64_t RenderingDeviceDriverWebGpu::get_resource_native_handle(DriverResource p_type, ID p_driver_id) {}
uint64_t RenderingDeviceDriverWebGpu::get_total_memory_used() {}
uint64_t RenderingDeviceDriverWebGpu::limit_get(Limit p_limit) {
	WGPUSupportedLimitsExtras extras;
	WGPUSupportedLimits limits;
	limits.nextInChain = (WGPUChainedStructOut *)&extras;
	wgpuDeviceGetLimits(device, &limits);
	return rd_limit_from_webgpu(p_limit, limits);
}

uint64_t RenderingDeviceDriverWebGpu::api_trait_get(ApiTrait p_trait) {
	// TODO
	return RenderingDeviceDriver::api_trait_get(p_trait);
}

bool RenderingDeviceDriverWebGpu::has_feature(Features p_feature) {
	// TODO
	return false;
}

const RenderingDeviceDriver::MultiviewCapabilities &RenderingDeviceDriverWebGpu::get_multiview_capabilities() {}

String RenderingDeviceDriverWebGpu::get_api_name() const {
	return "WebGpu";
}

String RenderingDeviceDriverWebGpu::get_api_version() const {
	// TODO: We should compile this in based on the wgpu / dawn version
	return "v0.19.3.1 (wgpu)";
}

String RenderingDeviceDriverWebGpu::get_pipeline_cache_uuid() const {}

const RenderingDeviceDriver::Capabilities &RenderingDeviceDriverWebGpu::get_capabilities() const {
	return capabilties;
}

RenderingDeviceDriverWebGpu::RenderingDeviceDriverWebGpu(RenderingContextDriverWebGpu *p_context_driver) {
	DEV_ASSERT(p_context_driver != nullptr);

	context_driver = p_context_driver;
}
RenderingDeviceDriverWebGpu::~RenderingDeviceDriverWebGpu() {
	if (queue != nullptr) {
		wgpuQueueRelease(queue);
	}

	if (queue != nullptr) {
		wgpuAdapterRelease(adapter);
	}

	if (device != nullptr) {
		wgpuDeviceRelease(device);
	}
}
