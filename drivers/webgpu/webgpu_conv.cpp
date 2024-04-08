#include "webgpu_conv.h"
#include "webgpu.h"

WGPUBufferUsage webgpu_buffer_usage_from_rd(BitField<RDD::BufferUsageBits> p_buffer_usage) {
	uint32_t ret = 0;
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

WGPUTextureFormat webgpu_texture_format_from_rd(RDD::DataFormat p_data_format) {
	WGPUTextureFormat ret = WGPUTextureFormat_Undefined;

	// See https://www.w3.org/TR/webgpu/#enumdef-gputextureformat
	// TODO: The BC, ETC2, and ASTC compressed formats have been left out.
	switch (p_data_format) {
		case RDD::DataFormat::DATA_FORMAT_R8_UNORM:
			ret = WGPUTextureFormat_R8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_SNORM:
			ret = WGPUTextureFormat_R8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_UINT:
			ret = WGPUTextureFormat_R8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8_SINT:
			ret = WGPUTextureFormat_R8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_UINT:
			ret = WGPUTextureFormat_R16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SINT:
			ret = WGPUTextureFormat_R16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16_SFLOAT:
			ret = WGPUTextureFormat_R16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_UINT:
			ret = WGPUTextureFormat_R32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SINT:
			ret = WGPUTextureFormat_R32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32_SFLOAT:
			ret = WGPUTextureFormat_R32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_UNORM:
			ret = WGPUTextureFormat_RG8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_SNORM:
			ret = WGPUTextureFormat_RG8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_UINT:
			ret = WGPUTextureFormat_RG8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8_SINT:
			ret = WGPUTextureFormat_RG8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_UINT:
			ret = WGPUTextureFormat_RG16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SINT:
			ret = WGPUTextureFormat_RG16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16_SFLOAT:
			ret = WGPUTextureFormat_RG16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_UNORM:
			ret = WGPUTextureFormat_RGBA8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SRGB:
			ret = WGPUTextureFormat_RGBA8UnormSrgb;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SNORM:
			ret = WGPUTextureFormat_RGBA8Snorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_UINT:
			ret = WGPUTextureFormat_RGBA8Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R8G8B8A8_SINT:
			ret = WGPUTextureFormat_RGBA8Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_B8G8R8A8_UNORM:
			ret = WGPUTextureFormat_BGRA8Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_B8G8R8A8_SRGB:
			ret = WGPUTextureFormat_BGRA8UnormSrgb;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_UINT:
			ret = WGPUTextureFormat_RG32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SINT:
			ret = WGPUTextureFormat_RG32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32_SFLOAT:
			ret = WGPUTextureFormat_RG32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_UINT:
			ret = WGPUTextureFormat_RGBA16Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SINT:
			ret = WGPUTextureFormat_RGBA16Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R16G16B16A16_SFLOAT:
			ret = WGPUTextureFormat_RGBA16Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_UINT:
			ret = WGPUTextureFormat_RGBA32Uint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SINT:
			ret = WGPUTextureFormat_RGBA32Sint;
			break;
		case RDD::DataFormat::DATA_FORMAT_R32G32B32A32_SFLOAT:
			ret = WGPUTextureFormat_RGBA32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_S8_UINT:
			ret = WGPUTextureFormat_Stencil8;
			break;
		case RDD::DataFormat::DATA_FORMAT_D16_UNORM:
			ret = WGPUTextureFormat_Depth16Unorm;
			break;
		case RDD::DataFormat::DATA_FORMAT_D24_UNORM_S8_UINT:
			ret = WGPUTextureFormat_Depth24PlusStencil8;
			break;
		case RDD::DataFormat::DATA_FORMAT_D32_SFLOAT:
			ret = WGPUTextureFormat_Depth32Float;
			break;
		case RDD::DataFormat::DATA_FORMAT_D32_SFLOAT_S8_UINT:
			ret = WGPUTextureFormat_Depth32FloatStencil8;
			break;
		default:
			break;
	}

	return ret;
}
