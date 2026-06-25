#ifndef SPIRV_WEBGPU_TRANSFORM_H
#define SPIRV_WEBGPU_TRANSFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPIRV_WEBGPU_TRANSFORM_CORRECTION_MAP_NULL NULL
#define SPIRV_WEBGPU_TRANSFORM_BOOL uint8_t

#define DEFINE_OPTIONAL(T)                \
	struct {                              \
		SPIRV_WEBGPU_TRANSFORM_BOOL some; \
		T value;                          \
	}

typedef struct _SpvTransformCorrectionMap *SpvTransformCorrectionMap;

typedef DEFINE_OPTIONAL(uint32_t) SpvTransformOptionalU32;

void spirv_webgpu_transform_combimgsampsplitter_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count, SpvTransformCorrectionMap *correction_map);
void spirv_webgpu_transform_combimgsampsplitter_free(uint32_t *out_spv);
void spirv_webgpu_transform_drefsplitter_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count, SpvTransformCorrectionMap *correction_map);
void spirv_webgpu_transform_drefsplitter_free(uint32_t *out_spv);
void spirv_webgpu_transform_immediatespatch_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count, SpvTransformCorrectionMap *correction_map);
void spirv_webgpu_transform_immediatespatch_free(uint32_t *out_spv);
void spirv_webgpu_transform_isnanisinfpatch_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count);
void spirv_webgpu_transform_isnanisinfpatch_free(uint32_t *out_spv);
void spirv_webgpu_transform_storagecubepatch_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count, SpvTransformCorrectionMap *correction_map);
void spirv_webgpu_transform_storagecubepatch_free(uint32_t *out_spv);
void spirv_webgpu_transform_pruneunuseddref_alloc(uint32_t *int_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count);
void spirv_webgpu_transform_pruneunuseddref_free(uint32_t *out_spv);
void spirv_webgpu_transform_splitbindingarray_alloc(uint32_t *in_spv, uint32_t in_count, uint32_t **out_spv, uint32_t *out_count, SpvTransformCorrectionMap *correction_map);
void spirv_webgpu_transform_splitbindingarray_free(uint32_t *out_spv);

void spirv_webgpu_transform_mirrorpatch_alloc(
		uint32_t *in_left_spv, uint32_t in_left_count, SpvTransformCorrectionMap *left_corrections,
		uint32_t *in_right_spv, uint32_t in_right_count, SpvTransformCorrectionMap *right_corrections,
		uint32_t **out_left_spv, uint32_t *out_left_count,
		uint32_t **out_right_spv, uint32_t *out_right_count);
void spirv_webgpu_transform_mirrorpatch_free(uint32_t *out_left_spv, uint32_t *out_right_spv);

typedef enum {
	SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_COMBINED = 0,
	SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_DREF_REGULAR = 1,
	SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_DREF_COMPARISON = 2,
	SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_CONVERT_STORAGE_CUBE = 3,
	SPIRV_WEBGPU_TRANSFORM_CORRECTION_TYPE_SPLIT_BINDING_ARRAY = 4,
} SpvTransformCorrectionType;

// SAFETY: `corrections` invalidates when `correction_map` is written to.
// Returns true if there is `Some` correction type.
SPIRV_WEBGPU_TRANSFORM_BOOL spirv_webgpu_transform_correction_sets_index(
		SpvTransformCorrectionMap correction_map,
		uint32_t set,
		uint32_t binding,
		uint16_t **corrections_ptr,
		uint32_t *correction_count);

SpvTransformOptionalU32 spirv_webgpu_transform_correction_read_immediates_set(
		SpvTransformCorrectionMap correction_map);
void spirv_webgpu_transform_correction_write_immediates_set(
		SpvTransformCorrectionMap *correction_map, uint32_t value);

void spirv_webgpu_transform_correction_map_free(SpvTransformCorrectionMap correction_map);

#ifdef __cplusplus
}
#endif

#endif
