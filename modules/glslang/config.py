def can_build(env, platform):
    # glslang is only needed when Vulkan, Direct3D 12, or WebGpu-based renderers are available,
    # as OpenGL doesn't use glslang.
    return env["vulkan"] or env["d3d12"] or env["webgpu"]


def configure(env):
    pass
