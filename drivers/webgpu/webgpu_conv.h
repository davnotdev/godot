#ifndef WEBGPU_CONV_H
#define WEBGPU_CONV_H

#include "servers/rendering/rendering_device.h"
#include <webgpu.h>

WGPUBufferUsage rd_to_webgpu_buffer_usage(BitField<RDD::BufferUsageBits> p_buffer_usage);

#endif // WEBGPU_CONV_H
