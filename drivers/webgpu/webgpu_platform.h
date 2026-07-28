#ifndef WEBGPU_PLATFORM_H
#define WEBGPU_PLATFORM_H

#ifdef WEBGPU_BACKEND_WGPU_DESKTOP
#include <webgpu.h>
#include <wgpu.h>
#endif

#if defined(WEBGPU_BACKEND_DAWN_DESKTOP) || defined(WEBGPU_BACKEND_EMDAWN)
#include <webgpu/webgpu.h>
#endif

#endif
