#include "webgpu_conv.h"

WGPUBufferUsage rd_to_webgpu_buffer_usage(BitField<RDD::BufferUsageBits> p_buffer_usage) {
	int ret = 0;
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_FROM_BIT) {
		ret |= WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TRANSFER_TO_BIT) {
		ret = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_TEXEL_BIT) {
		ret = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_UNIFORM_BIT) {
		ret = WGPUBufferUsage_Uniform;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_STORAGE_BIT) {
		ret = WGPUBufferUsage_Storage;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDEX_BIT) {
		ret = WGPUBufferUsage_Index;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_VERTEX_BIT) {
		ret = WGPUBufferUsage_Vertex;
	}
	if (p_buffer_usage & RDD::BufferUsageBits::BUFFER_USAGE_INDIRECT_BIT) {
		ret = WGPUBufferUsage_Indirect;
	}
	return (WGPUBufferUsage)ret;
}
