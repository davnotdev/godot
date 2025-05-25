#include "rendering_device_driver_webgpu.h"
#include "combimgsamplersplitter.h"
#include "core/error/error_macros.h"
#include "core/io/marshalls.h"
#include "core/os/memory.h"
#include "core/string/print_string.h"
#include "core/templates/local_vector.h"
#include "rendering_context_driver_webgpu.h"
#include "thirdparty/misc/smolv.h"
#include "webgpu.h"
#include "webgpu_conv.h"

#include <wgpu.h>
#include <cstdint>
#include <cstring>

#define WGPU_LOG

static void handle_request_device(WGPURequestDeviceStatus p_status,
		WGPUDevice p_device, WGPUStringView p_message,
		void *userdata, void *_) {
	if (p_status != WGPURequestDeviceStatus_Success) {
		print_line("[WGPU]", String::utf8(p_message.data, p_message.length));
	}
	*(WGPUDevice *)userdata = p_device;
}

Error RenderingDeviceDriverWebGpu::initialize(uint32_t p_device_index, uint32_t p_frame_count) {
#ifdef WGPU_LOG
	wgpuSetLogCallback([](WGPULogLevel p_level, WGPUStringView p_message, void *userdata) {
		print_line("[WGPU]", String::utf8(p_message.data, p_message.length));
	},
			nullptr);
#endif

	adapter = context_driver->adapter_get(p_device_index);
	context_device = context_driver->device_get(p_device_index);

	WGPUFeatureName required_features[] = {
		WGPUFeatureName_Depth32FloatStencil8,

		// Waiting on WebGPU spec, see https://github.com/gpuweb/gpuweb/blob/main/proposals/push-constants.md
		(WGPUFeatureName)WGPUNativeFeature_PushConstants,
		// Need to implement shader translation (via naga or tint)
		(WGPUFeatureName)WGPUNativeFeature_SpirvShaderPassthrough,
		// Need changes in Godot (default textures use 16bit I believe)
		(WGPUFeatureName)WGPUNativeFeature_TextureFormat16bitNorm,
		// Wating on "texture-formats-tier1" to be exposed
		(WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures,

		// Needs SPIRV workaround
		(WGPUFeatureName)WGPUNativeFeature_TextureBindingArray,
		(WGPUFeatureName)WGPUNativeFeature_StorageResourceBindingArray,
		(WGPUFeatureName)WGPUNativeFeature_BufferBindingArray,

		// Currently avoidable
		// (WGPUFeatureName)WGPUNativeFeature_VertexWritableStorage,
		// (WGPUFeatureName)WGPUNativeFeature_MultiDrawIndirect,
		// (WGPUFeatureName)WGPUNativeFeature_MultiDrawIndirectCount,
	};

	// NOTE: These tweaks are ONLY REQUIRED FOR FORWARD+.
	// Forward Mobile should still be fully supported under the default limits.
	WGPUNativeLimits required_native_limits = (WGPUNativeLimits){
		.chain = (WGPUChainedStructOut){
				.sType = (WGPUSType)WGPUSType_NativeLimits,
		},
		.maxPushConstantSize = 128,
		.maxNonSamplerBindings = WGPU_LIMIT_U32_UNDEFINED
	};

	WGPULimits required_limits =
			(WGPULimits){
				.nextInChain = (WGPUChainedStructOut *)&required_native_limits,
				.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED,
				.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED,
				.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED,
				.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED,
				.maxBindGroups = 5,
				.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED,
				.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED,
				.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED,
				.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED,
				.maxSampledTexturesPerShaderStage = 49,
				.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED,
				.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED,
				.maxStorageTexturesPerShaderStage = 15,
				// NOTE: I'm not sure why Godot uses 32768 + 272 of these...
				.maxUniformBuffersPerShaderStage = 32768 + 272,
				// NOTE: This is my system's max buffer size, needed for some godot examples.
				.maxUniformBufferBindingSize = 2147483648,
				.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED,
				.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED,
				.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED,
				.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED,
				.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED,
				.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED,
				.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED,
				.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED,
				.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED,
				.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED,
				.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED,
			};

	WGPUDeviceDescriptor device_desc = (WGPUDeviceDescriptor){
		.requiredFeatureCount = sizeof(required_features) / sizeof(WGPUFeatureName),
		.requiredFeatures = required_features,
		.requiredLimits = &required_limits,
	};
	WGPURequestDeviceCallbackInfo device_callback_info = (WGPURequestDeviceCallbackInfo){
		.mode = WGPUCallbackMode_AllowProcessEvents,
		.callback = handle_request_device,
		.userdata1 = &this->device,
	};
	wgpuAdapterRequestDevice(adapter, &device_desc, device_callback_info);
	ERR_FAIL_COND_V(!this->device, FAILED);

	queue = wgpuDeviceGetQueue(device);
	ERR_FAIL_COND_V(!this->queue, FAILED);

	capabilties = (Capabilities){
		// TODO: This information is not accurate, see modules/glslang/register_types.cpp:78.
		.device_family = DEVICE_WEBGPU,
		.version_major = 24,
		.version_minor = 0,
	};
	multiview_capabilities = (MultiviewCapabilities){
		.is_supported = false,
	};
	fdm_capabilities = (FragmentDensityMapCapabilities){
		.attachment_supported = false,
		.dynamic_attachment_supported = false,
		.non_subsampled_images_supported = false,
		.invocations_supported = false,
		.offset_supported = false,
	};
	fsr_capabilities = (FragmentShadingRateCapabilities){
		.pipeline_supported = false,
		.primitive_supported = false,
		.attachment_supported = false,
	};

	return OK;
}

/*****************/
/**** BUFFERS ****/
/*****************/

RenderingDeviceDriverWebGpu::BufferID RenderingDeviceDriverWebGpu::buffer_create(uint64_t p_size, BitField<BufferUsageBits> p_usage, MemoryAllocationType p_allocation_type) {
	WGPUBufferUsage usage = webgpu_buffer_usage_from_rd(p_usage);
	uint32_t map_mode = 0;
	bool is_transfer_buffer = false;

	if (p_allocation_type == MemoryAllocationType::MEMORY_ALLOCATION_TYPE_GPU) {
		usage = (WGPUBufferUsage)((int)usage & ~(WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite));
	} else {
		if (usage & WGPUBufferUsage_MapRead) {
			map_mode |= WGPUMapMode_Read;
			is_transfer_buffer = true;
		}
		if (usage & WGPUBufferUsage_MapWrite) {
			map_mode |= WGPUMapMode_Write;
			is_transfer_buffer = true;
		}
	}

	WGPUBufferDescriptor desc = (WGPUBufferDescriptor){
		.usage = usage,
		.size = STEPIFY(p_size, 256),
		.mappedAtCreation = is_transfer_buffer,
	};
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);

	BufferInfo *buffer_info = memnew(BufferInfo);
	// NOTE: The ACTUAL buffer size is aligned for copying.
	// Some calls to command_copy_buffer don't respect alignment
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
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	return buffer_info->size;
}

static void handle_buffer_map(WGPUMapAsyncStatus status, WGPUStringView _message, void *_userdata1, void *_userdata2) {
	ERR_FAIL_COND_V_MSG(
			status != WGPUMapAsyncStatus_Success, (void)0,
			vformat("Failed to map buffer"));
}

uint8_t *RenderingDeviceDriverWebGpu::buffer_map(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	uint64_t offset = 0;
	uint64_t size = buffer_info->size;

	if (!buffer_info->is_transfer_first_map) {
		WGPUBufferMapCallbackInfo buffer_map_callback_info = (WGPUBufferMapCallbackInfo){
			.mode = WGPUCallbackMode_AllowProcessEvents,
			.callback = handle_buffer_map,
		};
		wgpuBufferMapAsync(
				buffer_info->buffer, buffer_info->map_mode, offset, size, buffer_map_callback_info);
		wgpuDevicePoll(device, true, nullptr);
	} else {
		buffer_info->is_transfer_first_map = false;
	}
	const void *data = wgpuBufferGetConstMappedRange(buffer_info->buffer, offset, size);
	return (uint8_t *)data;
}

void RenderingDeviceDriverWebGpu::buffer_unmap(BufferID p_buffer) {
	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;
	buffer_info->is_transfer_first_map = false;

	wgpuBufferUnmap(buffer_info->buffer);
}

uint64_t RenderingDeviceDriverWebGpu::buffer_get_device_address(BufferID p_buffer) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> buffer_get_device_address");
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
	WGPUTextureFormat texture_format = webgpu_texture_format_from_rd(p_format.format);
	WGPUTextureFormat view_format = webgpu_texture_format_from_rd(p_view.format);
	WGPUTextureUsage usage = (WGPUTextureUsage)usage_bits;
	WGPUTextureAspect aspect = webgpu_texture_aspect_from_rd_format(p_format.format);

	WGPUExtent3D size;
	size.width = p_format.width;
	size.height = p_format.height;
	size.depthOrArrayLayers = p_format.array_layers;
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

	for (uint32_t i = 0; i < p_format.shareable_formats.size(); i++) {
		DataFormat format = p_format.shareable_formats[i];
		view_formats.push_back(webgpu_texture_format_from_rd(format));
	}

	WGPUTextureDescriptor texture_desc = (WGPUTextureDescriptor){
		.usage = usage,
		.dimension = dimension,
		.size = size,
		.format = texture_format,
		.mipLevelCount = mip_level_count,
		.sampleCount = sample_count,
		.viewFormatCount = (size_t)view_formats.size(),
		.viewFormats = view_formats.ptr(),
	};

	WGPUTexture texture = wgpuDeviceCreateTexture(device, &texture_desc);

	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){
				.next = nullptr,
				.sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras,
		},
		.swizzle = (WGPUTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};

	// HACK: You cannot have a Depth24PlusStencil8 or Depth32FloatStencil8 texture view be bound.
	if (view_format == WGPUTextureFormat_Depth24PlusStencil8) {
		view_format = WGPUTextureFormat_Depth24Plus;
		aspect = WGPUTextureAspect_DepthOnly;
	} else if (view_format == WGPUTextureFormat_Depth32FloatStencil8) {
		view_format = WGPUTextureFormat_Depth32Float;
		aspect = WGPUTextureAspect_DepthOnly;
	}

	WGPUTextureViewDescriptor texture_view_desc = (WGPUTextureViewDescriptor){
		.nextInChain = (WGPUChainedStruct *)&texture_view_desc_extras,
		.format = view_format,
		.dimension = webgpu_texture_view_dimension_from_rd(p_format.texture_type),
		.mipLevelCount = texture_desc.mipLevelCount,
		.arrayLayerCount = is_using_depth ? 1 : texture_desc.size.depthOrArrayLayers,
		.aspect = aspect,
	};

	WGPUTextureView view = wgpuTextureCreateView(texture, &texture_view_desc);

	TextureInfo *texture_info = memnew(TextureInfo);
	texture_info->texture = texture;
	texture_info->view = view;
	texture_info->rd_texture_format = p_format.format;
	texture_info->texture_desc = texture_desc;
	texture_info->texture_view_desc = texture_view_desc;
	texture_info->is_original_texture = true;
	texture_info->is_using_depth = is_using_depth;

	return TextureID(texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> texture_create_from_extension");
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared(TextureID p_original_texture, const TextureView &p_view) {
	TextureInfo *texture_info = (TextureInfo *)p_original_texture.id;

	// HACK: We need to account for the fact that some texture formats may not support the usages of the
	// The vulkan driver does a check then unflags certain usages, but we don't have that ability.
	// Note that on wgpu vulkan, this will fail old versions of the the api (1.0 with minimal extensions).
	WGPUTextureUsage texture_view_usage = texture_info->texture_desc.usage;
	if (p_view.format == DATA_FORMAT_R8G8B8A8_SRGB) {
		if (texture_info->texture_desc.usage & WGPUTextureUsage_StorageBinding) {
			texture_view_usage &= (~WGPUTextureUsage_StorageBinding);
		}
	}

	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){
				.next = nullptr,
				.sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras,
		},
		.swizzle = (WGPUTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};

	WGPUTextureViewDescriptor texture_view_desc = (WGPUTextureViewDescriptor){
		.nextInChain = (WGPUChainedStruct *)&texture_view_desc_extras,
		.format = webgpu_texture_format_from_rd(p_view.format),
		.mipLevelCount = texture_info->texture_view_desc.mipLevelCount,
		.arrayLayerCount = texture_info->texture_view_desc.arrayLayerCount,
		.usage = texture_view_usage,
	};

	WGPUTextureView view = wgpuTextureCreateView(texture_info->texture, &texture_view_desc);

	TextureInfo *new_texture_info = memnew(TextureInfo);
	*new_texture_info = *texture_info;
	new_texture_info->view = view;
	new_texture_info->is_original_texture = false;
	new_texture_info->texture_view_desc = texture_view_desc;

	return TextureID(new_texture_info);
}

RenderingDeviceDriver::TextureID RenderingDeviceDriverWebGpu::texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) {
	TextureInfo *texture_info = (TextureInfo *)p_original_texture.id;

	WGPUTextureViewDescriptorExtras texture_view_desc_extras = (WGPUTextureViewDescriptorExtras){
		.chain = (WGPUChainedStruct){
				.next = nullptr,
				.sType = (WGPUSType)WGPUSType_TextureViewDescriptorExtras,
		},
		.swizzle = (WGPUTextureViewSwizzle){
				.r = webgpu_component_swizzle_from_rd(p_view.swizzle_r),
				.g = webgpu_component_swizzle_from_rd(p_view.swizzle_g),
				.b = webgpu_component_swizzle_from_rd(p_view.swizzle_b),
				.a = webgpu_component_swizzle_from_rd(p_view.swizzle_a),
		},
	};

	WGPUTextureFormat format = webgpu_texture_format_from_rd(p_view.format);
	WGPUTextureAspect aspect = webgpu_texture_aspect_from_rd_format(p_view.format);
	WGPUTextureViewDescriptor texture_view_desc = (WGPUTextureViewDescriptor){
		.nextInChain = (WGPUChainedStruct *)&texture_view_desc_extras,
		.format = format,
		.dimension = texture_info->texture_view_desc.dimension,
		.baseMipLevel = p_mipmap,
		.mipLevelCount = p_mipmaps,
		.baseArrayLayer = p_layer,
		.arrayLayerCount = p_layers,
		.aspect = aspect,
	};

	switch (p_slice_type) {
		case RenderingDeviceCommons::TEXTURE_SLICE_2D:
			texture_view_desc.dimension = WGPUTextureViewDimension_2D;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_CUBEMAP:
			texture_view_desc.dimension = WGPUTextureViewDimension_Cube;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_3D:
			texture_view_desc.dimension = WGPUTextureViewDimension_3D;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_2D_ARRAY:
			texture_view_desc.dimension = WGPUTextureViewDimension_2DArray;
			break;
		case RenderingDeviceCommons::TEXTURE_SLICE_MAX:
			return TextureID();
	}

	WGPUTextureView view = wgpuTextureCreateView(texture_info->texture, &texture_view_desc);

	TextureInfo *new_texture_info = memnew(TextureInfo);
	*new_texture_info = *texture_info;
	new_texture_info->view = view;
	new_texture_info->is_original_texture = false;
	new_texture_info->texture_view_desc = texture_view_desc;

	return TextureID(new_texture_info);
}

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

void RenderingDeviceDriverWebGpu::texture_get_copyable_layout(TextureID p_texture, const TextureSubresource &p_subresource, TextureCopyableLayout *r_layout) {
	TextureInfo *texture_info = (TextureInfo *)p_texture.id;

	uint32_t block_size = webgpu_texture_format_block_copy_size(texture_info->texture_desc.format, webgpu_texture_aspect_from_rd((TextureAspectBits)(1 << p_subresource.aspect)));

	// TODO: Account for mipmaps and maybe compressed formats too.
	r_layout->offset = 0;
	r_layout->row_pitch = STEPIFY(block_size * texture_info->texture_desc.size.width, 256);
	r_layout->size = r_layout->row_pitch * texture_info->texture_desc.size.height * texture_info->texture_desc.size.depthOrArrayLayers;
	r_layout->depth_pitch = r_layout->size / texture_info->texture_desc.size.depthOrArrayLayers;
	r_layout->layer_pitch = r_layout->size / texture_info->texture_desc.size.depthOrArrayLayers;
}

uint8_t *RenderingDeviceDriverWebGpu::texture_map(TextureID p_texture, const TextureSubresource &p_subresource) {
	return nullptr;
}
void RenderingDeviceDriverWebGpu::texture_unmap(TextureID p_texture) {}

BitField<RenderingDeviceDriver::TextureUsageBits> RenderingDeviceDriverWebGpu::texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) {
	// TODO: Read this https://www.w3.org/TR/webgpu/#texture-format-caps
	BitField<RDD::TextureUsageBits> supported = INT64_MAX;

	// HACK: Here are the formats we dislike.
	if (p_format == DATA_FORMAT_ASTC_4x4_SRGB_BLOCK || p_format == DATA_FORMAT_R32G32B32_SFLOAT || p_format == DATA_FORMAT_BC1_RGB_UNORM_BLOCK) {
		return 0;
	}

	return supported;
}

bool RenderingDeviceDriverWebGpu::texture_can_make_shared_with_format(TextureID p_texture, DataFormat p_format, bool &r_raw_reinterpretation) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> texture_can_make_shared_with_format");
	return true;
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
		// NOTE: `min_lod` cannot be negative.
		// See https://www.w3.org/TR/webgpu/#sampler-creation
		.lodMinClamp = p_state.min_lod < 0.0f ? 0.0f : p_state.min_lod,
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

bool RenderingDeviceDriverWebGpu::sampler_is_format_supported_for_filter(DataFormat _p_format, SamplerFilter p_filter) {
	// "descriptor.magFilter, descriptor.minFilter, and descriptor.mipmapFilter must be "linear"."
	return p_filter == SamplerFilter::SAMPLER_FILTER_LINEAR;
}

/**********************/
/**** VERTEX ARRAY ****/
/**********************/

// NOTE: The attributes in `p_vertex_attribs` must be in order.
RenderingDeviceDriver::VertexFormatID RenderingDeviceDriverWebGpu::vertex_format_create(VectorView<VertexAttribute> p_vertex_attribs) {
	VertexFormatInfo *vertex_format_info = memnew(VertexFormatInfo);
	vertex_format_info->layouts.resize_zeroed(p_vertex_attribs.size());
	vertex_format_info->vertex_attributes.resize_zeroed(p_vertex_attribs.size());

	for (uint32_t i = 0; i < p_vertex_attribs.size(); i++) {
		VertexAttribute attrib = p_vertex_attribs[i];

		vertex_format_info->vertex_attributes.set(i,
				(WGPUVertexAttribute){
						.format = webgpu_vertex_format_from_rd(attrib.format),
						.offset = attrib.offset,
						.shaderLocation = attrib.location,
				});

		WGPUVertexStepMode step_mode = attrib.frequency == VertexFrequency::VERTEX_FREQUENCY_VERTEX ? WGPUVertexStepMode_Vertex : WGPUVertexStepMode_Instance;

		WGPUVertexBufferLayout layout = (WGPUVertexBufferLayout){
			.stepMode = step_mode,
			.arrayStride = attrib.stride,
			.attributeCount = 1,
			.attributes = vertex_format_info->vertex_attributes.ptr() + i,
		};
		vertex_format_info->layouts.set(i, layout);
	}

	return VertexFormatID(vertex_format_info);
}

void RenderingDeviceDriverWebGpu::vertex_format_free(VertexFormatID p_vertex_format) {
	VertexFormatInfo *vertex_format_info = (VertexFormatInfo *)p_vertex_format.id;
	memdelete(vertex_format_info);
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
		VectorView<TextureBarrier> p_texture_barriers) {
	// Empty.
}

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

Error RenderingDeviceDriverWebGpu::command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) {
	Vector<WGPUCommandBuffer> commands = Vector<WGPUCommandBuffer>();

	for (uint32_t i = 0; i < p_cmd_buffers.size(); i++) {
		CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffers[i].id;

		DEV_ASSERT(command_buffer_info != nullptr);
		DEV_ASSERT(command_buffer_info->encoder != nullptr);

		WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_buffer_info->encoder, nullptr);
		commands.push_back(command_buffer);

		wgpuCommandEncoderRelease(command_buffer_info->encoder);
		command_buffer_info->encoder = nullptr;
	}

	wgpuQueueSubmit(queue, commands.size(), commands.ptr());

	for (uint32_t i = 0; i < commands.size(); i++) {
		WGPUCommandBuffer command_buffer = commands[i];
		wgpuCommandBufferRelease(command_buffer);
	}

	// TODO: IMPL
	// Q: Will we get multiple surfaces?
	for (uint32_t i = 0; i < p_swap_chains.size(); i++) {
		SwapChainInfo *swapchain = (SwapChainInfo *)p_swap_chains[i].id;
		RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain->surface;

		wgpuSurfacePresent(surface->surface);
	}

	return OK;
}

void RenderingDeviceDriverWebGpu::command_queue_free(CommandQueueID _p_cmd_queue) {
	// Empty.
}

// ----- POOL -----

RenderingDeviceDriver::CommandPoolID RenderingDeviceDriverWebGpu::command_pool_create(CommandQueueFamilyID _p_cmd_queue_family, CommandBufferType _p_cmd_buffer_type) {
	TightLocalVector<CommandBufferInfo *> *command_pool = memnew(TightLocalVector<CommandBufferInfo *>);
	return CommandPoolID(command_pool);
}

bool RenderingDeviceDriverWebGpu::command_pool_reset(CommandPoolID p_cmd_pool) {
	// TODO: impl
	return true;
}

void RenderingDeviceDriverWebGpu::command_pool_free(CommandPoolID p_cmd_pool) {
	TightLocalVector<CommandBufferInfo *> *command_pool = (TightLocalVector<CommandBufferInfo *> *)p_cmd_pool.id;
	for (uint32_t i = 0; i < command_pool->size(); i++) {
		memfree(command_pool->ptr()[i]);
	}
	memfree(command_pool);
}

// ----- BUFFER -----

RenderingDeviceDriver::CommandBufferID RenderingDeviceDriverWebGpu::command_buffer_create(CommandPoolID p_cmd_pool) {
	TightLocalVector<CommandBufferInfo *> *command_pool = (TightLocalVector<CommandBufferInfo *> *)p_cmd_pool.id;
	CommandBufferInfo *command_buffer_info = memnew(CommandBufferInfo);

	command_buffer_info->encoder = nullptr;
	command_buffer_info->is_render_pass_active = false;

	command_pool->push_back(command_buffer_info);

	return CommandBufferID(command_buffer_info);
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);

	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;

	WGPUCommandEncoderDescriptor desc = (WGPUCommandEncoderDescriptor){};
	command_buffer_info->encoder = wgpuDeviceCreateCommandEncoder(device, &desc);
	command_buffer_info->active_compute_pass_info = (ComputePassEncoderInfo){
		.commands = Vector<ComputePassEncoderCommand>(),
	};

	return true;
}

bool RenderingDeviceDriverWebGpu::command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> command_buffer_begin_secondary");
}

void RenderingDeviceDriverWebGpu::command_buffer_end(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);

	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;

	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	if (command_buffer_info->active_compute_pass_info.commands.size() != 0) {
		WGPUComputePassEncoder compute_encoder = wgpuCommandEncoderBeginComputePass(
				command_buffer_info->encoder, nullptr);

		for (uint32_t i = 0; i < command_buffer_info->active_compute_pass_info.commands.size(); i++) {
			ComputePassEncoderCommand &command = command_buffer_info->active_compute_pass_info.commands.write[i];

			switch (command.type) {
				case ComputePassEncoderCommand::CommandType::SET_PIPELINE: {
					ComputePassEncoderCommand::SetPipeline data = command.set_pipeline;
					wgpuComputePassEncoderSetPipeline(compute_encoder, data.pipeline);
				} break;
				case ComputePassEncoderCommand::CommandType::SET_BIND_GROUP: {
					ComputePassEncoderCommand::SetBindGroup data = command.set_bind_group;
					wgpuComputePassEncoderSetBindGroup(compute_encoder, data.index, data.bind_group, 0, nullptr);
				} break;
				case ComputePassEncoderCommand::CommandType::SET_PUSH_CONSTANTS: {
					ComputePassEncoderCommand::SetPushConstants data = command.set_push_constants;
					wgpuComputePassEncoderSetPushConstants(compute_encoder, data.offset, command.push_constant_data.size(), command.push_constant_data.ptr());

				} break;
				case ComputePassEncoderCommand::CommandType::DISPATCH_WORKGROUPS: {
					ComputePassEncoderCommand::DispatchWorkgroups data = command.dispatch_workgroups;
					wgpuComputePassEncoderDispatchWorkgroups(compute_encoder, data.workgroup_count_x, data.workgroup_count_y, data.workgroup_count_z);
				} break;
				case ComputePassEncoderCommand::CommandType::DISPATCH_WORKGROUPS_INDIRECT: {
					ComputePassEncoderCommand::DispatchWorkgroupsIndirect data = command.dispatch_workgroups_indirect;
					wgpuComputePassEncoderDispatchWorkgroupsIndirect(compute_encoder, data.indirect_buffer, data.indirect_offset);
				} break;
					break;
			}
		}

		wgpuComputePassEncoderEnd(compute_encoder);
		wgpuComputePassEncoderRelease(compute_encoder);
	}
}

void RenderingDeviceDriverWebGpu::command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) {
	// TODO: impl
	CRASH_NOW_MSG("TODO --> command_buffer_execute_secondary");
}

/********************/
/**** SWAP CHAIN ****/
/********************/

RenderingDeviceDriver::SwapChainID RenderingDeviceDriverWebGpu::swap_chain_create(RenderingContextDriver::SurfaceID p_surface) {
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)p_surface;

	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);

	surface->configure(this->adapter, this->device);

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>({ (RenderPassAttachmentInfo){
			.format = surface->format,
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

Error RenderingDeviceDriverWebGpu::swap_chain_resize(CommandQueueID _p_cmd_queue, SwapChainID p_swap_chain, uint32_t _p_desired_framebuffer_count) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

	surface->configure(this->adapter, this->device);
	context_driver->surface_set_needs_resize(swapchain_info->surface, false);

	return OK;
}

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	if (context_driver->surface_get_needs_resize(swapchain_info->surface)) {
		r_resize_required = true;
		return FramebufferID();
	}

	FramebufferInfo *framebuffer_info = memnew(FramebufferInfo);
	framebuffer_info->maybe_swapchain = p_swap_chain;

	return FramebufferID(framebuffer_info);
}

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::swap_chain_get_render_pass(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	return swapchain_info->render_pass;
}

RenderingDeviceDriver::DataFormat RenderingDeviceDriverWebGpu::swap_chain_get_format(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;
	RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

	return surface->rd_format;
}

void RenderingDeviceDriverWebGpu::swap_chain_free(SwapChainID p_swap_chain) {
	SwapChainInfo *swapchain_info = (SwapChainInfo *)p_swap_chain.id;

	memdelete((RenderPassInfo *)swapchain_info->render_pass.id);
	memdelete(swapchain_info);
}

/*********************/
/**** FRAMEBUFFER ****/
/*********************/

RenderingDeviceDriver::FramebufferID RenderingDeviceDriverWebGpu::framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t _p_width, uint32_t _p_height) {
	FramebufferInfo *framebuffer_info = memnew(FramebufferInfo);
	framebuffer_info->maybe_swapchain = SwapChainID();

	Vector<TextureID> attachments = Vector<TextureID>();
	for (uint32_t i = 0; i < p_attachments.size(); i++) {
		attachments.push_back(p_attachments[i]);
	}
	framebuffer_info->attachments = attachments;

	return FramebufferID(framebuffer_info);
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
	Vector<ShaderStageSPIRVData> spirv;

	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		Vector<uint32_t> in_spirv;
		in_spirv.resize(p_spirv[i].spirv.size() / 4);
		memcpy(in_spirv.ptrw(), p_spirv[i].spirv.ptr(), p_spirv[i].spirv.size());
		Vector<uint32_t> out_spirv = combimgsampsplitter(in_spirv);

		spirv.push_back((ShaderStageSPIRVData){
				.shader_stage = p_spirv[i].shader_stage,
				.spirv = out_spirv.to_byte_array(),
		});
	}

	// This code is mostly taken from the vulkan device driver.
	ShaderReflection shader_refl;
	if (_reflect_spirv(p_spirv, shader_refl) != OK) {
		return Vector<uint8_t>();
	}

	ShaderBinary::Data binary_data;
	Vector<Vector<ShaderBinary::DataBinding>> uniforms; // Set bindings.
	Vector<ShaderBinary::SpecializationConstant> specialization_constants;
	{
		binary_data.vertex_input_mask = shader_refl.vertex_input_mask;
		binary_data.fragment_output_mask = shader_refl.fragment_output_mask;
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
			spec_constant.type = refl_sc.type;
			spec_constant.constant_id = refl_sc.constant_id;
			spec_constant.int_value = refl_sc.int_value;
			spec_constant.stage_flags = refl_sc.stages;

			CharString ascii_name = refl_sc.name.ascii();
			ERR_FAIL_COND_V((uint32_t)ascii_name.size() > ShaderBinary::SpecializationConstant::OVERRIDE_CONSTANT_STRLEN, Vector<uint8_t>());
			strncpy(spec_constant.value_name, ascii_name.ptr(), ShaderBinary::SpecializationConstant::OVERRIDE_CONSTANT_STRLEN);

			specialization_constants.push_back(spec_constant);
		}
	}

	Vector<Vector<uint8_t>> compressed_stages;
	Vector<uint32_t> smolv_size;
	Vector<uint32_t> zstd_size;

	uint32_t stages_binary_size = 0;

	bool strip_debug = false;

	for (uint32_t i = 0; i < spirv.size(); i++) {
		smolv::ByteArray smolv;
		if (!smolv::Encode(spirv[i].spirv.ptr(), spirv[i].spirv.size(), smolv, strip_debug ? smolv::kEncodeFlagStripDebugInfo : 0)) {
			ERR_FAIL_V_MSG(Vector<uint8_t>(), "Error compressing shader stage :" + String::utf8(SHADER_STAGE_NAMES[spirv[i].shader_stage]));
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
	binary_data.stage_count = spirv.size();

	CharString shader_name_utf = p_shader_name.utf8();

	binary_data.shader_name_len = shader_name_utf.length();

	// Header + version + main datasize;.
	uint32_t total_size = sizeof(uint32_t) * 3;

	total_size += sizeof(ShaderBinary::Data);

	total_size += STEPIFY(binary_data.shader_name_len, 4);
	for (uint32_t i = 0; i < uniforms.size(); i++) {
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

		for (uint32_t i = 0; i < uniforms.size(); i++) {
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

		for (uint32_t i = 0; i < compressed_stages.size(); i++) {
			encode_uint32(spirv[i].shader_stage, binptr + offset);
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

RenderingDeviceDriver::ShaderID RenderingDeviceDriverWebGpu::shader_create_from_bytecode(const Vector<uint8_t> &p_shader_binary, ShaderDescription &r_shader_desc, String &r_name, const Vector<ImmutableSampler> &p_immutable_samplers) {
	r_shader_desc = {};

	// TODO: We allocate memory and call wgpuDeviceCreate*. Perhaps, we should free that memory if we fail.
	ShaderInfo *shader_info = memnew(ShaderInfo);
	*shader_info = { };

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
		r_name = String::utf8((const char *)(binptr + read_offset), binary_data.shader_name_len);
		read_offset += STEPIFY(binary_data.shader_name_len, 4);
	}

	Vector<Vector<WGPUBindGroupLayoutEntry>> bind_group_layout_entries;

	r_shader_desc.uniform_sets.resize(binary_data.set_count);
	bind_group_layout_entries.resize(binary_data.set_count);

	for (uint32_t set_idx = 0; set_idx < binary_data.set_count; set_idx++) {
		ERR_FAIL_COND_V(read_offset + sizeof(uint32_t) >= binsize, ShaderID());
		uint32_t binding_count = decode_uint32(binptr + read_offset);
		read_offset += sizeof(uint32_t);
		const ShaderBinary::DataBinding *set_ptr = reinterpret_cast<const ShaderBinary::DataBinding *>(binptr + read_offset);
		uint32_t binding_size = binding_count * sizeof(ShaderBinary::DataBinding);
		ERR_FAIL_COND_V(read_offset + binding_size >= binsize, ShaderID());

		uint32_t wgpu_binding_offset = 0;

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
			layout_entry.binding = set_ptr[j].binding + wgpu_binding_offset;
			for (uint32_t k = 0; k < SHADER_STAGE_MAX; k++) {
				if ((set_ptr[j].stages & (1 << k))) {
					layout_entry.visibility |= webgpu_shader_stage_from_rd((ShaderStage)k);
				}
			}

			WGPUBindGroupLayoutEntryExtras *layout_entry_extras = ALLOCA_SINGLE(WGPUBindGroupLayoutEntryExtras);
			*layout_entry_extras = (WGPUBindGroupLayoutEntryExtras){
				.chain = (WGPUChainedStruct){
						.sType = (WGPUSType)WGPUSType_BindGroupLayoutEntryExtras,
				},
				.count = 1
			};

			layout_entry.nextInChain = (const WGPUChainedStruct *)layout_entry_extras;

			switch (info.type) {
				case UNIFORM_TYPE_SAMPLER: {
					// TODO: I don't know what this means.
					layout_entry.sampler = (WGPUSamplerBindingLayout){
						.type = WGPUSamplerBindingType_Comparison
					};
					layout_entry_extras->count = set_ptr[j].length;
				} break;
				case UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
					WGPUBindGroupLayoutEntry texture_layout_entry = layout_entry;
					texture_layout_entry.binding = set_ptr[j].binding + wgpu_binding_offset;

					texture_layout_entry.texture = (WGPUTextureBindingLayout){
						// NOTE: Other texture types don't appear to be supported by spirv reflect, but utexture2D does appear once in godot.
						.sampleType = WGPUTextureSampleType_UnfilterableFloat,
						.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type),
						.multisampled = info.texture_is_multisample,
					};

					bind_group_layout_entries.write[set_idx].push_back(texture_layout_entry);
					layout_entry.sampler = (WGPUSamplerBindingLayout){
						.type = WGPUSamplerBindingType_Comparison
					};
					// TODO: Figure our array of combined image samplers.

					layout_entry.binding += 1;
					wgpu_binding_offset += 1;
				} break;
				case UNIFORM_TYPE_TEXTURE: {
					layout_entry.texture = (WGPUTextureBindingLayout){
						// NOTE: Other texture types don't appear to be supported by spirv reflect, but utexture2D does appear once in godot.
						.sampleType = WGPUTextureSampleType_UnfilterableFloat,
						.viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type),
						.multisampled = info.texture_is_multisample,
					};
					layout_entry_extras->count = set_ptr[j].length;
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

					// HACK: Replace cube storage texture to bypass error for now.
					// entry.storageTexture.viewDimension is not "cube" or "cube-array".
					WGPUTextureViewDimension viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type);
					if (viewDimension == WGPUTextureViewDimension_Cube) {
						viewDimension = WGPUTextureViewDimension_3D;
					}

					layout_entry.storageTexture = (WGPUStorageTextureBindingLayout){
						.access = access,
						.format = webgpu_texture_format_from_rd(info.image_format),
						/* .viewDimension = webgpu_texture_view_dimension_from_rd(info.texture_image_type), */
						.viewDimension = viewDimension,
					};
					layout_entry_extras->count = set_ptr[j].length;
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
						// TODO: Investigate this further.
						// .type = info.writable ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage,
						.type = (layout_entry.visibility & WGPUShaderStage_Vertex) ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage,
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

			r_shader_desc.uniform_sets.write[set_idx].push_back(info);
			bind_group_layout_entries.write[set_idx].push_back(layout_entry);
		}

		read_offset += binding_size;
	}

	ERR_FAIL_COND_V(read_offset + binary_data.specialization_constants_count * sizeof(ShaderBinary::SpecializationConstant) >= binsize, ShaderID());

	r_shader_desc.specialization_constants.resize(binary_data.specialization_constants_count);
	for (uint32_t i = 0; i < binary_data.specialization_constants_count; i++) {
		const ShaderBinary::SpecializationConstant &src_sc = *(reinterpret_cast<const ShaderBinary::SpecializationConstant *>(binptr + read_offset));
		ShaderSpecializationConstant sc;
		sc.type = (PipelineSpecializationConstantType)src_sc.type;
		sc.constant_id = src_sc.constant_id;
		sc.int_value = src_sc.int_value;
		sc.stages = src_sc.stage_flags;
		sc.name = String::utf8((char *)src_sc.value_name);
		r_shader_desc.specialization_constants.write[i] = sc;

		shader_info->override_keys[sc.constant_id] = String::utf8((char *)src_sc.value_name);

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
			ERR_FAIL_V_MSG(ShaderID(), "Malformed smolv input uncompressing shader stage:" + String::utf8(SHADER_STAGE_NAMES[stage]));
		}

		r_shader_desc.stages.set(i, ShaderStage(stage));

		buf_size = STEPIFY(buf_size, 4);
		read_offset += buf_size;
		ERR_FAIL_COND_V(read_offset > binsize, ShaderID());
	}

	ERR_FAIL_COND_V(read_offset != binsize, ShaderID());

	for (uint32_t i = 0; i < r_shader_desc.stages.size(); i++) {
		WGPUShaderModuleDescriptorSpirV shader_module_spirv_desc = (WGPUShaderModuleDescriptorSpirV){
			.sourceSize = (uint32_t)stages_spirv[i].size() / 4,
			.source = (const uint32_t *)stages_spirv[i].ptr(),
		};

		WGPUShaderModule shader_module = wgpuDeviceCreateShaderModuleSpirV(device, &shader_module_spirv_desc);
		ERR_FAIL_COND_V(!shader_module, ShaderID());

		ShaderStage stage = r_shader_desc.stages[i];
		switch (stage) {
			case RenderingDeviceCommons::SHADER_STAGE_VERTEX:
				ERR_FAIL_COND_V_MSG(shader_info->vertex_shader, ShaderID(), "More than one vertex stage in one shader.");
				shader_info->vertex_shader = shader_module;
				shader_info->stage_flags |= WGPUShaderStage_Vertex;
				break;
			case RenderingDeviceCommons::SHADER_STAGE_FRAGMENT:
				ERR_FAIL_COND_V_MSG(shader_info->fragment_shader, ShaderID(), "More than one fragment stage in one shader.");
				shader_info->fragment_shader = shader_module;
				shader_info->stage_flags |= WGPUShaderStage_Fragment;
				break;
			case RenderingDeviceCommons::SHADER_STAGE_COMPUTE:
				ERR_FAIL_COND_V_MSG(shader_info->compute_shader, ShaderID(), "More than one compute stage in one shader.");
				shader_info->compute_shader = shader_module;
				shader_info->stage_flags |= WGPUShaderStage_Compute;
				break;
			default:
				ERR_FAIL_V_MSG(ShaderID(), vformat("WebGpu shader stage %d not supported", stage));
				break;
		}
	}

	DEV_ASSERT((uint32_t)bind_group_layout_entries.size() == binary_data.set_count);
	for (uint32_t set_idx = 0; set_idx < binary_data.set_count; set_idx++) {
		WGPUBindGroupLayoutDescriptor bind_group_layout_desc = (WGPUBindGroupLayoutDescriptor){
			.entryCount = (size_t)bind_group_layout_entries[set_idx].size(),
			.entries = bind_group_layout_entries[set_idx].ptr(),
		};

		WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);

		ERR_FAIL_COND_V(!bind_group_layout, ShaderID());

		shader_info->bind_group_layouts.push_back(bind_group_layout);
	}

	WGPUPushConstantRange push_constant_range;

	if (binary_data.push_constant_size) {
		push_constant_range = (WGPUPushConstantRange){
			.stages = binary_data.push_constant_stages,
			.start = 0,
			.end = binary_data.push_constant_size,
		};
	}
	shader_info->push_constant_stage_flags = binary_data.push_constant_stages;

	WGPUPipelineLayoutExtras wgpu_pipeline_extras = (WGPUPipelineLayoutExtras){
		.chain = (WGPUChainedStruct){
				.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras,
		},
		.pushConstantRangeCount = (size_t)(binary_data.push_constant_size ? 1 : 0),
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

void RenderingDeviceDriverWebGpu::shader_destroy_modules(ShaderID p_shader) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> shader_destroy_modules");
}

/*********************/
/**** UNIFORM SET ****/
/*********************/

RenderingDeviceDriver::UniformSetID RenderingDeviceDriverWebGpu::uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index, int p_linear_pool_index) {
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	Vector<WGPUBindGroupEntry> entries;

	uint32_t binding_offset = 0;
	for (uint32_t i = 0; i < p_uniforms.size(); i++) {
		const BoundUniform &uniform = p_uniforms[i];
		switch (uniform.type) {
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER: {
				WGPUBindGroupEntry entry = {};
				entry.binding = uniform.binding + binding_offset;
				if (uniform.ids.size() == 1) {
					entry.sampler = (WGPUSampler)uniform.ids[0].id;
				} else {
					WGPUSampler *uniform_samplers = ALLOCA_ARRAY(WGPUSampler, uniform.ids.size());

					for (uint32_t j = 0; j < uniform.ids.size(); j++) {
						WGPUSampler sampler = (WGPUSampler)uniform.ids[j].id;
						uniform_samplers[j] = sampler;
					}

					WGPUBindGroupEntryExtras *entry_extras = ALLOCA_SINGLE(WGPUBindGroupEntryExtras);
					*entry_extras = (WGPUBindGroupEntryExtras){
						.chain = (WGPUChainedStruct){
								.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras,
						},
						.samplers = uniform_samplers,
						.samplerCount = (size_t)uniform.ids.size(),
					};
					entry.nextInChain = (WGPUChainedStruct *)entry_extras;
				}
				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
				WGPUBindGroupEntry texture_entry = { };
				texture_entry.binding = uniform.binding + binding_offset;

				binding_offset += 1;

				WGPUBindGroupEntry sampler_entry = { };
				sampler_entry.binding = uniform.binding + binding_offset;

				if (uniform.ids.size() == 2) {
					WGPUSampler sampler = (WGPUSampler)uniform.ids[0].id;
					TextureInfo *texture_info = (TextureInfo *)uniform.ids[1].id;

					texture_entry.textureView = texture_info->view;
					sampler_entry.sampler = sampler;
				} else {
					uint32_t uniform_count = uniform.ids.size() / 2;
					WGPUSampler *uniform_samplers = ALLOCA_ARRAY(WGPUSampler, uniform_count);
					WGPUTextureView *uniform_texture_views = ALLOCA_ARRAY(WGPUTextureView, uniform_count);

					for (uint32_t i = 0; i < uniform_count; i++) {
						WGPUSampler sampler = (WGPUSampler)uniform.ids[i * 2 + 0].id;
						TextureInfo *texture_info = (TextureInfo *)uniform.ids[i * 2 + 1].id;

						uniform_samplers[i] = sampler;
						uniform_texture_views[i] = texture_info->view;

						WGPUBindGroupEntryExtras *texture_view_entry_extras = ALLOCA_SINGLE(WGPUBindGroupEntryExtras);
						*texture_view_entry_extras = (WGPUBindGroupEntryExtras){
							.chain = (WGPUChainedStruct){
									.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras,
							},
							.textureViews = uniform_texture_views,
							.textureViewCount = uniform_count,
						};
						texture_entry.nextInChain = (WGPUChainedStruct *)texture_view_entry_extras;

						WGPUBindGroupEntryExtras *sampler_entry_extras = ALLOCA_SINGLE(WGPUBindGroupEntryExtras);
						*sampler_entry_extras = (WGPUBindGroupEntryExtras){
							.chain = (WGPUChainedStruct){
									.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras,
							},
							.samplers = uniform_samplers,
							.samplerCount = uniform_count,
						};
						sampler_entry.nextInChain = (WGPUChainedStruct *)sampler_entry_extras;
					}
				}
				entries.push_back(texture_entry);
				entries.push_back(sampler_entry);
			} break;

			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE:
			case RenderingDeviceCommons::UNIFORM_TYPE_INPUT_ATTACHMENT: {
				WGPUBindGroupEntry entry = {};
				entry.binding = uniform.binding + binding_offset;

				if (uniform.ids.size() == 1) {
					TextureInfo *texture_info = (TextureInfo *)uniform.ids[0].id;
					entry.textureView = texture_info->view;
				} else {
					WGPUTextureView *uniform_texture_views = ALLOCA_ARRAY(WGPUTextureView, uniform.ids.size());

					for (uint32_t j = 0; j < uniform.ids.size(); j++) {
						TextureInfo *texture_info = (TextureInfo *)uniform.ids[j].id;
						uniform_texture_views[j] = texture_info->view;
					}

					WGPUBindGroupEntryExtras *entry_extras = ALLOCA_SINGLE(WGPUBindGroupEntryExtras);
					*entry_extras = (WGPUBindGroupEntryExtras){
						.chain = (WGPUChainedStruct){
								.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras,
						},
						.textureViews = uniform_texture_views,
						.textureViewCount = (size_t)uniform.ids.size(),
					};
					entry.nextInChain = (WGPUChainedStruct *)entry_extras;
				}
				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_IMAGE_BUFFER:
				CRASH_NOW_MSG("Unimplemented!"); // TODO.
				break;
			case RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER:
			case RenderingDeviceCommons::UNIFORM_TYPE_STORAGE_BUFFER: {
				WGPUBindGroupEntry entry = {};
				entry.binding = uniform.binding + binding_offset;

				BufferInfo *buffer_info = (BufferInfo *)uniform.ids[0].id;
				entry.buffer = buffer_info->buffer;
				entry.offset = 0;
				entry.size = buffer_info->size;

				entries.push_back(entry);
			} break;
			case RenderingDeviceCommons::UNIFORM_TYPE_MAX:
				break;
		}
	}

	WGPUBindGroupDescriptor bind_group_desc = (WGPUBindGroupDescriptor){
		.layout = shader_info->bind_group_layouts[p_set_index],
		.entryCount = (size_t)entries.size(),
		.entries = entries.ptr(),
	};

	WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(device, &bind_group_desc);

	return UniformSetID(bind_group);
}

void RenderingDeviceDriverWebGpu::uniform_set_free(UniformSetID p_uniform_set) {
	WGPUBindGroup bind_group = (WGPUBindGroup)p_uniform_set.id;
	wgpuBindGroupRelease(bind_group);
}

// ----- COMMANDS -----

void RenderingDeviceDriverWebGpu::command_uniform_set_prepare_for_use(CommandBufferID _p_cmd_buffer, UniformSetID _p_uniform_set, ShaderID _p_shader, uint32_t _p_set_index) {
	// Empty.
}

/******************/
/**** TRANSFER ****/
/******************/

void RenderingDeviceDriverWebGpu::command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	wgpuCommandEncoderClearBuffer(command_buffer_info->encoder, buffer_info->buffer, p_offset, p_size);

	if (buffer_info->is_transfer_first_map) {
		this->buffer_unmap(p_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *src_buffer_info = (BufferInfo *)p_src_buffer.id;
	BufferInfo *dst_buffer_info = (BufferInfo *)p_dst_buffer.id;

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferCopyRegion region = p_regions[i];
		wgpuCommandEncoderCopyBufferToBuffer(command_buffer_info->encoder, src_buffer_info->buffer, region.src_offset, dst_buffer_info->buffer, region.dst_offset, STEPIFY(region.size, 256));
	}

	if (dst_buffer_info->is_transfer_first_map) {
		this->buffer_unmap(p_dst_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout _p_src_texture_layout, TextureID p_dst_texture, TextureLayout _p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	TextureInfo *src_texture_info = (TextureInfo *)p_src_texture.id;
	TextureInfo *dst_texture_info = (TextureInfo *)p_dst_texture.id;

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		TextureCopyRegion region = p_regions[i];
		WGPUTexelCopyTextureInfo src_texture_cp = (WGPUTexelCopyTextureInfo){
			.texture = src_texture_info->texture,
			.mipLevel = region.src_subresources.mipmap,
			.origin = (WGPUOrigin3D){
					.x = (uint32_t)region.src_offset.x,
					.y = (uint32_t)region.src_offset.y,
					.z = (uint32_t)region.src_offset.z,
			},
			.aspect = webgpu_texture_aspect_from_rd(region.src_subresources.aspect),
		};
		WGPUTexelCopyTextureInfo dst_texture_cp = (WGPUTexelCopyTextureInfo){
			.texture = dst_texture_info->texture,
			.mipLevel = region.dst_subresources.mipmap,
			.origin = (WGPUOrigin3D){
					.x = (uint32_t)region.dst_offset.x,
					.y = (uint32_t)region.dst_offset.y,
					.z = (uint32_t)region.dst_offset.z,
			},
			.aspect = webgpu_texture_aspect_from_rd(region.dst_subresources.aspect),
		};
		WGPUExtent3D cp_size = (WGPUExtent3D){
			.width = (uint32_t)region.size.x,
			.height = (uint32_t)region.size.y,
			.depthOrArrayLayers = (uint32_t)region.size.z,
		};
		wgpuCommandEncoderCopyTextureToTexture(command_buffer_info->encoder, &src_texture_cp, &dst_texture_cp, &cp_size);
	}
}

void RenderingDeviceDriverWebGpu::command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}
void RenderingDeviceDriverWebGpu::command_clear_color_texture(CommandBufferID _p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}

void RenderingDeviceDriverWebGpu::command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout _p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *src_buffer_info = (BufferInfo *)p_src_buffer.id;
	TextureInfo *dst_texture_info = (TextureInfo *)p_dst_texture.id;

	FormatBlockDimension block_dimensions = webgpu_texture_format_block_dimensions(dst_texture_info->texture_view_desc.format);

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferTextureCopyRegion region = p_regions[i];

		uint32_t block_copy_size = webgpu_texture_format_block_copy_size(dst_texture_info->texture_desc.format, dst_texture_info->texture_view_desc.aspect);

		WGPUTexelCopyBufferInfo cp_buffer = (WGPUTexelCopyBufferInfo){
			.layout = (WGPUTexelCopyBufferLayout){
					.offset = region.buffer_offset,
					.bytesPerRow = ((region.texture_region_size.x * block_copy_size) / block_dimensions.block_dim_x + 255) & ~255,
					.rowsPerImage = region.texture_region_size.z > 1 ? region.texture_region_size.y / block_dimensions.block_dim_y : WGPU_COPY_STRIDE_UNDEFINED,
			},
			.buffer = src_buffer_info->buffer,
		};

		WGPUTexelCopyTextureInfo cp_texture = (WGPUTexelCopyTextureInfo){
			.texture = dst_texture_info->texture,
			.mipLevel = region.texture_subresources.mipmap,
			.origin = (WGPUOrigin3D){
					.x = (uint32_t)region.texture_offset.x,
					.y = (uint32_t)region.texture_offset.y,
					.z = (uint32_t)region.texture_offset.z,
			},
			.aspect = webgpu_texture_aspect_from_rd(region.texture_subresources.aspect),
		};
		WGPUExtent3D cp_size = (WGPUExtent3D){
			.width = (uint32_t)region.texture_region_size.x,
			.height = (uint32_t)region.texture_region_size.y,
			.depthOrArrayLayers = (uint32_t)region.texture_region_size.z,
		};

		wgpuCommandEncoderCopyBufferToTexture(command_buffer_info->encoder, &cp_buffer, &cp_texture, &cp_size);
	}

	if (src_buffer_info->is_transfer_first_map) {
		this->buffer_unmap(p_src_buffer);
	}
}

void RenderingDeviceDriverWebGpu::command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	TextureInfo *src_texture_info = (TextureInfo *)p_src_texture.id;
	BufferInfo *dst_buffer_info = (BufferInfo *)p_dst_buffer.id;

	FormatBlockDimension block_dimensions = webgpu_texture_format_block_dimensions(src_texture_info->texture_view_desc.format);

	for (uint32_t i = 0; i < p_regions.size(); i++) {
		BufferTextureCopyRegion region = p_regions[i];

		uint32_t block_copy_size = webgpu_texture_format_block_copy_size(src_texture_info->texture_desc.format, src_texture_info->texture_view_desc.aspect);

		WGPUTexelCopyTextureInfo cp_texture = (WGPUTexelCopyTextureInfo){
			.texture = src_texture_info->texture,
			.mipLevel = region.texture_subresources.mipmap,
			.origin = (WGPUOrigin3D){
					.x = (uint32_t)region.texture_offset.x,
					.y = (uint32_t)region.texture_offset.y,
					.z = (uint32_t)region.texture_offset.z,
			},
			.aspect = webgpu_texture_aspect_from_rd(region.texture_subresources.aspect),
		};

		WGPUTexelCopyBufferInfo cp_buffer = (WGPUTexelCopyBufferInfo){
			.layout = (WGPUTexelCopyBufferLayout){
					.offset = region.buffer_offset,
					.bytesPerRow = ((region.texture_region_size.x * block_copy_size) / block_dimensions.block_dim_x + 255) & ~255,
					.rowsPerImage = region.texture_region_size.z > 1 ? region.texture_region_size.y / block_dimensions.block_dim_y : WGPU_COPY_STRIDE_UNDEFINED,

			},
			.buffer = dst_buffer_info->buffer,
		};

		WGPUExtent3D cp_size = (WGPUExtent3D){
			.width = (uint32_t)region.texture_region_size.x,
			.height = (uint32_t)region.texture_region_size.y,
			.depthOrArrayLayers = (uint32_t)region.texture_region_size.z,
		};

		wgpuCommandEncoderCopyTextureToBuffer(command_buffer_info->encoder, &cp_texture, &cp_buffer, &cp_size);
	}

	if (dst_buffer_info->is_transfer_first_map) {
		this->buffer_unmap(p_dst_buffer);
	}
}

/******************/
/**** PIPELINE ****/
/******************/

void RenderingDeviceDriverWebGpu::pipeline_free(PipelineID p_pipeline) {}

// ----- BINDING -----

void RenderingDeviceDriverWebGpu::command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_first_index, VectorView<uint32_t> p_data) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	uint32_t byte_size = p_data.size() * (uint32_t)sizeof(uint32_t);
	Vector<uint8_t> data = Vector<uint8_t>();
	data.resize(byte_size);
	memcpy(data.ptrw(), p_data.ptr(), byte_size);

	if (shader_info->push_constant_stage_flags & WGPUShaderStage_Compute) {
		command_buffer_info->active_compute_pass_info.commands.push_back((ComputePassEncoderCommand){
				.type = ComputePassEncoderCommand::CommandType::SET_PUSH_CONSTANTS,
				.set_push_constants = (ComputePassEncoderCommand::SetPushConstants){
						.offset = p_first_index },
				.push_constant_data = data,
		});
	} else if (shader_info->push_constant_stage_flags & WGPUShaderStage_Vertex || shader_info->push_constant_stage_flags & WGPUShaderStage_Fragment) {
		command_buffer_info->active_render_pass_info.commands.push_back((RenderPassEncoderCommand){
				.type = RenderPassEncoderCommand::CommandType::SET_PUSH_CONSTANTS,
				.set_push_constants = (RenderPassEncoderCommand::SetPushConstants){
						.stages = shader_info->push_constant_stage_flags,
						.offset = p_first_index },
				.push_constant_data = data });
	}
}

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

RenderingDeviceDriver::RenderPassID RenderingDeviceDriverWebGpu::render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> _p_subpasses, VectorView<SubpassDependency> _p_subpass_dependencies, uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) {
	// WebGpu does not have subpasses so we will store this info until we create a render pipeline later.
	RenderPassInfo *render_pass_info = memnew(RenderPassInfo);

	render_pass_info->attachments = Vector<RenderPassAttachmentInfo>();
	for (uint32_t i = 0; i < p_attachments.size(); i++) {
		Attachment attachment = p_attachments[i];
		RenderPassAttachmentInfo attachment_info = (RenderPassAttachmentInfo){
			.format = webgpu_texture_format_from_rd(attachment.format),
			// TODO: Assert that p_format.samples follows this behavior.
			.sample_count = (uint32_t)pow(2, (uint32_t)attachment.samples),
			.load_op = webgpu_load_op_from_rd(attachment.load_op),
			.store_op = webgpu_store_op_from_rd(attachment.store_op),
			.stencil_load_op = webgpu_load_op_from_rd(attachment.stencil_load_op),
			.stencil_store_op = webgpu_store_op_from_rd(attachment.stencil_store_op),
			.is_depth_stencil =
					attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
					attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			.is_depth_stencil_read_only =
					attachment.final_layout == TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL

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

void RenderingDeviceDriverWebGpu::command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	Vector<WGPURenderPassColorAttachment> color_attachments;

	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	FramebufferInfo *framebuffer_info = (FramebufferInfo *)p_framebuffer.id;

	// DEV_ASSERT(render_pass_info->attachments.size() == framebuffer_info->attachments.size());

	WGPUTextureView maybe_surface_texture_view = nullptr;
	Pair<WGPURenderPassDepthStencilAttachment, bool> maybe_depth_stencil_attachment = Pair((WGPURenderPassDepthStencilAttachment){}, false);

	for (uint32_t i = 0; i < render_pass_info->attachments.size(); i++) {
		RenderPassAttachmentInfo attachment = render_pass_info->attachments[i];

		if (attachment.is_depth_stencil || attachment.is_depth_stencil_read_only) {
			TextureID attachment_texture_id = framebuffer_info->attachments[i];
			TextureInfo *attachment_texture = (TextureInfo *)attachment_texture_id.id;
			maybe_depth_stencil_attachment.first = (WGPURenderPassDepthStencilAttachment){
				.view = attachment_texture->view,
				.depthLoadOp = attachment.load_op,
				.depthStoreOp = attachment.store_op,
				.depthClearValue = p_clear_values[i].depth,
				.depthReadOnly = attachment.is_depth_stencil_read_only,
				.stencilLoadOp = attachment.stencil_load_op,
				.stencilStoreOp = attachment.stencil_store_op,
				.stencilClearValue = p_clear_values[i].stencil,
				.stencilReadOnly = attachment.is_depth_stencil_read_only,
			};
			maybe_depth_stencil_attachment.second = true;
		} else {
			WGPUTextureView view;
			if (framebuffer_info->maybe_swapchain) {
				SwapChainInfo *swapchain_info = (SwapChainInfo *)framebuffer_info->maybe_swapchain.id;
				RenderingContextDriverWebGpu::Surface *surface = (RenderingContextDriverWebGpu::Surface *)swapchain_info->surface;

				WGPUSurfaceTexture surface_texture;
				wgpuSurfaceGetCurrentTexture(surface->surface, &surface_texture);

				WGPUTextureView surface_texture_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
				maybe_surface_texture_view = surface_texture_view;

				view = surface_texture_view;
			} else {
				TextureID attachment_texture_id = framebuffer_info->attachments[i];
				TextureInfo *attachment_texture = (TextureInfo *)attachment_texture_id.id;
				view = attachment_texture->view;
			}

			color_attachments.push_back((WGPURenderPassColorAttachment){
					.view = view,
					.loadOp = attachment.load_op,
					.storeOp = attachment.store_op,
					.clearValue = (WGPUColor){
							.r = p_clear_values[i].color.r,
							.g = p_clear_values[i].color.g,
							.b = p_clear_values[i].color.b,
							.a = p_clear_values[i].color.a,
					},
			});
		}

		command_buffer_info->active_render_pass_info = (RenderPassEncoderInfo){
			.color_attachments = color_attachments,
			.depth_stencil_attachment = maybe_depth_stencil_attachment,
			.commands = Vector<RenderPassEncoderCommand>({}),
			.maybe_surface_texture_view = maybe_surface_texture_view,
		};
		command_buffer_info->is_render_pass_active = true;
	}
}

void RenderingDeviceDriverWebGpu::command_end_render_pass(CommandBufferID p_cmd_buffer) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	WGPURenderPassDescriptor render_pass_descriptor = (WGPURenderPassDescriptor){
		.colorAttachmentCount = (uint32_t)command_buffer_info->active_render_pass_info.color_attachments.size(),
		.colorAttachments = command_buffer_info->active_render_pass_info.color_attachments.ptr(),
		.depthStencilAttachment = command_buffer_info->active_render_pass_info.depth_stencil_attachment.second ? &command_buffer_info->active_render_pass_info.depth_stencil_attachment.first : nullptr,
	};

	WGPURenderPassEncoder render_encoder = wgpuCommandEncoderBeginRenderPass(command_buffer_info->encoder, &render_pass_descriptor);

	for (uint32_t i = 0; i < command_buffer_info->active_render_pass_info.commands.size(); i++) {
		RenderPassEncoderCommand &command = command_buffer_info->active_render_pass_info.commands.write[i];

		switch (command.type) {
			case RenderPassEncoderCommand::CommandType::SET_VIEWPORT: {
				RenderPassEncoderCommand::SetViewport data = command.set_viewport;
				wgpuRenderPassEncoderSetViewport(render_encoder, data.x, data.y, data.width, data.height, data.min_depth, data.max_depth);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_SCISSOR_RECT: {
				RenderPassEncoderCommand::SetScissorRect data = command.set_scissor_rect;
				wgpuRenderPassEncoderSetScissorRect(render_encoder, data.x, data.y, data.width, data.height);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_PIPELINE: {
				RenderPassEncoderCommand::SetPipeline data = command.set_pipeline;
				wgpuRenderPassEncoderSetPipeline(render_encoder, data.pipeline);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_BIND_GROUP: {
				RenderPassEncoderCommand::SetBindGroup data = command.set_bind_group;
				wgpuRenderPassEncoderSetBindGroup(render_encoder, data.group_index, data.bind_group, 0, nullptr);
			} break;
			case RenderPassEncoderCommand::CommandType::DRAW: {
				RenderPassEncoderCommand::Draw data = command.draw;
				wgpuRenderPassEncoderDraw(render_encoder, data.vertex_count, data.instance_count, data.first_vertex, data.first_instance);
			} break;
			case RenderPassEncoderCommand::CommandType::DRAW_INDEXED: {
				RenderPassEncoderCommand::DrawIndexed data = command.draw_indexed;
				wgpuRenderPassEncoderDrawIndexed(render_encoder, data.index_count, data.instance_count, data.first_index, data.base_vertex, data.first_instance);
			} break;
			case RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDIRECT: {
				CRASH_NOW_MSG("NO MULTIDRAW INDIRECT");
				RenderPassEncoderCommand::MultiDrawIndirect data = command.multi_draw_indirect;
				wgpuRenderPassEncoderMultiDrawIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count);
			} break;
			case RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDIRECT_COUNT: {
				CRASH_NOW_MSG("NO MULTIDRAW INDIRECT COUNT");
				RenderPassEncoderCommand::MultiDrawIndirectCount data = command.multi_draw_indirect_count;
				wgpuRenderPassEncoderMultiDrawIndirectCount(render_encoder, data.indirect_buffer, data.indirect_offset, data.count_buffer, data.count_offset, data.max_count);
			} break;
			case RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDEXED_INDIRECT: {
				CRASH_NOW_MSG("NO MULTIDRAW INDIRECT");
				RenderPassEncoderCommand::MultiDrawIndexedIndirect data = command.multi_draw_indexed_indirect;
				wgpuRenderPassEncoderMultiDrawIndexedIndirect(render_encoder, data.indirect_buffer, data.indirect_offset, data.count);
			} break;
			case RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDEXED_INDIRECT_COUNT: {
				CRASH_NOW_MSG("NO MULTIDRAW INDIRECT COUNT");
				RenderPassEncoderCommand::MultiDrawIndexedIndirectCount data = command.multi_draw_indexed_indirect_count;
				wgpuRenderPassEncoderMultiDrawIndexedIndirectCount(render_encoder, data.indirect_buffer, data.indirect_offset, data.count_buffer, data.count_offset, data.max_count);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_VERTEX_BUFFER: {
				RenderPassEncoderCommand::SetVertexBuffer data = command.set_vertex_buffer;
				wgpuRenderPassEncoderSetVertexBuffer(render_encoder, data.slot, data.buffer, data.offset, data.size);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_INDEX_BUFFER: {
				RenderPassEncoderCommand::SetIndexBuffer data = command.set_index_buffer;
				wgpuRenderPassEncoderSetIndexBuffer(render_encoder, data.buffer, data.format, data.offset, data.size);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_BLEND_CONSTANTS: {
				const RenderPassEncoderCommand::SetBlendConstant &data = command.set_blend_constant;
				wgpuRenderPassEncoderSetBlendConstant(render_encoder, &data.color);
			} break;
			case RenderPassEncoderCommand::CommandType::SET_PUSH_CONSTANTS: {
				const RenderPassEncoderCommand::SetPushConstants &data = command.set_push_constants;
				wgpuRenderPassEncoderSetPushConstants(render_encoder, data.stages, data.offset, command.push_constant_data.size(), command.push_constant_data.ptr());
			} break;
		}
	}

	command_buffer_info->is_render_pass_active = false;

	wgpuRenderPassEncoderEnd(render_encoder);
	wgpuRenderPassEncoderRelease(render_encoder);
}

void RenderingDeviceDriverWebGpu::command_next_render_subpass(CommandBufferID _p_cmd_buffer, CommandBufferType _p_cmd_buffer_type) {
	// Empty.
}

void RenderingDeviceDriverWebGpu::command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	ERR_FAIL_COND_MSG(p_viewports.size() != 1, "WebGpu cannot set multiple viewports.");

	for (uint32_t i = 0; i < p_viewports.size(); i++) {
		command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
				.type = RenderPassEncoderCommand::CommandType::SET_VIEWPORT,
				.set_viewport = (RenderPassEncoderCommand::SetViewport){
						.x = (float)p_viewports[i].position.x,
						.y = (float)p_viewports[i].position.y,
						.width = (float)p_viewports[i].size.x,
						.height = (float)p_viewports[i].size.y,
						.min_depth = 0.0,
						.max_depth = 1.0,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	ERR_FAIL_COND_MSG(p_scissors.size() != 1, "WebGpu cannot set multiple scissors.");

	for (uint32_t i = 0; i < p_scissors.size(); i++) {
		command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
				.type = RenderPassEncoderCommand::CommandType::SET_SCISSOR_RECT,
				.set_scissor_rect = (RenderPassEncoderCommand::SetScissorRect){
						.x = (uint32_t)p_scissors[i].position.x,
						.y = (uint32_t)p_scissors[i].position.y,
						.width = (uint32_t)p_scissors[i].size.width,
						.height = (uint32_t)p_scissors[i].size.height,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_clear_attachments(CommandBufferID _p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) {
	// NOTE: No easy support.
	// CRASH_NOW_MSG("NOT SUPPORTED?");
}

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	WGPURenderPipeline render_pipeline = (WGPURenderPipeline)p_pipeline.id;

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::SET_PIPELINE,

			.set_pipeline = (RenderPassEncoderCommand::SetPipeline){
					.pipeline = render_pipeline,
			} }));
}

void RenderingDeviceDriverWebGpu::command_bind_render_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID _p_shader, uint32_t p_set_index) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	WGPUBindGroup bind_group = (WGPUBindGroup)p_uniform_set.id;
	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::SET_BIND_GROUP,

			.set_bind_group = (RenderPassEncoderCommand::SetBindGroup){
					.group_index = p_set_index,
					.bind_group = bind_group,
			} }));
}

void RenderingDeviceDriverWebGpu::command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> command_bind_render_uniform_sets");
	for (uint32_t i = 0; i < p_set_count; i++) {
		command_bind_render_uniform_set(p_cmd_buffer, p_uniform_sets[i], p_shader, p_first_set_index + i);
	}
}

// Drawing.
void RenderingDeviceDriverWebGpu::command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::DRAW,
			.draw = (RenderPassEncoderCommand::Draw){
					.vertex_count = p_vertex_count,
					.instance_count = p_instance_count,
					.first_vertex = p_base_vertex,
					.first_instance = p_first_instance,
			},
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::DRAW_INDEXED,
			.draw_indexed = (RenderPassEncoderCommand::DrawIndexed){
					.index_count = p_index_count,
					.instance_count = p_instance_count,
					.first_index = p_first_index,
					.base_vertex = p_vertex_offset,
					.first_instance = p_first_instance,
			} }));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDIRECT,
			.multi_draw_indirect = (RenderPassEncoderCommand::MultiDrawIndirect){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count = p_draw_count },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;
	BufferInfo *count_buffer = (BufferInfo *)p_count_buffer.id;

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDIRECT_COUNT,
			.multi_draw_indirect_count = (RenderPassEncoderCommand::MultiDrawIndirectCount){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count_buffer = count_buffer->buffer,
					.count_offset = p_count_buffer_offset,
					.max_count = p_max_draw_count,

			},
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDEXED_INDIRECT,
			.multi_draw_indexed_indirect = (RenderPassEncoderCommand::MultiDrawIndexedIndirect){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count = p_draw_count },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t _p_stride) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *indirect_buffer = (BufferInfo *)p_indirect_buffer.id;
	BufferInfo *count_buffer = (BufferInfo *)p_count_buffer.id;

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::MULTI_DRAW_INDEXED_INDIRECT_COUNT,
			.multi_draw_indexed_indirect_count = (RenderPassEncoderCommand::MultiDrawIndexedIndirectCount){
					.indirect_buffer = indirect_buffer->buffer,
					.indirect_offset = p_offset,
					.count_buffer = count_buffer->buffer,
					.count_offset = p_count_buffer_offset,
					.max_count = p_max_draw_count,
			} }));
}

// Buffer binding.
void RenderingDeviceDriverWebGpu::command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	for (uint32_t i = 0; i < p_binding_count; i++) {
		BufferInfo *buffer_info = (BufferInfo *)p_buffers[i].id;

		command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
				.type = RenderPassEncoderCommand::CommandType::SET_VERTEX_BUFFER,
				.set_vertex_buffer = (RenderPassEncoderCommand::SetVertexBuffer){
						.slot = i,
						.buffer = buffer_info->buffer,
						.offset = p_offsets[i],
						.size = buffer_info->size,
				},
		}));
	}
}

void RenderingDeviceDriverWebGpu::command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	BufferInfo *buffer_info = (BufferInfo *)p_buffer.id;

	WGPUIndexFormat format = WGPUIndexFormat_Undefined;
	switch (p_format) {
		case RenderingDeviceCommons::INDEX_BUFFER_FORMAT_UINT16:
			format = WGPUIndexFormat_Uint16;
			break;
		case RenderingDeviceCommons::INDEX_BUFFER_FORMAT_UINT32:
			format = WGPUIndexFormat_Uint32;
			break;
	}

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::SET_INDEX_BUFFER,
			.set_index_buffer = (RenderPassEncoderCommand::SetIndexBuffer){
					.buffer = buffer_info->buffer,
					.format = format,
					.offset = p_offset,
					.size = buffer_info->size,
			} }));
}

// Dynamic state.
void RenderingDeviceDriverWebGpu::command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);
	DEV_ASSERT(command_buffer_info->is_render_pass_active == true);

	command_buffer_info->active_render_pass_info.commands.push_back(((RenderPassEncoderCommand){
			.type = RenderPassEncoderCommand::CommandType::SET_BLEND_CONSTANTS,
			.set_blend_constant = (RenderPassEncoderCommand::SetBlendConstant){
					.color = (WGPUColor){
							.r = p_constants.r,
							.g = p_constants.b,
							.b = p_constants.b,
							.a = p_constants.a,
					} },
	}));
}

void RenderingDeviceDriverWebGpu::command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) {
	// Note: This functionality is unsupported.
	// Empty.
}

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
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;
	WGPURenderPipelineDescriptor pipeline_descriptor = {};

	// pipeline_descriptor.layout
	pipeline_descriptor.layout = shader_info->pipeline_layout;

	// pipeline_descriptor.vertex
	WGPUConstantEntry *constants = ALLOCA_ARRAY(WGPUConstantEntry, p_specialization_constants.size());
	Vector<CharString> constant_keys;
	constant_keys.resize(p_specialization_constants.size());

	for (uint32_t i = 0; i < p_specialization_constants.size(); i++) {
		PipelineSpecializationConstant constant = p_specialization_constants[i];
		constant_keys.ptrw()[i] = shader_info->override_keys[constant.constant_id].ascii();
		WGPUConstantEntry entry = (WGPUConstantEntry){
			.key = { constant_keys[i].get_data(), WGPU_STRLEN },
		};

		if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT) {
			entry.value = (double)constant.float_value;
		} else if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT) {
			entry.value = (double)constant.int_value;
		} else {
			entry.value = (double)constant.bool_value;
		}

		constants[i] = entry;
	}

	WGPUVertexState vertex_state = (WGPUVertexState){
		.module = shader_info->vertex_shader,
		.entryPoint = { "main", WGPU_STRLEN },
		.constantCount = (size_t)p_specialization_constants.size(),
		.constants = constants,
		.bufferCount = 0,
	};

	// NOTE: I'm not sure dynamic vertex state is supported.
	if (p_vertex_format) {
		VertexFormatInfo *format_info = (VertexFormatInfo *)p_vertex_format.id;
		vertex_state.buffers = format_info->layouts.ptr();
		vertex_state.bufferCount = format_info->layouts.size();
	}

	pipeline_descriptor.vertex = vertex_state;

	// pipeline_descriptor.fragment
	WGPUColorTargetState *targets = ALLOCA_ARRAY(WGPUColorTargetState, p_color_attachments.size());
	size_t targets_count = 0;

	RenderPassInfo *render_pass_info = (RenderPassInfo *)p_render_pass.id;
	uint32_t render_pass_attachments_offset = 0;

	for (uint32_t i = 0; i < p_color_attachments.size(); i++) {
		if (p_color_attachments[i] != ATTACHMENT_UNUSED) {
			const PipelineColorBlendState::Attachment attachment = p_blend_state.attachments[i];
			WGPUBlendState *blend_state = ALLOCA_SINGLE(WGPUBlendState);
			*blend_state = (WGPUBlendState){
				.color =
						(WGPUBlendComponent){
								.operation = webgpu_blend_operation_from_rd(attachment.color_blend_op),
								.srcFactor = webgpu_blend_factor_from_rd(attachment.src_color_blend_factor),
								.dstFactor = webgpu_blend_factor_from_rd(attachment.dst_color_blend_factor),
						},
				.alpha =
						(WGPUBlendComponent){
								.operation = webgpu_blend_operation_from_rd(attachment.alpha_blend_op),
								.srcFactor = webgpu_blend_factor_from_rd(attachment.src_alpha_blend_factor),
								.dstFactor = webgpu_blend_factor_from_rd(attachment.dst_alpha_blend_factor),
						},
			};

			uint32_t write_mask = WGPUColorWriteMask_None;
			if (attachment.write_r) {
				write_mask |= WGPUColorWriteMask_Red;
			}
			if (attachment.write_g) {
				write_mask |= WGPUColorWriteMask_Green;
			}
			if (attachment.write_b) {
				write_mask |= WGPUColorWriteMask_Blue;
			}
			if (attachment.write_a) {
				write_mask |= WGPUColorWriteMask_Alpha;
			}

			targets[targets_count] = (WGPUColorTargetState){
				// TODO: We do not have info on color target format.
				.format = render_pass_info->attachments[i + render_pass_attachments_offset].format,
				.blend = attachment.enable_blend ? blend_state : nullptr,
				.writeMask = write_mask,
			};
			targets_count++;
		} else {
			render_pass_attachments_offset += 1;
		}
	}

	WGPUFragmentState fragment_state = (WGPUFragmentState){
		.module = shader_info->fragment_shader,
		.entryPoint = { "main", WGPU_STRLEN },
		.constantCount = (size_t)p_specialization_constants.size(),
		.constants = constants,
		.targetCount = p_color_attachments.size() - render_pass_attachments_offset,
		.targets = targets,
	};
	pipeline_descriptor.fragment = &fragment_state;

	// pipeline_descriptor.primitive
	// NOTE: We will default to `WGPUPrimitiveTopology_PointList` since not all topologies are supported.
	WGPUPrimitiveTopology topology;
	switch (p_render_primitive) {
		case RenderingDeviceCommons::RENDER_PRIMITIVE_POINTS:
			topology = WGPUPrimitiveTopology_PointList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_LINES:
			topology = WGPUPrimitiveTopology_LineList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_LINESTRIPS:
			topology = WGPUPrimitiveTopology_LineStrip;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_TRIANGLES:
			topology = WGPUPrimitiveTopology_TriangleList;
			break;
		case RenderingDeviceCommons::RENDER_PRIMITIVE_TRIANGLE_STRIPS:
			topology = WGPUPrimitiveTopology_TriangleStrip;
			break;
		default:
			topology = WGPUPrimitiveTopology_PointList;
			break;
	}

	WGPUFrontFace front_face;
	switch (p_rasterization_state.front_face) {
		case RenderingDeviceCommons::POLYGON_FRONT_FACE_CLOCKWISE:
			front_face = WGPUFrontFace_CW;
			break;
		case RenderingDeviceCommons::POLYGON_FRONT_FACE_COUNTER_CLOCKWISE:
			front_face = WGPUFrontFace_CCW;
			break;
	}

	WGPUCullMode cull_mode = WGPUCullMode_None;
	switch (p_rasterization_state.cull_mode) {
		case RenderingDeviceCommons::POLYGON_CULL_FRONT:
			cull_mode = WGPUCullMode_Front;
			break;
		case RenderingDeviceCommons::POLYGON_CULL_BACK:
			cull_mode = WGPUCullMode_Back;
			break;
		case RenderingDeviceCommons::POLYGON_CULL_DISABLED:
		case RenderingDeviceCommons::POLYGON_CULL_MAX:
			break;
	}

	WGPUPrimitiveState primitive_state = (WGPUPrimitiveState){
		.topology = topology,
		// TODO: We need this for primitive restart but currently cannot know the proper value.
		.stripIndexFormat = WGPUIndexFormat_Undefined,
		.frontFace = front_face,
		.cullMode = cull_mode,
		// TODO Consider implementing wireframe rendering (required native feature).
		// TODO Consider implementing `p_rasterization_state.enable_depth_clamp` (required native feature).

	};
	pipeline_descriptor.primitive = primitive_state;

	// pipeline_descriptor.depth_stencil
	WGPUDepthStencilState depth_stencil_state;

	// HACK: Whether a depth stencil state should be attached isn't always reported, so let's walk the render pass'
	// attachments to double check.
	bool use_depth = p_depth_stencil_state.enable_depth_write || p_depth_stencil_state.enable_stencil;

	for (uint32_t i = 0; i < render_pass_info->attachments.size(); i++) {
		const RenderPassAttachmentInfo &attachment = render_pass_info->attachments[i];
		if (attachment.is_depth_stencil || attachment.is_depth_stencil_read_only) {
			use_depth = true;
			break;
		}
	}

	if (use_depth) {
		depth_stencil_state = (WGPUDepthStencilState){
			// TODO: We do not have info on depth target format.
			.format = WGPUTextureFormat_Depth32Float,
			.depthWriteEnabled = p_depth_stencil_state.enable_depth_write ? WGPUOptionalBool_True : WGPUOptionalBool_False,
			.depthCompare = webgpu_compare_mode_from_rd(p_depth_stencil_state.depth_compare_operator),
			.stencilFront =
					(WGPUStencilFaceState){
							.compare = webgpu_compare_mode_from_rd(p_depth_stencil_state.front_op.compare),
							.failOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.fail),
							.depthFailOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.depth_fail),
							.passOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.front_op.pass),
					},
			.stencilBack =
					(WGPUStencilFaceState){
							.compare = webgpu_compare_mode_from_rd(p_depth_stencil_state.back_op.compare),
							.failOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.fail),
							.depthFailOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.depth_fail),
							.passOp = webgpu_stencil_operation_from_rd(p_depth_stencil_state.back_op.pass),
					},
			// NOTE: We assume stencil read masks are the same for both front and back.
			// This is how wgpu does it, see https://github.com/gfx-rs/wgpu/blob/6405dcf611a336eb7d3bf9de7b78d7d0b3d3b48d/wgpu-hal/src/vulkan/device.rs#L1778
			.stencilReadMask = p_depth_stencil_state.front_op.compare_mask,
			.stencilWriteMask = p_depth_stencil_state.front_op.write_mask,
			.depthBias = (int32_t)p_rasterization_state.depth_bias_constant_factor,
			.depthBiasSlopeScale = p_rasterization_state.depth_bias_slope_factor,
			.depthBiasClamp = p_rasterization_state.depth_bias_clamp,
		};
		pipeline_descriptor.depthStencil = &depth_stencil_state;
	} else {
		pipeline_descriptor.depthStencil = nullptr;
	}

	// pipeline_descriptor.multisample
	// NOTE: In a future version of wgpu, multisample.mask will be `u64`.
	static_assert(sizeof(WGPUMultisampleState) == 24);
	// TODO: Assert that p_format.samples follows this behavior.
	uint32_t sample_count = pow(2, (uint32_t)p_multisample_state.sample_count);
	pipeline_descriptor.multisample = (WGPUMultisampleState){
		.count = sample_count,
		.mask = p_multisample_state.sample_mask.size() ? *p_multisample_state.sample_mask.ptr() : ~0,
		.alphaToCoverageEnabled = p_multisample_state.enable_alpha_to_coverage,
	};

	// pipeline_descriptor.multiview
	// TODO: Implement render pipeline multiview.

	WGPURenderPipeline render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_descriptor);
	ERR_FAIL_COND_V(!render_pipeline, PipelineID());

	return PipelineID(render_pipeline);
}

/*****************/
/**** COMPUTE ****/
/*****************/

// ----- COMMANDS -----

// Binding.
void RenderingDeviceDriverWebGpu::command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	WGPUComputePipeline pipeline = (WGPUComputePipeline)p_pipeline.id;

	command_buffer_info->active_compute_pass_info.commands.push_back((ComputePassEncoderCommand){
			.type = ComputePassEncoderCommand::CommandType::SET_PIPELINE,
			.set_pipeline = (ComputePassEncoderCommand::SetPipeline){
					.pipeline = pipeline,
			} });
}
void RenderingDeviceDriverWebGpu::command_bind_compute_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	WGPUBindGroup bind_group = (WGPUBindGroup)p_uniform_set.id;

	command_buffer_info->active_compute_pass_info.commands.push_back((ComputePassEncoderCommand){
			.type = ComputePassEncoderCommand::CommandType::SET_BIND_GROUP,
			.set_bind_group = (ComputePassEncoderCommand::SetBindGroup){
					.index = p_set_index,
					.bind_group = bind_group,
			},
	});
}

void RenderingDeviceDriverWebGpu::command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> command_bind_compute_uniform_sets");
	for (uint32_t i = 0; i < p_set_count; i++) {
		command_bind_compute_uniform_set(p_cmd_buffer, p_uniform_sets[i], p_shader, p_first_set_index + i);
	}
}

// Dispatching.
void RenderingDeviceDriverWebGpu::command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	command_buffer_info->active_compute_pass_info.commands.push_back((ComputePassEncoderCommand){
			.type = ComputePassEncoderCommand::CommandType::DISPATCH_WORKGROUPS,
			.dispatch_workgroups = (ComputePassEncoderCommand::DispatchWorkgroups){
					.workgroup_count_x = p_x_groups,
					.workgroup_count_y = p_y_groups,
					.workgroup_count_z = p_z_groups,
			},
	});
}
void RenderingDeviceDriverWebGpu::command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) {
	DEV_ASSERT(p_cmd_buffer.id != 0);
	CommandBufferInfo *command_buffer_info = (CommandBufferInfo *)p_cmd_buffer.id;
	DEV_ASSERT(command_buffer_info->encoder != nullptr);

	BufferInfo *buffer_info = (BufferInfo *)p_indirect_buffer.id;

	command_buffer_info->active_compute_pass_info.commands.push_back((ComputePassEncoderCommand){
			.type = ComputePassEncoderCommand::CommandType::DISPATCH_WORKGROUPS_INDIRECT,
			.dispatch_workgroups_indirect = (ComputePassEncoderCommand::DispatchWorkgroupsIndirect){
					.indirect_buffer = buffer_info->buffer,
					.indirect_offset = p_offset,
			},
	});
}

// ----- PIPELINE -----

RenderingDeviceDriver::PipelineID RenderingDeviceDriverWebGpu::compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	ShaderInfo *shader_info = (ShaderInfo *)p_shader.id;

	ERR_FAIL_COND_V_MSG(!shader_info->compute_shader, PipelineID(), "Compute pipeline shader null.");

	Vector<WGPUConstantEntry> constants;
	Vector<CharString> constant_keys;
	constant_keys.resize(p_specialization_constants.size());
	for (uint32_t i = 0; i < p_specialization_constants.size(); i++) {
		PipelineSpecializationConstant constant = p_specialization_constants[i];
		constant_keys.ptrw()[i] = shader_info->override_keys[constant.constant_id].ascii();
		WGPUConstantEntry entry = (WGPUConstantEntry){
			.key = { constant_keys[i].get_data(), WGPU_STRLEN },
		};

		if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT) {
			entry.value = (double)constant.float_value;
		} else if (constant.type == PipelineSpecializationConstantType::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT) {
			entry.value = (double)constant.int_value;
		} else {
			entry.value = (double)constant.bool_value;
		}

		constants.push_back(entry);
	}

	WGPUProgrammableStageDescriptor programmable_stage_desc = (WGPUProgrammableStageDescriptor){
		.module = shader_info->compute_shader,
		.entryPoint = { "main", WGPU_STRLEN },
		.constantCount = (size_t)constants.size(),
		.constants = constants.ptr(),
	};

	WGPUComputePipelineDescriptor compute_pipeline_descriptor = (WGPUComputePipelineDescriptor){
		.layout = shader_info->pipeline_layout,
		.compute = programmable_stage_desc,
	};

	WGPUComputePipeline compute_pipeline = wgpuDeviceCreateComputePipeline(device, &compute_pipeline_descriptor);

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

/****************/
/**** DEBUG *****/
/****************/

void RenderingDeviceDriverWebGpu::command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) {
	// TODO: impl
	// CRASH_NOW_MSG("TODO --> command_insert_breadcrumb");
}

/********************/
/**** SUBMISSION ****/
/********************/

void RenderingDeviceDriverWebGpu::begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) {}
void RenderingDeviceDriverWebGpu::end_segment() {}

/**************/
/**** MISC ****/
/**************/

void RenderingDeviceDriverWebGpu::set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) {}
uint64_t RenderingDeviceDriverWebGpu::get_resource_native_handle(DriverResource p_type, ID p_driver_id) {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::get_total_memory_used() {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::get_lazily_memory_used() {
	// TODO: impl
	return 0;
}

uint64_t RenderingDeviceDriverWebGpu::limit_get(Limit p_limit) {
	WGPUNativeLimits extras;
	WGPULimits limits;
	limits.nextInChain = (WGPUChainedStructOut *)&extras;
	wgpuDeviceGetLimits(device, &limits);
	return rd_limit_from_webgpu(p_limit, limits);
}

uint64_t RenderingDeviceDriverWebGpu::api_trait_get(ApiTrait p_trait) {
	switch (p_trait) {
		case API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT:
			return 256;
		case API_TRAIT_HONORS_PIPELINE_BARRIERS:
			return 0;
		case API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP:
			return 256;
		default:
			return RenderingDeviceDriver::api_trait_get(p_trait);
	}
}

bool RenderingDeviceDriverWebGpu::has_feature(Features p_feature) {
	// TODO
	return false;
}

const RenderingDeviceDriver::MultiviewCapabilities &RenderingDeviceDriverWebGpu::get_multiview_capabilities() {
	return multiview_capabilities;
}

const RenderingDeviceDriver::FragmentShadingRateCapabilities &RenderingDeviceDriverWebGpu::get_fragment_shading_rate_capabilities() {
	return fsr_capabilities;
}

const RenderingDeviceDriver::FragmentDensityMapCapabilities &RenderingDeviceDriverWebGpu::get_fragment_density_map_capabilities() {
	return fdm_capabilities;
}

String RenderingDeviceDriverWebGpu::get_api_name() const {
	return "WebGpu";
}

String RenderingDeviceDriverWebGpu::get_api_version() const {
	// TODO: We should compile this in based on the wgpu / dawn version
	return "v24.0.0 (wgpu)";
}

String RenderingDeviceDriverWebGpu::get_pipeline_cache_uuid() const {
	// TODO: impl
	return "webgpu";
}

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
