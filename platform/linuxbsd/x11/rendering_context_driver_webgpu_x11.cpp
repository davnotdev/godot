#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_x11.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuX11::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	WGPUSurfaceSourceXlibWindow xlib_desc =
			(const WGPUSurfaceSourceXlibWindow){
				.chain =
						(const WGPUChainedStruct){
								.sType = WGPUSType_SurfaceSourceXlibWindow,
						},
				.display = wpd->display,
				.window = wpd->window,
			};

	WGPUSurfaceDescriptor surface_desc =
			(WGPUSurfaceDescriptor){
				.nextInChain = &xlib_desc.chain
			};

	WGPUSurface wgpu_surface = wgpuInstanceCreateSurface(
			instance_get(),
			&surface_desc);

	ERR_FAIL_COND_V(!wgpu_surface, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->surface = wgpu_surface;
	return SurfaceID(surface);
}

RenderingContextDriverWebGpuX11::RenderingContextDriverWebGpuX11() {
	// Does nothing.
}

RenderingContextDriverWebGpuX11::~RenderingContextDriverWebGpuX11() {
	// Does nothing.
}

#endif // WEBGPU_ENABLED
