#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct ConvertResult {
  char *wgsl_string;
  uintptr_t wgsl_length;
  char *error_string;
  uintptr_t error_length;
};

extern "C" {

ConvertResult convert_spirv_to_wgsl_alloc(const uint8_t *spv, uintptr_t spv_count);

void convert_result_free(ConvertResult result);

}  // extern "C"
