#ifndef RENDERING_DEVICE_DRIVER_WEBGPU_H
#define RENDERING_DEVICE_DRIVER_WEBGPU_H

#include "core/templates/hash_map.h"
#include "drivers/webgpu/rendering_context_driver_webgpu.h"

#include "servers/rendering/rendering_context_driver.h"
#include "servers/rendering/rendering_device_driver.h"

class RenderingDeviceDriverWebGpu : public RenderingDeviceDriver {
	WGPUDevice device = nullptr;
	WGPUAdapter adapter = nullptr;
	WGPUQueue queue = nullptr;
	RenderingContextDriverWebGpu *context_driver = nullptr;
	RenderingContextDriver::Device context_device;

	Capabilities capabilties;
	MultiviewCapabilities multiview_capabilities;
	FragmentShadingRateCapabilities fsr_capabilities;
	FragmentDensityMapCapabilities fdm_capabilities;

public:
	Error initialize(uint32_t p_device_index, uint32_t p_frame_count) override final;

private:
	/*****************/
	/**** BUFFERS ****/
	/*****************/

	struct BufferInfo {
		WGPUBuffer buffer;
		WGPUMapMode map_mode;
		uint64_t size;

		// Transfer buffers will be mapped on creation.
		bool is_transfer_first_map;
	};

public:
	virtual BufferID buffer_create(uint64_t p_size, BitField<BufferUsageBits> p_usage, MemoryAllocationType p_allocation_type) override final;
	virtual bool buffer_set_texel_format(BufferID p_buffer, DataFormat p_format) override final;
	virtual void buffer_free(BufferID p_buffer) override final;
	virtual uint64_t buffer_get_allocation_size(BufferID p_buffer) override final;
	virtual uint8_t *buffer_map(BufferID p_buffer) override final;
	virtual void buffer_unmap(BufferID p_buffer) override final;
	virtual uint64_t buffer_get_device_address(BufferID p_buffer) override final;

	/*****************/
	/**** TEXTURE ****/
	/*****************/

private:
	struct TextureInfo {
	public:
		WGPUTexture texture;
		WGPUTextureView view;

		// TODO: nextInChain is not preserved.
		WGPUTextureDescriptor texture_desc;
		WGPUTextureViewDescriptor texture_view_desc;

		RDD::DataFormat rd_texture_format;

		bool is_original_texture;
		bool is_using_depth;
	};

	// Keep track of existing mirror textures to ensure we don't write to a deleted texture.
	HashSet<TextureID> mirror_textures;

public:
	virtual TextureID texture_create(const TextureFormat &p_format, const TextureView &p_view) override final;
	virtual TextureID texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) override final;
	virtual TextureID texture_create_shared(TextureID p_original_texture, const TextureView &p_view) override final;
	virtual TextureID texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) override final;
	virtual void texture_free(TextureID p_texture) override final;
	virtual uint64_t texture_get_allocation_size(TextureID p_texture) override final;
	virtual void texture_get_copyable_layout(TextureID p_texture, const TextureSubresource &p_subresource, TextureCopyableLayout *r_layout) override final;
	virtual uint8_t *texture_map(TextureID p_texture, const TextureSubresource &p_subresource) override final;
	virtual void texture_unmap(TextureID p_texture) override final;
	virtual BitField<TextureUsageBits> texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) override final;
	virtual bool texture_can_make_shared_with_format(TextureID p_texture, DataFormat p_format, bool &r_raw_reinterpretation) override final;

	/*****************/
	/**** SAMPLER ****/
	/*****************/

public:
	virtual SamplerID sampler_create(const SamplerState &p_state) override final;
	virtual void sampler_free(SamplerID p_sampler) override final;
	virtual bool sampler_is_format_supported_for_filter(DataFormat p_format, SamplerFilter p_filter) override final;

	/**********************/
	/**** VERTEX ARRAY ****/
	/**********************/

private:
	struct VertexFormatInfo {
		Vector<WGPUVertexBufferLayout> layouts;
		Vector<WGPUVertexAttribute> vertex_attributes;
	};

public:
	virtual VertexFormatID vertex_format_create(VectorView<VertexAttribute> p_vertex_attribs) override final;
	virtual void vertex_format_free(VertexFormatID p_vertex_format) override final;

	/******************/
	/**** BARRIERS ****/
	/******************/

public:
	virtual void command_pipeline_barrier(
			CommandBufferID p_cmd_buffer,
			BitField<PipelineStageBits> p_src_stages,
			BitField<PipelineStageBits> p_dst_stages,
			VectorView<MemoryBarrier> p_memory_barriers,
			VectorView<BufferBarrier> p_buffer_barriers,
			VectorView<TextureBarrier> p_texture_barriers) override final;

	/****************/
	/**** FENCES ****/
	/****************/

public:
	virtual FenceID fence_create() override final;
	virtual Error fence_wait(FenceID p_fence) override final;
	virtual void fence_free(FenceID p_fence) override final;

	/********************/
	/**** SEMAPHORES ****/
	/********************/

	virtual SemaphoreID semaphore_create() override final;
	virtual void semaphore_free(SemaphoreID p_semaphore) override final;

	/******************/
	/**** COMMANDS ****/
	/******************/

	// ----- QUEUE FAMILY -----

	virtual CommandQueueFamilyID command_queue_family_get(BitField<CommandQueueFamilyBits> p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID p_surface = 0) override final;

	// ----- QUEUE -----
public:
	virtual CommandQueueID command_queue_create(CommandQueueFamilyID p_cmd_queue_family, bool p_identify_as_main_queue = false) override final;
	virtual Error command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) override final;
	virtual void command_queue_free(CommandQueueID p_cmd_queue) override final;

	// ----- POOL -----

public:
	virtual CommandPoolID command_pool_create(CommandQueueFamilyID p_cmd_queue_family, CommandBufferType p_cmd_buffer_type) override final;
	virtual bool command_pool_reset(CommandPoolID p_cmd_pool) override final;
	virtual void command_pool_free(CommandPoolID p_cmd_pool) override final;

	// ----- BUFFER -----

private:
	// We defer beginning a render / compute pass
	// because when we begin a pass, there is info we do not yet know.
	struct RenderPassEncoderCommand {
	public:
		enum class CommandType {
			SET_VIEWPORT,
			SET_SCISSOR_RECT,
			SET_PIPELINE,
			SET_BIND_GROUP,
			DRAW,
			DRAW_INDEXED,
			MULTI_DRAW_INDIRECT,
			MULTI_DRAW_INDIRECT_COUNT,
			MULTI_DRAW_INDEXED_INDIRECT,
			MULTI_DRAW_INDEXED_INDIRECT_COUNT,
			SET_VERTEX_BUFFER,
			SET_INDEX_BUFFER,
			SET_BLEND_CONSTANTS,
			SET_PUSH_CONSTANTS,
		};

		CommandType type;

		struct SetViewport {
			float x;
			float y;
			float width;
			float height;
			float min_depth;
			float max_depth;
		};

		struct SetScissorRect {
			uint32_t x;
			uint32_t y;
			uint32_t width;
			uint32_t height;
		};

		struct SetPipeline {
			WGPURenderPipeline pipeline;
		};

		struct SetBindGroup {
			uint32_t group_index;
			WGPUBindGroup bind_group;
		};

		struct Draw {
			uint32_t vertex_count;
			uint32_t instance_count;
			uint32_t first_vertex;
			uint32_t first_instance;
		};

		struct DrawIndexed {
			uint32_t index_count;
			uint32_t instance_count;
			uint32_t first_index;
			int32_t base_vertex;
			uint32_t first_instance;
		};

		struct MultiDrawIndirect {
			WGPUBuffer indirect_buffer;
			uint64_t indirect_offset;
			uint32_t count;
		};

		struct MultiDrawIndirectCount {
			WGPUBuffer indirect_buffer;
			uint64_t indirect_offset;
			WGPUBuffer count_buffer;
			uint64_t count_offset;
			uint32_t max_count;
		};

		struct MultiDrawIndexedIndirect {
			WGPUBuffer indirect_buffer;
			uint64_t indirect_offset;
			uint32_t count;
		};

		struct MultiDrawIndexedIndirectCount {
			WGPUBuffer indirect_buffer;
			uint64_t indirect_offset;
			WGPUBuffer count_buffer;
			uint64_t count_offset;
			uint32_t max_count;
		};

		struct SetVertexBuffer {
			uint32_t slot;
			WGPUBuffer buffer;
			uint64_t offset;
			uint64_t size;
		};

		struct SetIndexBuffer {
			WGPUBuffer buffer;
			WGPUIndexFormat format;
			uint64_t offset;
			uint64_t size;
		};

		struct SetBlendConstant {
			WGPUColor color;
		};

		struct SetPushConstants {
			WGPUShaderStage stages;
			uint32_t offset;
		};

		union {
			SetViewport set_viewport;
			SetScissorRect set_scissor_rect;
			SetPipeline set_pipeline;
			SetBindGroup set_bind_group;
			Draw draw;
			DrawIndexed draw_indexed;
			MultiDrawIndirect multi_draw_indirect;
			MultiDrawIndexedIndirect multi_draw_indexed_indirect;
			MultiDrawIndirectCount multi_draw_indirect_count;
			MultiDrawIndexedIndirectCount multi_draw_indexed_indirect_count;
			SetVertexBuffer set_vertex_buffer;
			SetIndexBuffer set_index_buffer;
			SetBlendConstant set_blend_constant;
			SetPushConstants set_push_constants;
		};

		Vector<uint8_t> push_constant_data;
	};

	struct RenderPassEncoderInfo {
		Vector<WGPURenderPassColorAttachment> color_attachments;
		Pair<WGPURenderPassDepthStencilAttachment, bool> depth_stencil_attachment;
		Vector<RenderPassEncoderCommand> commands;
		WGPUTextureView maybe_surface_texture_view;
	};

	struct ComputePassEncoderCommand {
		enum class CommandType {
			SET_PIPELINE,
			SET_BIND_GROUP,
			SET_PUSH_CONSTANTS,
			DISPATCH_WORKGROUPS,
			DISPATCH_WORKGROUPS_INDIRECT,
		};

		struct SetPipeline {
			WGPUComputePipeline pipeline;
		};

		struct SetBindGroup {
			uint32_t index;
			WGPUBindGroup bind_group;
		};

		struct SetPushConstants {
			uint32_t offset;
		};

		struct DispatchWorkgroups {
			uint32_t workgroup_count_x;
			uint32_t workgroup_count_y;
			uint32_t workgroup_count_z;
		};

		struct DispatchWorkgroupsIndirect {
			WGPUBuffer indirect_buffer;
			uint64_t indirect_offset;
		};

		CommandType type;

		union {
			SetBindGroup set_bind_group;
			SetPipeline set_pipeline;
			SetPushConstants set_push_constants;
			DispatchWorkgroups dispatch_workgroups;
			DispatchWorkgroupsIndirect dispatch_workgroups_indirect;
		};

		Vector<uint8_t> push_constant_data;
	};

	struct ComputePassEncoderInfo {
		Vector<ComputePassEncoderCommand> commands;
	};

	struct CommandBufferInfo {
		WGPUCommandEncoder encoder = nullptr;
		bool is_render_pass_active = false;
		RenderPassEncoderInfo active_render_pass_info = RenderPassEncoderInfo();
		ComputePassEncoderInfo active_compute_pass_info = ComputePassEncoderInfo();
	};

public:
	virtual CommandBufferID command_buffer_create(CommandPoolID p_cmd_pool) override final;
	virtual bool command_buffer_begin(CommandBufferID p_cmd_buffer) override final;
	virtual bool command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) override final;
	virtual void command_buffer_end(CommandBufferID p_cmd_buffer) override final;
	virtual void command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) override final;

	/********************/
	/**** SWAP CHAIN ****/
	/********************/

private:
	struct SwapChainInfo {
		RenderingContextDriver::SurfaceID surface;
		RenderPassID render_pass;
	};

public:
	virtual SwapChainID swap_chain_create(RenderingContextDriver::SurfaceID p_surface) override final;
	virtual Error swap_chain_resize(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, uint32_t p_desired_framebuffer_count) override final;
	virtual FramebufferID swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) override final;
	virtual RenderPassID swap_chain_get_render_pass(SwapChainID p_swap_chain) override final;
	virtual DataFormat swap_chain_get_format(SwapChainID p_swap_chain) override final;
	virtual void swap_chain_free(SwapChainID p_swap_chain) override final;

	/*********************/
	/**** FRAMEBUFFER ****/
	/*********************/

private:
	struct FramebufferInfo {
		SwapChainID maybe_swapchain;

		Vector<TextureID> attachments;
	};

public:
	virtual FramebufferID framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t p_width, uint32_t p_height) override final;
	virtual void framebuffer_free(FramebufferID p_framebuffer) override final;

	/****************/
	/**** SHADER ****/
	/****************/

private:
	struct ShaderBinary {
		// Version 1: initial.
		static const uint32_t VERSION = 1;

		struct DataBinding {
			uint32_t type = 0;
			uint32_t binding = 0;
			uint32_t stages = 0;
			uint32_t length = 0; // Size of arrays (in total elements), or UBOs (in bytes * total elements).
			uint32_t writable = 0;

			uint32_t image_format = 0;
			uint32_t image_access = 0;
			uint32_t texture_image_type = 0;
			uint32_t texture_is_multisample = 0;
		};

		struct SpecializationConstant {
			static const size_t OVERRIDE_CONSTANT_STRLEN = 48;
			// NOTE: This is based on the current longest override variable at 39 characters.
			// char[48] = uint32_t[12]
			char value_name[OVERRIDE_CONSTANT_STRLEN] = { 0 };
			uint32_t type = 0;
			uint32_t constant_id = 0;
			uint32_t int_value = 0;
			uint32_t stage_flags = 0;
		};

		struct Data {
			uint64_t vertex_input_mask = 0;
			uint32_t fragment_output_mask = 0;
			uint32_t specialization_constants_count = 0;
			uint32_t is_compute = 0;
			uint32_t compute_local_size[3] = {};
			uint32_t set_count = 0;
			uint32_t push_constant_size = 0;
			uint32_t push_constant_stages = 0;
			uint32_t stage_count = 0;
			uint32_t shader_name_len = 0;
		};
	};

	struct ShaderInfo {
		WGPU_NULLABLE WGPUShaderModule vertex_shader;
		WGPU_NULLABLE WGPUShaderModule fragment_shader;
		WGPU_NULLABLE WGPUShaderModule compute_shader;

		WGPUShaderStage stage_flags;

		Vector<WGPUBindGroupLayout> bind_group_layouts;
		// Maps `constant_id` to override key name
		HashMap<uint32_t, String> override_keys;
		WGPUPipelineLayout pipeline_layout;
	};

public:
	virtual String shader_get_binary_cache_key() override final;
	virtual Vector<uint8_t> shader_compile_binary_from_spirv(VectorView<ShaderStageSPIRVData> p_spirv, const String &p_shader_name) override final;
	virtual ShaderID shader_create_from_bytecode(const Vector<uint8_t> &p_shader_binary, ShaderDescription &r_shader_desc, String &r_name, const Vector<ImmutableSampler> &p_immutable_samplers) override final;
	virtual void shader_free(ShaderID p_shader) override final;
	virtual void shader_destroy_modules(ShaderID p_shader) override final;

	/*********************/
	/**** UNIFORM SET ****/
	/*********************/

public:
	virtual UniformSetID uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index, int p_linear_pool_index) override final;
	virtual void uniform_set_free(UniformSetID p_uniform_set) override final;

	// ----- COMMANDS -----

	virtual void command_uniform_set_prepare_for_use(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) override final;

	/******************/
	/**** TRANSFER ****/
	/******************/

	virtual void command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) override final;
	virtual void command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) override final;

	virtual void command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) override final;
	virtual void command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) override final;
	virtual void command_clear_color_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) override final;

	virtual void command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) override final;
	virtual void command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) override final;

	/******************/
	/**** PIPELINE ****/
	/******************/
public:
	virtual void pipeline_free(PipelineID p_pipeline) override final;

	// ----- BINDING -----

	virtual void command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_first_index, VectorView<uint32_t> p_data) override final;

	// ----- CACHE -----

	virtual bool pipeline_cache_create(const Vector<uint8_t> &p_data) override final;
	virtual void pipeline_cache_free() override final;
	virtual size_t pipeline_cache_query_size() override final;
	virtual Vector<uint8_t> pipeline_cache_serialize() override final;

	/*******************/
	/**** RENDERING ****/
	/*******************/

	// ----- SUBPASS -----

private:
	struct RenderPassAttachmentInfo {
		WGPUTextureFormat format;
		uint32_t sample_count;
		WGPULoadOp load_op;
		WGPUStoreOp store_op;
		WGPULoadOp stencil_load_op;
		WGPUStoreOp stencil_store_op;
		bool is_depth_stencil;
		bool is_depth_stencil_read_only;
	};

	struct RenderPassInfo {
		Vector<RenderPassAttachmentInfo> attachments;
		uint32_t view_count;
	};

	virtual RenderPassID render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> p_subpasses, VectorView<SubpassDependency> p_subpass_dependencies, uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) override final;
	virtual void render_pass_free(RenderPassID p_render_pass) override final;

	// ----- COMMANDS -----

public:
	virtual void command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) override final;
	virtual void command_end_render_pass(CommandBufferID p_cmd_buffer) override final;
	virtual void command_next_render_subpass(CommandBufferID p_cmd_buffer, CommandBufferType p_cmd_buffer_type) override final;
	virtual void command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) override final;
	virtual void command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) override final;
	virtual void command_render_clear_attachments(CommandBufferID p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) override final;

	// Binding.
	virtual void command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) override final;
	virtual void command_bind_render_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) override final;
	virtual void command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) override final;

	// Drawing.
	virtual void command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) override final;
	virtual void command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) override final;
	virtual void command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override final;

	// Buffer binding.
	virtual void command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets) override final;
	virtual void command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) override final;

	// Dynamic state.
	virtual void command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) override final;
	virtual void command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) override final;

	// ----- PIPELINE -----

	virtual PipelineID render_pipeline_create(
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
			VectorView<PipelineSpecializationConstant> p_specialization_constants) override final;

	/*****************/
	/**** COMPUTE ****/
	/*****************/

	// ----- COMMANDS -----

public:
	// Binding.
	virtual void command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) override final;
	virtual void command_bind_compute_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) override final;
	virtual void command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) override final;

	// Dispatching.
	virtual void command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) override final;
	virtual void command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) override final;

	// ----- PIPELINE -----

	virtual PipelineID compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) override final;

	/*****************/
	/**** QUERIES ****/
	/*****************/

	// ----- TIMESTAMP -----

	// Basic.
	virtual QueryPoolID timestamp_query_pool_create(uint32_t p_query_count) override final;
	virtual void timestamp_query_pool_free(QueryPoolID p_pool_id) override final;
	virtual void timestamp_query_pool_get_results(QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t *r_results) override final;
	virtual uint64_t timestamp_query_result_to_time(uint64_t p_result) override final;

	// Commands.
	virtual void command_timestamp_query_pool_reset(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) override final;
	virtual void command_timestamp_write(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) override final;

	/****************/
	/**** LABELS ****/
	/****************/

	virtual void command_begin_label(CommandBufferID p_cmd_buffer, const char *p_label_name, const Color &p_color) override final;
	virtual void command_end_label(CommandBufferID p_cmd_buffer) override final;

	/****************/
	/**** DEBUG *****/
	/****************/

	virtual void command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) override final;

	/********************/
	/**** SUBMISSION ****/
	/********************/

	virtual void begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) override final;
	virtual void end_segment() override final;

	/**************/
	/**** MISC ****/
	/**************/

	virtual void set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) override final;
	virtual uint64_t get_resource_native_handle(DriverResource p_type, ID p_driver_id) override final;
	virtual uint64_t get_total_memory_used() override final;
	virtual uint64_t get_lazily_memory_used() override final;
	virtual uint64_t limit_get(Limit p_limit) override final;
	virtual uint64_t api_trait_get(ApiTrait p_trait) override final;
	virtual bool has_feature(Features p_feature) override final;
	virtual const MultiviewCapabilities &get_multiview_capabilities() override final;
	virtual const FragmentShadingRateCapabilities &get_fragment_shading_rate_capabilities() override final;
	virtual const FragmentDensityMapCapabilities &get_fragment_density_map_capabilities() override final;
	virtual String get_api_name() const override final;
	virtual String get_api_version() const override final;
	virtual String get_pipeline_cache_uuid() const override final;
	virtual const Capabilities &get_capabilities() const override final;

public:
	RenderingDeviceDriverWebGpu(RenderingContextDriverWebGpu *p_context_driver);
	virtual ~RenderingDeviceDriverWebGpu();
};

#endif // RENDERING_DEVICE_DRIVER_WEBGPU_H
