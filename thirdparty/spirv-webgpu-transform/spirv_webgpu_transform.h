#ifndef SPIRV_WEBGPU_TRANSFORM_H
#define SPIRV_WEBGPU_TRANSFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TransformCorrectionMap;

#define SPIRV_WEBGPU_TRANFORM_CORRECTION_MAP_NULL NULL

void spirv_webgpu_transform_combimgsampsplitter_alloc(uint32_t* in_spv, uint32_t in_count, uint32_t** out_spv, uint32_t* out_count, TransformCorrectionMap* correction_map);
void spirv_webgpu_transform_combimgsampsplitter_free(uint32_t* out_spv);
void spirv_webgpu_transform_drefsplitter_alloc(uint32_t* in_spv, uint32_t in_count, uint32_t** out_spv, uint32_t* out_count, TransformCorrectionMap* correction_map);
void spirv_webgpu_transform_drefsplitter_free(uint32_t* out_spv);

typedef enum {
    SPIRV_WEBGPU_TRANSFORM_CORRECTION_STATUS_NONE = 0,
    SPIRV_WEBGPU_TRANSFORM_CORRECTION_STATUS_SOME = 1,
} TransformCorrectionStatus;

typedef enum {
    SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_COMBINED = 0,
    SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_DREF_REGULAR = 1,
    SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_DREF_COMPARISON = 2,
} TransformCorrectionType;

// SAFETY: `corrections` invalidates when `correction_map` is written to.
TransformCorrectionStatus spirv_webgpu_transform_correction_map_index(
    TransformCorrectionMap correction_map, 
    uint32_t set, 
    uint32_t binding, 
    uint16_t** corrections_ptr, 
    uint32_t* correction_count);

void spirv_webgpu_transform_correction_map_free(TransformCorrectionMap correction_map);

#ifdef __cplusplus
}
#endif

#endif
