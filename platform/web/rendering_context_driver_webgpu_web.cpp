#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_web.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuWeb::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc = {};
	canvas_desc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
	canvas_desc.selector.data = wpd->canvas_id;
	canvas_desc.selector.length = strlen(wpd->canvas_id);

	WGPUSurfaceDescriptor surface_desc = {};
	surface_desc.nextInChain = &canvas_desc.chain;

	WGPUSurface wgpu_surface = wgpuInstanceCreateSurface(
			instance_get(),
			&surface_desc);

	ERR_FAIL_COND_V(!wgpu_surface, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->surface = wgpu_surface;
	return SurfaceID(surface);
}

RenderingContextDriverWebGpuWeb::RenderingContextDriverWebGpuWeb() {
	// Does nothing.
}

RenderingContextDriverWebGpuWeb::~RenderingContextDriverWebGpuWeb() {
	// Does nothing.
}

#endif // WEBGPU_ENABLED
