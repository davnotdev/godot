#ifndef NAGA_WRAPPER_H
#define NAGA_WRAPPER_H

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <ostream>

struct ConvertResult {
	char *wgsl_string;
	uintptr_t wgsl_length;
	char *error_string;
	uintptr_t error_length;
};

struct PipelineOverride {
	const char *key;
	double value;
};

extern "C" {

ConvertResult convert_spirv_to_wgsl_alloc(const uint8_t *spv, uint32_t spv_count, const PipelineOverride *overrides, uint32_t override_count);
void convert_result_free(ConvertResult result);

} // extern "C"

#endif
