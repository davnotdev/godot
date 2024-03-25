#ifndef RENDERING_CONTEXT_DRIVER_WGPU_H
#define RENDERING_CONTEXT_DRIVER_WGPU_H

#include "servers/rendering/rendering_context_driver.h"

#include <webgpu.h>

class RenderingContextDriverWebGpu : public RenderingContextDriver {
private:
	WGPUInstance instance = nullptr;
	TightLocalVector<WGPUAdapter> adapters;
	TightLocalVector<Device> driver_devices;

public:
	virtual Error initialize() override;
	virtual const RenderingContextDriver::Device &device_get(uint32_t p_device_index) const override;
	virtual uint32_t device_get_count() const override;
	virtual bool device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const override;
	virtual RenderingDeviceDriver *driver_create() override;
	virtual void driver_free(RenderingDeviceDriver *p_driver) override;
	virtual SurfaceID surface_create(const void *p_platform_data) override;
	virtual void surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) override;
	virtual void surface_set_vsync_mode(SurfaceID p_surface, DisplayServer::VSyncMode p_vsync_mode) override;
	virtual DisplayServer::VSyncMode surface_get_vsync_mode(SurfaceID p_surface) const override;
	virtual uint32_t surface_get_width(SurfaceID p_surface) const override;
	virtual uint32_t surface_get_height(SurfaceID p_surface) const override;
	virtual void surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) override;
	virtual bool surface_get_needs_resize(SurfaceID p_surface) const override;
	virtual void surface_destroy(SurfaceID p_surface) override;
	virtual bool is_debug_utils_enabled() const override;

	RenderingContextDriverWebGpu();
	virtual ~RenderingContextDriverWebGpu() override;

	struct Surface {
		WGPUSurface surface;
		uint32_t width = 0;
		uint32_t height = 0;
		DisplayServer::VSyncMode vsync_mode = DisplayServer::VSYNC_ENABLED;
		bool needs_resize = false;
	};

	WGPUInstance instance_get() const;
	WGPUAdapter adapter_get(uint32_t p_adapter_index) const;
	void adapter_push_back(WGPUAdapter p_adapter, Device p_device);
};

#endif // RENDERING_CONTEXT_DRIVER_WGPU_H
