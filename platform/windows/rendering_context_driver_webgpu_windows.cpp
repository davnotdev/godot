#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_windows.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuWindows::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	const WGPUSurfaceDescriptorFromWindowsHWND winHWND_desc =
			(const WGPUSurfaceDescriptorFromWindowsHWND){
				.chain =
						(const WGPUChainedStruct){
								.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
						},
				.hinstance = wpd->instance,
				.hwnd = wpd->window,
			};

	WGPUSurfaceDescriptor surface_desc =
			(WGPUSurfaceDescriptor){
				.nextInChain =
						(const WGPUChainedStruct *)&winHWND_desc
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