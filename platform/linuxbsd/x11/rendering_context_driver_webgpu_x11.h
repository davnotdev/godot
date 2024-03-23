#ifndef RENDERING_CONTEXT_DRIVER_WEBGPU_X11_H
#define RENDERING_CONTEXT_DRIVER_WEBGPU_X11_H

#ifdef WEBGPU_ENABLED

#include "drivers/webgpu/rendering_context_driver_webgpu.h"

#include <X11/Xlib.h>

class RenderingContextDriverWebGpuX11 : public RenderingContextDriverWebGpu {
protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		::Window window;
		Display *display;
	};

	RenderingContextDriverWebGpuX11();
	~RenderingContextDriverWebGpuX11();
};

#endif // WEBGPU_ENABLED

#endif // RENDERING_CONTEXT_DRIVER_WEBGPU_X11_H
