#ifdef WEBGPU_ENABLED

#include "rendering_context_driver_webgpu_web.h"

RenderingContextDriver::SurfaceID RenderingContextDriverWebGpuWeb::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc =
			(const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector){
				.chain =
						(const WGPUChainedStruct){
								.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector,
						},
				.selector = (WGPUStringView){
						.data = wpd->canvas_id,
						.length = strlen(wpd->canvas_id),
				},
			};

	WGPUSurfaceDescriptor surface_desc =
			(WGPUSurfaceDescriptor){
				.nextInChain = &canvas_desc.chain
			};

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
