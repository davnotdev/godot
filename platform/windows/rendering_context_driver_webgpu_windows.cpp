#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_windows.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuWindows::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	WGPUSurfaceSourceWindowsHWND winHWND_desc = {};
	winHWND_desc.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
	winHWND_desc.hinstance = wpd->instance;
	winHWND_desc.hwnd = wpd->window;

	WGPUSurfaceDescriptor surface_desc = {};
	surface_desc.nextInChain = (WGPUChainedStruct *)&winHWND_desc;

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

