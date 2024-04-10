#ifndef RENDERING_CONTEXT_DRIVER_WEBGPU_WINDOWS_H
#define RENDERING_CONTEXT_DRIVER_WEBGPU_WINDOWS_H

#ifdef WEBGPU_ENABLED

#include "drivers/webgpu/rendering_context_driver_webgpu.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class RenderingContextDriverWebGpuWindows : public RenderingContextDriverWebGpu {
protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		HWND window;
		HINSTANCE instance;
	};

	RenderingContextDriverWebGpuWindows();
	~RenderingContextDriverWebGpuWindows();
};

#endif // WEBGPU_ENABLED

#endif // RENDERING_CONTEXT_DRIVER_WEBGPU_WINDOWS_H