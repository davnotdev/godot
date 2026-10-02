#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_windows.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuWindows::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	const WGPUSurfaceSourceWindowsHWND winHWND_desc =
			(const WGPUSurfaceSourceWindowsHWND){
				.chain =
						(const WGPUChainedStruct){
								.sType = WGPUSType_SurfaceSourceWindowsHWND,
						},
				.hinstance = wpd->instance,
				.hwnd = wpd->window,
			};

	WGPUSurfaceDescriptor surface_desc =
			(WGPUSurfaceDescriptor){
				.nextInChain =
						(WGPUChainedStruct *)&winHWND_desc
			};

	WGPUSurface wgpu_surface = wgpuInstanceCreateSurface(
			instance_get(),
			&surface_desc);

	ERR_FAIL_COND_V(!wgpu_surface, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->surface = wgpu_surface;
	return SurfaceID(surface);
}

RenderingContextDriverWebGpuWindows::RenderingContextDriverWebGpuWindows() {
	// Does nothing.
}

RenderingContextDriverWebGpuWindows::~RenderingContextDriverWebGpuWindows() {
	// Does nothing.
}

#endif // WEBGPU_ENABLED

