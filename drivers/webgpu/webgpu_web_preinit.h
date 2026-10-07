#ifndef WEBGPU_WEB_PREINIT_H
#define WEBGPU_WEB_PREINIT_H

#if defined(WEBGPU_ENABLED) && defined(WEBGPU_BACKEND_EMDAWN)

#include "webgpu_platform.h"

// On the web, we need async to request the following.
// To avoid this, we preinitialize these values early.
struct WebGpuWebPreinit {
	WGPUInstance instance = nullptr;
	WGPUAdapter adapter = nullptr;
	WGPUDevice device = nullptr;
};

const WebGpuWebPreinit &webgpu_web_preinit_get();
void webgpu_web_preinit_start(void (*p_callback)(void *p_userdata), void *p_userdata);

#endif // defined(WEBGPU_ENABLED) && defined(WEBGPU_BACKEND_EMDAWN)

#endif // WEBGPU_WEB_PREINIT_H
