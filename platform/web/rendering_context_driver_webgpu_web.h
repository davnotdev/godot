#ifndef RENDERING_CONTEXT_DRIVER_WEBGPU_WEB_H
#define RENDERING_CONTEXT_DRIVER_WEBGPU_WEB_H

#ifdef WEBGPU_ENABLED

#include "drivers/webgpu/rendering_context_driver_webgpu.h"

class RenderingContextDriverWebGpuWeb : public RenderingContextDriverWebGpu {
protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		const char *canvas_id;
	};

	RenderingContextDriverWebGpuWeb();
	~RenderingContextDriverWebGpuWeb();
};

#endif // WEBGPU_ENABLED

#endif // RENDERING_CONTEXT_DRIVER_WEBGPU_WEB_H
