#include "webgpu.h"

#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu.h"

#include "core/error/error_macros.h"

#include "rendering_device_driver_webgpu.h"

static void handle_request_adapter(WGPURequestAdapterStatus status,
		WGPUAdapter adapter, WGPUStringView message,
		void *userdata, void *_) {
	ERR_FAIL_COND_V_MSG(
			status != WGPURequestAdapterStatus_Success, (void)0,
			vformat("Failed to get wgpu adapter: %s", message.data));

	WGPUAdapterInfo info;
	wgpuAdapterGetInfo(adapter, &info);

	RenderingContextDriver::Device device;
	device.name = String(info.device.data);
	device.vendor = info.vendorID;
	device.type = (RenderingContextDriver::DeviceType)info.adapterType;

	RenderingContextDriverWebGpu *context = (RenderingContextDriverWebGpu *)userdata;
	context->adapter_push_back(
			adapter, device);
}

RenderingContextDriverWebGpu::RenderingContextDriverWebGpu() {
}

RenderingContextDriverWebGpu::~RenderingContextDriverWebGpu() {
	if (instance != nullptr) {
		wgpuInstanceRelease(instance);
	}

	for (WGPUAdapter &adapter : adapters) {
		wgpuAdapterRelease(adapter);
	}
}

Error RenderingContextDriverWebGpu::initialize() {
	instance = wgpuCreateInstance(nullptr);

	WGPURequestAdapterOptions adapter_options = {};
	WGPURequestAdapterCallbackInfo adapter_callback_info = {
		.mode = WGPUCallbackMode_AllowProcessEvents,
		.callback = handle_request_adapter,
		.userdata1 = this,
	};

	// There is no way to request all adapters, so we just get the high and low power ones.

	adapter_options.powerPreference = WGPUPowerPreference::WGPUPowerPreference_HighPerformance;
	wgpuInstanceRequestAdapter(instance,
			&adapter_options,
			adapter_callback_info);

	adapter_options.powerPreference = WGPUPowerPreference::WGPUPowerPreference_LowPower;
	wgpuInstanceRequestAdapter(instance,
		&adapter_options,
		adapter_callback_info);

	// NOTE: Currently unimplemented in wgpu.
	// wgpuInstanceProcessEvents(instance);

	return OK;
}

const RenderingContextDriver::Device &RenderingContextDriverWebGpu::device_get(uint32_t p_device_index) const {
	DEV_ASSERT(p_device_index < adapters.size());
	const RenderingContextDriver::Device &driver_device = driver_devices[p_device_index];
	return driver_device;
}

uint32_t RenderingContextDriverWebGpu::device_get_count() const {
	return adapters.size();
}

bool RenderingContextDriverWebGpu::device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const {
	DEV_ASSERT(p_device_index < adapters.size());
	WGPUAdapter adapter = adapters[p_device_index];
	Surface *surface = (Surface *)p_surface;
	WGPUSurfaceCapabilities caps;
	wgpuSurfaceGetCapabilities(surface->surface, adapter, &caps);
	return caps.formatCount != 0;
}

RenderingDeviceDriver *RenderingContextDriverWebGpu::driver_create() {
	return memnew(RenderingDeviceDriverWebGpu(this));
}

void RenderingContextDriverWebGpu::driver_free(RenderingDeviceDriver *p_driver) {
	memdelete(p_driver);
}

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpu::surface_create(const void *p_platform_data) {
	DEV_ASSERT(false && "Surface creation should not be called on the platform-agnostic version of the driver.");
	return SurfaceID();
}

void RenderingContextDriverWebGpu::surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) {
	Surface *surface = (Surface *)(p_surface);
	surface->width = p_width;
	surface->height = p_height;
	surface->needs_resize = true;
}

void RenderingContextDriverWebGpu::surface_set_vsync_mode(SurfaceID p_surface, DisplayServer::VSyncMode p_vsync_mode) {
	Surface *surface = (Surface *)(p_surface);
	surface->vsync_mode = p_vsync_mode;
	surface->needs_resize = true;
}

DisplayServer::VSyncMode RenderingContextDriverWebGpu::surface_get_vsync_mode(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->vsync_mode;
}

uint32_t RenderingContextDriverWebGpu::surface_get_width(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->width;
}

uint32_t RenderingContextDriverWebGpu::surface_get_height(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->height;
}

void RenderingContextDriverWebGpu::surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) {
	Surface *surface = (Surface *)(p_surface);
	surface->needs_resize = p_needs_resize;
}

bool RenderingContextDriverWebGpu::surface_get_needs_resize(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->needs_resize;
}

void RenderingContextDriverWebGpu::surface_destroy(SurfaceID p_surface) {
	Surface *surface = (Surface *)(p_surface);
	wgpuSurfaceRelease(surface->surface);
	memdelete(surface);
}
bool RenderingContextDriverWebGpu::is_debug_utils_enabled() const {
	//  Although there is a flag to enable validation in WGPU, the spec doesn't support this.
	//  See: https://docs.rs/wgpu/latest/wgpu/struct.InstanceFlags.html#associatedconstant.DEBUG
	return false;
}

WGPUInstance RenderingContextDriverWebGpu::instance_get() const {
	return instance;
}

WGPUAdapter RenderingContextDriverWebGpu::adapter_get(uint32_t p_adapter_index) const {
	DEV_ASSERT(p_adapter_index < adapters.size());
	WGPUAdapter adapter = adapters[p_adapter_index];
	return adapter;
}

void RenderingContextDriverWebGpu::adapter_push_back(WGPUAdapter p_adapter, Device p_device) {
	adapters.push_back(p_adapter);
	driver_devices.push_back(p_device);
}

void RenderingContextDriverWebGpu::Surface::configure(WGPUAdapter p_adapter, WGPUDevice p_device) {
	WGPUSurfaceCapabilities capabilities;
	wgpuSurfaceGetCapabilities(surface, p_adapter, &capabilities);

	// Godot only supports these swapchain formats.
	for (uint32_t i = 0; i < capabilities.formatCount; i++) {
		WGPUTextureFormat format = capabilities.formats[i];
		switch (format) {
			case WGPUTextureFormat_BGRA8Unorm:
				this->format = format;
				this->rd_format = RDD::DATA_FORMAT_B8G8R8A8_UNORM;
				break;
			case WGPUTextureFormat_RGBA8Unorm:
				this->format = format;
				this->rd_format = RDD::DATA_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
				DEV_ASSERT(false && "No supported surface formats.");
		}
	}

	// TODO: Complete full surface config.
	WGPUSurfaceConfiguration surface_config = (WGPUSurfaceConfiguration){
		.device = p_device,
		.format = this->format,
		.usage = WGPUTextureUsage_RenderAttachment,
		.width = this->width,
		.height = this->height,
	};

	wgpuSurfaceConfigure(this->surface, &surface_config);
}

#endif // WEBGPU_ENABLED
