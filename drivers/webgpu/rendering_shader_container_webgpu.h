#pragma once

#include "drivers/webgpu/webgpu_common.h"
#include "servers/rendering/rendering_shader_container.h"

class RenderingShaderContainerWebGpu : public RenderingShaderContainer {
	GDSOFTCLASS(RenderingShaderContainerWebGpu, RenderingShaderContainer);

public:
	static const uint32_t FORMAT_VERSION;

	struct UniformData {
		WebGpuBindingHint base_hint;
		uint32_t image_format = 0; // RDC::DataFormat
		uint32_t image_access = 0; // RDC::ShaderUniform::ImageAccess
		uint32_t texture_image_type = 0; // RDC::TextureType
		uint32_t texture_sample_type = 0; // RDC::ShaderUniform::TextureSampleType
		uint32_t texture_is_multisample = 0;
		Vector<uint32_t> corrections;
		Vector<WebGpuBindingHint> correction_hints; // size == corrections.size()
	};

	Vector<UniformData> webgpu_uniform_data;

protected:
	virtual uint32_t _format() const override;
	virtual uint32_t _format_version() const override;
	virtual bool _set_code_from_spirv(const ReflectShader &p_shader) override;

	virtual void _set_from_shader_reflection_post(const ReflectShader &p_shader) override;

	virtual uint32_t _to_bytes_reflection_binding_uniform_extra_data(uint8_t *p_bytes, uint32_t p_index) const override;
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data_start(const uint8_t *p_bytes) override;
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data(const uint8_t *p_bytes, uint32_t p_index) override;

public:
	RenderingShaderContainerWebGpu();
};

class RenderingShaderContainerFormatWebGpu : public RenderingShaderContainerFormat {
public:
	virtual Ref<RenderingShaderContainer> create_container() const override;
	virtual ShaderLanguageVersion get_shader_language_version() const override;
	virtual ShaderSpirvVersion get_shader_spirv_version() const override;
	RenderingShaderContainerFormatWebGpu();
	virtual ~RenderingShaderContainerFormatWebGpu();
};
