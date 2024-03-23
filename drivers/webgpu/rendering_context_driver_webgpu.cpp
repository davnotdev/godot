#include "rendering_context_driver_webgpu.h"
#include "core/error/error_macros.h"
#include <webgpu.h>

static void handleRequestAdapter(WGPURequestAdapterStatus status,
		WGPUAdapter adapter, char const *message,
		void *userdata) {
	RenderingContextDriverWebGpu *context = (RenderingContextDriverWebGpu *)userdata;
	context->adapter_set(
			adapter);
	ERR_FAIL_COND_V_MSG(
			status != WGPURequestAdapterStatus_Success, (void)0,
			vformat("Failed to get wgpu adapter: %s", message));
}

RenderingContextDriverWebGpu::RenderingContextDriverWebGpu() {
}

RenderingContextDriverWebGpu::~RenderingContextDriverWebGpu() {
}

Error RenderingContextDriverWebGpu::initialize() {
	instance = wgpuCreateInstance(nullptr);

	WGPURequestAdapterOptions adapter_options = {};
	wgpuInstanceRequestAdapter(instance,
			&adapter_options,
			handleRequestAdapter, this);
	return OK;
}

const RenderingContextDriver::Device &RenderingContextDriverWebGpu::device_get(uint32_t p_device_index) const {
	DEV_ASSERT(false)
	//  TODO
}

uint32_t RenderingContextDriverWebGpu::device_get_count() const {
	DEV_ASSERT(false)
	//  TODO
}

bool RenderingContextDriverWebGpu::device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const {
	DEV_ASSERT(false)
	//  TODO
}

RenderingDeviceDriver *RenderingContextDriverWebGpu::driver_create() {
	DEV_ASSERT(false)
	//  TODO
}

void RenderingContextDriverWebGpu::driver_free(RenderingDeviceDriver *p_driver) {
	DEV_ASSERT(false)
	//  TODO
}

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpu::surface_create(const void *p_platform_data) {
	DEV_ASSERT(false && "Surface creation should not be called on the platform-agnostic version of the driver.");
	return SurfaceID();
}

void RenderingContextDriverWebGpu::surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) {
	DEV_ASSERT(false)
	//  TODO
}

void RenderingContextDriverWebGpu::surface_set_vsync_mode(SurfaceID p_surface, DisplayServer::VSyncMode p_vsync_mode) {
	DEV_ASSERT(false)
	//  TODO
}

DisplayServer::VSyncMode RenderingContextDriverWebGpu::surface_get_vsync_mode(SurfaceID p_surface) const {
	DEV_ASSERT(false)
	//  TODO
	return DisplayServer::VSyncMode::VSYNC_DISABLED;
}

uint32_t RenderingContextDriverWebGpu::surface_get_width(SurfaceID p_surface) const {
	DEV_ASSERT(false)
	//  TODO
	return 0;
}

uint32_t RenderingContextDriverWebGpu::surface_get_height(SurfaceID p_surface) const {
	DEV_ASSERT(false)
	//  TODO
	return 0;
}

void RenderingContextDriverWebGpu::surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) {
	DEV_ASSERT(false)
	//  TODO
}

bool RenderingContextDriverWebGpu::surface_get_needs_resize(SurfaceID p_surface) const {
	DEV_ASSERT(false)
	//  TODO
	return false;
}

void RenderingContextDriverWebGpu::surface_destroy(SurfaceID p_surface) {
	DEV_ASSERT(false)
	//  TODO
}
bool RenderingContextDriverWebGpu::is_debug_utils_enabled() const {
	DEV_ASSERT(false)
	//  TODO
	return false;
}

void RenderingContextDriverWebGpu::adapter_set(WGPUAdapter new_adapter) {
	adapter = new_adapter;
}

WGPUInstance RenderingContextDriverWebGpu::instance_get() const {
	return instance;
}
