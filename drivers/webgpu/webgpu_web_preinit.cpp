#if defined(WEBGPU_ENABLED) && defined(WEBGPU_BACKEND_EMDAWN)

#include "webgpu_web_preinit.h"

#include "rendering_device_driver_webgpu.h"

#include "core/os/memory.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"

static WebGpuWebPreinit preinit;

struct WebGpuWebPreinitRequest {
	void (*callback)(void *) = nullptr;
	void *userdata = nullptr;
	RenderingDeviceDriverWebGpu::DeviceRequirements requirements;
};

const WebGpuWebPreinit &webgpu_web_preinit_get() {
	return preinit;
}

static void _preinit_finish(WebGpuWebPreinitRequest *p_request) {
	void (*callback)(void *) = p_request->callback;
	void *userdata = p_request->userdata;
	memdelete(p_request);
	callback(userdata);
}

static void _preinit_handle_request_device(WGPURequestDeviceStatus p_status, WGPUDevice p_device, WGPUStringView p_message, void *p_userdata, void *) {
	WebGpuWebPreinitRequest *request = (WebGpuWebPreinitRequest *)p_userdata;
	if (p_status == WGPURequestDeviceStatus_Success && p_device) {
		preinit.device = p_device;
	} else {
		print_line("[WEBGPU] Failed to request a device:", String::utf8(p_message.data, p_message.length));
	}
	_preinit_finish(request);
}

static void _preinit_handle_request_adapter(WGPURequestAdapterStatus p_status, WGPUAdapter p_adapter, WGPUStringView p_message, void *p_userdata, void *) {
	WebGpuWebPreinitRequest *request = (WebGpuWebPreinitRequest *)p_userdata;
	if (p_status != WGPURequestAdapterStatus_Success || !p_adapter) {
		print_line("[WEBGPU] Failed to request an adapter:", String::utf8(p_message.data, p_message.length));
		_preinit_finish(request);
		return;
	}
	preinit.adapter = p_adapter;

	RenderingDeviceDriverWebGpu::device_requirements_get(p_adapter, request->requirements);
	WGPURequestDeviceCallbackInfo device_callback_info = {};
	device_callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
	device_callback_info.callback = _preinit_handle_request_device;
	device_callback_info.userdata1 = request;
	wgpuAdapterRequestDevice(p_adapter, &request->requirements.descriptor, device_callback_info);
}

void webgpu_web_preinit_start(void (*p_callback)(void *p_userdata), void *p_userdata) {
	WebGpuWebPreinitRequest *request = memnew(WebGpuWebPreinitRequest);
	request->callback = p_callback;
	request->userdata = p_userdata;

	// NOTE: `TimedWaitAny` would require ASYNCIFY or JSPI.
	WGPUInstanceDescriptor instance_descriptor = WGPU_INSTANCE_DESCRIPTOR_INIT;
	preinit.instance = wgpuCreateInstance(&instance_descriptor);
	if (!preinit.instance) {
		print_line("[WEBGPU] Failed to create an instance.");
		_preinit_finish(request);
		return;
	}

	// Browsers generally return the same adapter regardless of the power preference.
	WGPURequestAdapterOptions adapter_options = {};
	adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;
	WGPURequestAdapterCallbackInfo adapter_callback_info = {};
	adapter_callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
	adapter_callback_info.callback = _preinit_handle_request_adapter;
	adapter_callback_info.userdata1 = request;
	wgpuInstanceRequestAdapter(preinit.instance, &adapter_options, adapter_callback_info);
}

#endif // defined(WEBGPU_ENABLED) && defined(WEBGPU_BACKEND_EMDAWN)
