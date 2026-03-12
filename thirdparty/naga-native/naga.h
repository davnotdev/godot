#ifndef NAGA_FFI_H
#define NAGA_FFI_H

#include <stdint.h>
#include <stdlib.h>

#define NAGA_FFI_VERSION 28
#define NAGA_NULLABLE
#define NAGA_UNIMPLEMENTED

#define DEFINE_OPTIONAL(T) \
	struct {               \
		NagaBool some;     \
		T value;           \
	}
#define DEFINE_HANDLE_INDEX(T) size_t

#ifdef __cplusplus
extern "C" {
#endif

struct NagaEmpty {
	uint8_t _phantom;
};
typedef uint8_t NagaBool;

// TODO: desctructors to free everything
// TODO: validator errors
// TODO: implement *::ReflectionInfo

typedef struct NagaSpan {
	uint32_t start;
	uint32_t end;
} NagaSpan;

#define NAGA_FLAGS_EMPTY(T) (T)0
#define NAGA_FLAGS_ALL(T) (T)((T) ~(T)0)

// --- naga::ir ---

typedef uint8_t NagaBytes;

typedef struct NagaType NagaType;
typedef struct NagaOverride NagaOverride;

typedef enum NagaScalarKind {
	NagaScalarKind_Sint = 0,
	NagaScalarKind_Uint = 1,
	NagaScalarKind_Float = 2,
	NagaScalarKind_Bool = 3,
	NagaScalarKind_AbstractInt = 4,
	NagaScalarKind_AbstractFloat = 5,
} NagaScalarKind;

typedef struct NagaScalar {
	NagaScalarKind kind;
	NagaBytes width;
} NagaScalar;

typedef enum NagaVectorSize {
	NagaVectorSize_Bi = 2,
	NagaVectorSize_Tri = 3,
	NagaVectorSize_Quad = 4,
} NagaVectorSize;

typedef enum NagaArraySizeTag {
	NagaArraySizeTag_Constant,
	NagaArraySizeTag_Pending,
	NagaArraySizeTag_Dynamic,
} NagaArraySizeTag;

typedef struct NagaArraySize {
	NagaArraySizeTag tag;
	union {
		uint32_t constant;
		DEFINE_HANDLE_INDEX(NagaOverride)
		pending;
	} data;
} NagaArraySize;

typedef enum NagaStorageFormat {
	NagaStorageFormat_R8Unorm,
	NagaStorageFormat_R8Snorm,
	NagaStorageFormat_R8Uint,
	NagaStorageFormat_R8Sint,
	NagaStorageFormat_R16Uint,
	NagaStorageFormat_R16Sint,
	NagaStorageFormat_R16Float,
	NagaStorageFormat_Rg8Unorm,
	NagaStorageFormat_Rg8Snorm,
	NagaStorageFormat_Rg8Uint,
	NagaStorageFormat_Rg8Sint,
	NagaStorageFormat_R32Uint,
	NagaStorageFormat_R32Sint,
	NagaStorageFormat_R32Float,
	NagaStorageFormat_Rg16Uint,
	NagaStorageFormat_Rg16Sint,
	NagaStorageFormat_Rg16Float,
	NagaStorageFormat_Rgba8Unorm,
	NagaStorageFormat_Rgba8Snorm,
	NagaStorageFormat_Rgba8Uint,
	NagaStorageFormat_Rgba8Sint,
	NagaStorageFormat_Bgra8Unorm,
	NagaStorageFormat_Rgb10a2Uint,
	NagaStorageFormat_Rgb10a2Unorm,
	NagaStorageFormat_Rg11b10Ufloat,
	NagaStorageFormat_R64Uint,
	NagaStorageFormat_Rg32Uint,
	NagaStorageFormat_Rg32Sint,
	NagaStorageFormat_Rg32Float,
	NagaStorageFormat_Rgba16Uint,
	NagaStorageFormat_Rgba16Sint,
	NagaStorageFormat_Rgba16Float,
	NagaStorageFormat_Rgba32Uint,
	NagaStorageFormat_Rgba32Sint,
	NagaStorageFormat_Rgba32Float,
	NagaStorageFormat_R16Unorm,
	NagaStorageFormat_R16Snorm,
	NagaStorageFormat_Rg16Unorm,
	NagaStorageFormat_Rg16Snorm,
	NagaStorageFormat_Rgba16Unorm,
	NagaStorageFormat_Rgba16Snorm,
} NagaStorageFormat;

typedef uint32_t NagaStorageAccessFlags;
typedef enum NagaStorageAccess {
	NagaStorageAccess_LOAD = 0x1,
	NagaStorageAccess_STORE = 0x2,
	NagaStorageAccess_ATOMIC = 0x4,
} NagaStorageAccess;

typedef enum NagaRelationalFunction {
	NagaRelationalFunction_All,
	NagaRelationalFunction_Any,
	NagaRelationalFunction_IsNan,
	NagaRelationalFunction_IsInf,
} NagaRelationalFunction;

typedef enum NagaImageClassTag {
	NagaImageClassTag_Sampled,
	NagaImageClassTag_Depth,
	NagaImageClassTag_External,
	NagaImageClassTag_Storage,
} NagaImageClassTag;

typedef struct NagaImageClass {
	NagaImageClassTag tag;
	union {
		struct {
			NagaScalarKind kind;
			NagaBool multi;
		} sampled;
		struct {
			NagaBool multi;
		} depth;
		struct {
			NagaStorageFormat format;
			NagaStorageAccessFlags access;
		} storage;
	} data;
} NagaImageClass;

typedef enum NagaAddressSpaceTag {
	NagaAddressSpaceTag_Function,
	NagaAddressSpaceTag_Private,
	NagaAddressSpaceTag_WorkGroup,
	NagaAddressSpaceTag_Uniform,
	NagaAddressSpaceTag_Storage,
	NagaAddressSpaceTag_Handle,
	NagaAddressSpaceTag_Immediate,
	NagaAddressSpaceTag_TaskPayload,
} NagaAddressSpaceTag;

typedef struct NagaAddressSpace {
	NagaAddressSpaceTag tag;
	union {
		struct {
			NagaStorageAccessFlags access;
		} storage;
	} data;
} NagaAddressSpace;

typedef enum NagaMathFunction {
	NagaMathFunction_Abs,
	NagaMathFunction_Min,
	NagaMathFunction_Max,
	NagaMathFunction_Clamp,
	NagaMathFunction_Saturate,
	NagaMathFunction_Cos,
	NagaMathFunction_Cosh,
	NagaMathFunction_Sin,
	NagaMathFunction_Sinh,
	NagaMathFunction_Tan,
	NagaMathFunction_Tanh,
	NagaMathFunction_Acos,
	NagaMathFunction_Asin,
	NagaMathFunction_Atan,
	NagaMathFunction_Atan2,
	NagaMathFunction_Asinh,
	NagaMathFunction_Acosh,
	NagaMathFunction_Atanh,
	NagaMathFunction_Radians,
	NagaMathFunction_Degrees,
	NagaMathFunction_Ceil,
	NagaMathFunction_Floor,
	NagaMathFunction_Round,
	NagaMathFunction_Fract,
	NagaMathFunction_Trunc,
	NagaMathFunction_Modf,
	NagaMathFunction_Frexp,
	NagaMathFunction_Ldexp,
	NagaMathFunction_Exp,
	NagaMathFunction_Exp2,
	NagaMathFunction_Log,
	NagaMathFunction_Log2,
	NagaMathFunction_Pow,
	NagaMathFunction_Dot,
	NagaMathFunction_Dot4I8Packed,
	NagaMathFunction_Dot4U8Packed,
	NagaMathFunction_Outer,
	NagaMathFunction_Cross,
	NagaMathFunction_Distance,
	NagaMathFunction_Length,
	NagaMathFunction_Normalize,
	NagaMathFunction_FaceForward,
	NagaMathFunction_Reflect,
	NagaMathFunction_Refract,
	NagaMathFunction_Sign,
	NagaMathFunction_Fma,
	NagaMathFunction_Mix,
	NagaMathFunction_Step,
	NagaMathFunction_SmoothStep,
	NagaMathFunction_Sqrt,
	NagaMathFunction_InverseSqrt,
	NagaMathFunction_Inverse,
	NagaMathFunction_Transpose,
	NagaMathFunction_Determinant,
	NagaMathFunction_QuantizeToF16,
	NagaMathFunction_CountTrailingZeros,
	NagaMathFunction_CountLeadingZeros,
	NagaMathFunction_CountOneBits,
	NagaMathFunction_ReverseBits,
	NagaMathFunction_ExtractBits,
	NagaMathFunction_InsertBits,
	NagaMathFunction_FirstTrailingBit,
	NagaMathFunction_FirstLeadingBit,
	NagaMathFunction_Pack4x8snorm,
	NagaMathFunction_Pack4x8unorm,
	NagaMathFunction_Pack2x16snorm,
	NagaMathFunction_Pack2x16unorm,
	NagaMathFunction_Pack2x16float,
	NagaMathFunction_Pack4xI8,
	NagaMathFunction_Pack4xU8,
	NagaMathFunction_Pack4xI8Clamp,
	NagaMathFunction_Pack4xU8Clamp,
	NagaMathFunction_Unpack4x8snorm,
	NagaMathFunction_Unpack4x8unorm,
	NagaMathFunction_Unpack2x16snorm,
	NagaMathFunction_Unpack2x16unorm,
	NagaMathFunction_Unpack2x16float,
	NagaMathFunction_Unpack4xI8,
	NagaMathFunction_Unpack4xU8,
} NagaMathFunction;

typedef enum NagaInterpolation {
	NagaInterpolation_Perspective,
	NagaInterpolation_Linear,
	NagaInterpolation_Flat,
} NagaInterpolation;

typedef enum NagaSampling {
	NagaSampling_Center,
	NagaSampling_Centroid,
	NagaSampling_Sample,
	NagaSampling_First,
	NagaSampling_Either,
} NagaSampling;

typedef enum NagaBuiltInTag {
	NagaBuiltInTag_Position,
	NagaBuiltInTag_ViewIndex,
	NagaBuiltInTag_BaseInstance,
	NagaBuiltInTag_BaseVertex,
	NagaBuiltInTag_ClipDistance,
	NagaBuiltInTag_CullDistance,
	NagaBuiltInTag_InstanceIndex,
	NagaBuiltInTag_PointSize,
	NagaBuiltInTag_VertexIndex,
	NagaBuiltInTag_DrawID,
	NagaBuiltInTag_FragDepth,
	NagaBuiltInTag_PointCoord,
	NagaBuiltInTag_FrontFacing,
	NagaBuiltInTag_PrimitiveIndex,
	NagaBuiltInTag_Barycentric,
	NagaBuiltInTag_SampleIndex,
	NagaBuiltInTag_SampleMask,
	NagaBuiltInTag_GlobalInvocationId,
	NagaBuiltInTag_LocalInvocationId,
	NagaBuiltInTag_LocalInvocationIndex,
	NagaBuiltInTag_WorkGroupId,
	NagaBuiltInTag_WorkGroupSize,
	NagaBuiltInTag_NumWorkGroups,
	NagaBuiltInTag_NumSubgroups,
	NagaBuiltInTag_SubgroupId,
	NagaBuiltInTag_SubgroupSize,
	NagaBuiltInTag_SubgroupInvocationId,
	NagaBuiltInTag_MeshTaskSize,
	NagaBuiltInTag_CullPrimitive,
	NagaBuiltInTag_PointIndex,
	NagaBuiltInTag_LineIndices,
	NagaBuiltInTag_TriangleIndices,
	NagaBuiltInTag_VertexCount,
	NagaBuiltInTag_Vertices,
	NagaBuiltInTag_PrimitiveCount,
	NagaBuiltInTag_Primitives,
} NagaBuiltInTag;

typedef struct NagaBuiltIn {
	NagaBuiltInTag tag;
	union {
		struct {
			NagaBool invariant;
		} position;
	} data;
} NagaBuiltIn;

typedef enum NagaBindingTag {
	NagaBindingTag_BuiltIn,
	NagaBindingTag_Location,
} NagaBindingTag;

typedef struct NagaBinding {
	NagaBindingTag tag;
	union {
		NagaBuiltIn built_in;
		struct {
			uint32_t location;
			NagaInterpolation interpolation;
			NagaSampling sampling;
			uint32_t blend_src;
			NagaBool per_primitive;
		} location;
	} data;
} NagaBinding;

typedef struct NagaStructMember {
	char *NAGA_NULLABLE name;
	DEFINE_HANDLE_INDEX(NagaType)
	ty;
	DEFINE_OPTIONAL(NagaBinding)
	binding;
	uint32_t offset;
} NagaStructMember;

typedef enum NagaImageDimension {
	NagaImageDimension_D1,
	NagaImageDimension_D2,
	NagaImageDimension_D3,
	NagaImageDimension_Cube,
} NagaImageDimension;

typedef enum NagaTypeInnerTag {
	NagaTypeInnerTag_Scalar,
	NagaTypeInnerTag_Vector,
	NagaTypeInnerTag_Matrix,
	NagaTypeInnerTag_Atomic,
	NagaTypeInnerTag_Pointer,
	NagaTypeInnerTag_ValuePointer,
	NagaTypeInnerTag_Array,
	NagaTypeInnerTag_Struct,
	NagaTypeInnerTag_Image,
	NagaTypeInnerTag_Sampler,
	NagaTypeInnerTag_AccelerationStructure,
	NagaTypeInnerTag_RayQuery,
	NagaTypeInnerTag_BindingArray,
} NagaTypeInnerTag;

typedef struct NagaTypeInner {
	NagaTypeInnerTag tag;
	union {
		NagaScalar scalar;
		struct {
			NagaVectorSize size;
			NagaScalar scalar;
		} vector;
		struct {
			NagaVectorSize columns;
			NagaVectorSize rows;
			NagaScalar scalar;
		} matrix;
		NagaScalar atomic;
		struct {
			DEFINE_HANDLE_INDEX(NagaType)
			base;
			NagaAddressSpace space;
		} pointer;
		struct {
			NagaVectorSize size;
			NagaScalar scalar;
			NagaAddressSpace space;
		} value_pointer;
		struct {
			DEFINE_HANDLE_INDEX(NagaType)
			base;
			NagaArraySize size;
			uint32_t stride;
		} array;
		struct {
			NagaStructMember *members;
			size_t members_len;
			uint32_t span;
		} struct_;
		struct {
			NagaImageDimension dim;
			NagaBool arrayed;
			NagaImageClass class_;
		} image;
		struct {
			NagaBool comparison;
		} sampler;
		struct {
			NagaBool vertex_return;
		} acceleration_structure;
		struct {
			NagaBool vertex_return;
		} ray_query;
		struct {
			DEFINE_HANDLE_INDEX(NagaType)
			base;
			NagaArraySize size;
		} binding_array;
	} data;
} NagaTypeInner;

typedef struct NagaType {
	char *NAGA_NULLABLE name;
	NagaTypeInner inner;
} NagaType;

typedef struct NagaConstant {
	char *NAGA_NULLABLE name;
	DEFINE_HANDLE_INDEX(NagaType)
	ty;
	struct NagaEmpty *NAGA_UNIMPLEMENTED init;
} NagaConstant;

typedef struct NagaOverride {
	char *NAGA_NULLABLE name;
	DEFINE_OPTIONAL(uint16_t)
	id;
	DEFINE_HANDLE_INDEX(NagaType)
	ty;
	struct NagaEmpty *NAGA_UNIMPLEMENTED init;
} NagaOverride;

typedef struct NagaResourceBinding {
	uint32_t group;
	uint32_t binding;
} NagaResourceBinding;

typedef struct NagaGlobalVariable {
	char *NAGA_NULLABLE name;
	NagaAddressSpace space;
	DEFINE_OPTIONAL(NagaResourceBinding)
	binding;
	DEFINE_HANDLE_INDEX(NagaType)
	ty;
	struct NagaEmpty *NAGA_UNIMPLEMENTED init;
} NagaGlobalVariable;

typedef enum NagaConservativeDepth {
	NagaConservativeDepth_GreaterEqual,
	NagaConservativeDepth_LessEqual,
	NagaConservativeDepth_Unchanged,
} NagaConservativeDepth;

typedef enum NagaEarlyDepthTestTag {
	NagaEarlyDepthTestTag_Force,
	NagaEarlyDepthTestTag_Allow,
} NagaEarlyDepthTestTag;

typedef struct NagaEarlyDepthTest {
	NagaEarlyDepthTestTag tag;
	union {
		struct {
			NagaConservativeDepth conservative;
		} allow;
	} data;
} NagaEarlyDepthTest;

typedef enum NagaShaderStage {
	NagaShaderStage_Vertex,
	NagaShaderStage_Task,
	NagaShaderStage_Mesh,
	NagaShaderStage_Fragment,
	NagaShaderStage_Compute,
} NagaShaderStage;

typedef struct NagaEntryPoint {
	char *name;
	NagaShaderStage stage;
	DEFINE_OPTIONAL(NagaEarlyDepthTest)
	early_depth_test;
	uint32_t workgroup_size[3];
	struct NagaEmpty *NAGA_UNIMPLEMENTED workgroup_size_overrides[3];
	struct NagaEmpty *NAGA_UNIMPLEMENTED function;
	struct NagaEmpty *NAGA_UNIMPLEMENTED mesh_info;
	struct NagaEmpty *NAGA_UNIMPLEMENTED task_payload;
} NagaEntryPoint;

typedef uint32_t NagaModuleFillFlags;
typedef enum NagaModuleFill {
	NagaModuleFill_Types = 0x1,
	NagaModuleFill_Constants = 0x2,
	NagaModuleFill_Overrides = 0x4,
	NagaModuleFill_GlobalVariables = 0x8,
	NagaModuleFill_GlobalExpressions = 0x10,
	NagaModuleFill_Functions = 0x20,
	NagaModuleFill_EntryPoints = 0x40,
	NagaModuleFill_DiagnosticFilters = 0x80,
} NagaModuleFill;

typedef struct NagaModule {
	void *_inner_module;

	NagaType *types;
	size_t types_len;
	struct NagaEmpty *NAGA_UNIMPLEMENTED special_types;
	NagaConstant *constants;
	size_t constants_len;
	NagaOverride *overrides;
	size_t overrides_len;
	NagaGlobalVariable *global_variables;
	size_t global_variables_len;
	struct NagaEmpty *NAGA_UNIMPLEMENTED global_expressions;
	size_t global_expressions_len;
	struct NagaEmpty *NAGA_UNIMPLEMENTED functions;
	size_t functions_len;
	NagaEntryPoint *entry_points;
	size_t entry_points_len;
	struct NagaEmpty *NAGA_UNIMPLEMENTED diagnostic_filters;
	size_t diagnostic_filters_len;
	struct NagaEmpty *NAGA_UNIMPLEMENTED diagnostic_filter_leaf;
	struct NagaEmpty *NAGA_UNIMPLEMENTED doc_comments;
} NagaModule;

typedef enum NagaModuleInfoFillFlags {
	NagaModuleInfoFillFlags_Unimplemented = UINT32_MAX
} NagaModuleInfoFillFlags;

typedef struct NagaModuleInfo {
	void *_inner_module_info;
} NagaModuleInfo;

// --- naga::proc ---

typedef enum NagaConstantEvaluatorErrorTag {
	NagaConstantEvaluatorErrorTag_FunctionArg,
	NagaConstantEvaluatorErrorTag_GlobalVariable,
	NagaConstantEvaluatorErrorTag_LocalVariable,
	NagaConstantEvaluatorErrorTag_InvalidArrayLengthArg,
	NagaConstantEvaluatorErrorTag_ArrayLengthDynamic,
	NagaConstantEvaluatorErrorTag_ArrayLengthOverridden,
	NagaConstantEvaluatorErrorTag_Call,
	NagaConstantEvaluatorErrorTag_WorkGroupUniformLoadResult,
	NagaConstantEvaluatorErrorTag_Atomic,
	NagaConstantEvaluatorErrorTag_Derivative,
	NagaConstantEvaluatorErrorTag_Load,
	NagaConstantEvaluatorErrorTag_ImageExpression,
	NagaConstantEvaluatorErrorTag_RayQueryExpression,
	NagaConstantEvaluatorErrorTag_SubgroupExpression,
	NagaConstantEvaluatorErrorTag_InvalidAccessBase,
	NagaConstantEvaluatorErrorTag_InvalidAccessIndex,
	NagaConstantEvaluatorErrorTag_InvalidAccessIndexTy,
	NagaConstantEvaluatorErrorTag_ArrayLength,
	NagaConstantEvaluatorErrorTag_InvalidCastArg,
	NagaConstantEvaluatorErrorTag_InvalidUnaryOpArg,
	NagaConstantEvaluatorErrorTag_InvalidBinaryOpArgs,
	NagaConstantEvaluatorErrorTag_InvalidMathArg,
	NagaConstantEvaluatorErrorTag_InvalidMathArgCount,
	NagaConstantEvaluatorErrorTag_InvalidRelationalArg,
	NagaConstantEvaluatorErrorTag_InvalidClamp,
	NagaConstantEvaluatorErrorTag_InvalidVectorComposeLength,
	NagaConstantEvaluatorErrorTag_InvalidVectorComposeComponent,
	NagaConstantEvaluatorErrorTag_SplatScalarOnly,
	NagaConstantEvaluatorErrorTag_SwizzleVectorOnly,
	NagaConstantEvaluatorErrorTag_SwizzleOutOfBounds,
	NagaConstantEvaluatorErrorTag_TypeNotConstructible,
	NagaConstantEvaluatorErrorTag_SubexpressionsAreNotConstant,
	NagaConstantEvaluatorErrorTag_NotImplemented,
	NagaConstantEvaluatorErrorTag_Overflow,
	NagaConstantEvaluatorErrorTag_AutomaticConversionLossy,
	NagaConstantEvaluatorErrorTag_DivisionByZero,
	NagaConstantEvaluatorErrorTag_RemainderByZero,
	NagaConstantEvaluatorErrorTag_ShiftedMoreThan32Bits,
	NagaConstantEvaluatorErrorTag_Literal,
	NagaConstantEvaluatorErrorTag_Override,
	NagaConstantEvaluatorErrorTag_RuntimeExpr,
	NagaConstantEvaluatorErrorTag_OverrideExpr,
	NagaConstantEvaluatorErrorTag_SelectScalarConditionNotABool,
	NagaConstantEvaluatorErrorTag_SelectVecRejectAcceptSizeMismatch,
	NagaConstantEvaluatorErrorTag_SelectConditionNotAVecBool,
	NagaConstantEvaluatorErrorTag_SelectConditionVecSizeMismatch,
	NagaConstantEvaluatorErrorTag_SelectAcceptRejectTypeMismatch,
} NagaConstantEvaluatorErrorTag;

typedef struct NagaConstantEvaluatorError {
	NagaConstantEvaluatorErrorTag tag;
	union {
		struct {
			char *from;
			char *to;
		} invalid_cast_arg;
		struct {
			NagaMathFunction function;
			size_t expected;
			size_t actual;
		} invalid_math_arg_count;
		struct NagaEmpty *NAGA_UNIMPLEMENTED invalid_relational_arg;
		struct {
			size_t expected;
			size_t actual;
		} invalid_vector_compose_length;
		char *not_implemented;
		char *overflow;
		struct {
			char *value;
			char *to_type;
		} automatic_conversion_lossy;
		struct NagaEmpty *NAGA_UNIMPLEMENTED literal;
		struct {
			NagaVectorSize reject;
			NagaVectorSize accept;
		} select_vec_reject_accept_size_mismatch;
	} data;
} NagaConstantEvaluatorError;

typedef enum NagaBoundsCheckPolicy {
	NagaBoundsCheckPolicy_Restrict,
	NagaBoundsCheckPolicy_ReadZeroSkipWrite,
	NagaBoundsCheckPolicy_Unchecked,
} NagaBoundsCheckPolicy;

typedef struct NagaBoundsCheckPolicies {
	NagaBoundsCheckPolicy index;
	NagaBoundsCheckPolicy buffer;
	NagaBoundsCheckPolicy image_load;
	NagaBoundsCheckPolicy binding_array;
} NagaBoundsCheckPolicies;

typedef enum NagaResolveArraySizeError {
	NagaResolveArraySizeError_ExpectedPositiveArrayLength,
	NagaResolveArraySizeError_NonConstArrayLength,
} NagaResolveArraySizeError;

// --- naga::valid ---

typedef struct NagaValidator {
	void *_inner_validator;
} NagaValidator;

typedef uint64_t NagaCapabilitiesFlags;
typedef enum NagaCapabilities {
	NagaCapabilities_IMMEDIATES = 0x1,
	NagaCapabilities_FLOAT64 = 0x2,
	NagaCapabilities_PRIMITIVE_INDEX = 0x4,
	NagaCapabilities_TEXTURE_AND_SAMPLER_BINDING_ARRAY = 0x8,
	NagaCapabilities_BUFFER_BINDING_ARRAY = 0x10,
	NagaCapabilities_STORAGE_TEXTURE_BINDING_ARRAY = 0x20,
	NagaCapabilities_STORAGE_BUFFER_BINDING_ARRAY = 0x40,
	NagaCapabilities_CLIP_DISTANCE = 0x80,
	NagaCapabilities_CULL_DISTANCE = 0x100,
	NagaCapabilities_STORAGE_TEXTURE_16BIT_NORM_FORMATS = 0x200,
	NagaCapabilities_MULTIVIEW = 0x400,
	NagaCapabilities_EARLY_DEPTH_TEST = 0x800,
	NagaCapabilities_MULTISAMPLED_SHADING = 0x1000,
	NagaCapabilities_RAY_QUERY = 0x2000,
	NagaCapabilities_DUAL_SOURCE_BLENDING = 0x4000,
	NagaCapabilities_CUBE_ARRAY_TEXTURES = 0x8000,
	NagaCapabilities_SHADER_INT64 = 0x10000,
	NagaCapabilities_SUBGROUP = 0x20000,
	NagaCapabilities_SUBGROUP_BARRIER = 0x40000,
	NagaCapabilities_SUBGROUP_VERTEX_STAGE = 0x80000,
	NagaCapabilities_SHADER_INT64_ATOMIC_MIN_MAX = 0x100000,
	NagaCapabilities_SHADER_INT64_ATOMIC_ALL_OPS = 0x200000,
	NagaCapabilities_SHADER_FLOAT32_ATOMIC = 0x400000,
	NagaCapabilities_TEXTURE_ATOMIC = 0x800000,
	NagaCapabilities_TEXTURE_INT64_ATOMIC = 0x1000000,
	NagaCapabilities_RAY_HIT_VERTEX_POSITION = 0x2000000,
	NagaCapabilities_SHADER_FLOAT16 = 0x4000000,
	NagaCapabilities_TEXTURE_EXTERNAL = 0x8000000,
	NagaCapabilities_SHADER_FLOAT16_IN_FLOAT32 = 0x10000000,
	NagaCapabilities_SHADER_BARYCENTRICS = 0x20000000,
	NagaCapabilities_MESH_SHADER = 0x40000000,
	NagaCapabilities_MESH_SHADER_POINT_TOPOLOGY = 0x80000000,
	NagaCapabilities_TEXTURE_AND_SAMPLER_BINDING_ARRAY_NON_UNIFORM_INDEXING = 0x100000000,
	NagaCapabilities_BUFFER_BINDING_ARRAY_NON_UNIFORM_INDEXING = 0x200000000,
	NagaCapabilities_STORAGE_TEXTURE_BINDING_ARRAY_NON_UNIFORM_INDEXING = 0x400000000,
	NagaCapabilities_STORAGE_BUFFER_BINDING_ARRAY_NON_UNIFORM_INDEXING = 0x800000000,
} NagaCapabilities;

// On the topic of `FlagsFlags1, I'd pick consistency over grammar any day.
typedef uint8_t NagaValidationFlagsFlags;
typedef enum NagaValidationFlags {
	NagaValidationFlags_EXPRESSIONS = 0x1,
	NagaValidationFlags_BLOCKS = 0x2,
	NagaValidationFlags_CONTROL_FLOW_UNIFORMITY = 0x4,
	NagaValidationFlags_STRUCT_LAYOUTS = 0x8,
	NagaValidationFlags_CONSTANTS = 0x10,
	NagaValidationFlags_BINDINGS = 0x20,
} NagaValidationFlags;

// --- naga::back::dot ---

typedef struct NagaDOTBackOptions {
	NagaBool cfg_only;
} NagaDOTBackOptions;

// --- naga::back::glsl ---

typedef enum NagaGLSLBackVersionTag {
	NagaGLSLBackVersionTag_Desktop,
	NagaGLSLBackVersionTag_Embedded,
} NagaGLSLBackVersionTag;

typedef struct NagaGLSLBackVersion {
	NagaGLSLBackVersionTag tag;
	union {
		uint16_t desktop;
		struct {
			uint16_t version;
			NagaBool is_webgl;
		} embedded;
	} data;
} NagaGLSLBackVersion;

typedef uint32_t NagaGLSLBackWriterFlagsFlags;
typedef enum NagaGLSLBackWriterFlags {
	NagaGLSLBackWriterFlags_ADJUST_COORDINATE_SPACE = 0x1,
	NagaGLSLBackWriterFlags_TEXTURE_SHADOW_LOD = 0x2,
	NagaGLSLBackWriterFlags_DRAW_PARAMETERS = 0x4,
	NagaGLSLBackWriterFlags_INCLUDE_UNUSED_ITEMS = 0x10,
	NagaGLSLBackWriterFlags_FORCE_POINT_SIZE = 0x20,
} NagaGLSLBackWriterFlags;

typedef struct NagaGLSLBackBindingMapEntry {
	NagaResourceBinding key;
	uint8_t value;
} NagaGLSLBackBindingMapEntry;

typedef struct NagaGLSLBackBindingMap {
	NagaGLSLBackBindingMapEntry *entries;
	size_t entries_len;
} NagaGLSLBackBindingMap;

typedef struct NagaGLSLBackOptions {
	NagaGLSLBackVersion version;
	NagaGLSLBackWriterFlagsFlags writer_flags;
	NagaGLSLBackBindingMap binding_map;
	NagaBool zero_initialize_workgroup_memory;
} NagaGLSLBackOptions;

typedef struct NagaGLSLBackPipelineOptions {
	NagaShaderStage shader_stage;
	char *entry_point;
	DEFINE_OPTIONAL(uint32_t)
	multiview;
} NagaGLSLBackPipelineOptions;

typedef uint32_t NagaGLSLBackFeaturesFlags;
typedef enum NagaGLSLBackFeatures {
	NagaGLSLBackFeatures_BUFFER_STORAGE = 0x1,
	NagaGLSLBackFeatures_ARRAY_OF_ARRAYS = 0x2,
	NagaGLSLBackFeatures_DOUBLE_TYPE = 0x4,
	NagaGLSLBackFeatures_FULL_IMAGE_FORMATS = 0x8,
	NagaGLSLBackFeatures_MULTISAMPLED_TEXTURES = 0x10,
	NagaGLSLBackFeatures_MULTISAMPLED_TEXTURE_ARRAYS = 0x20,
	NagaGLSLBackFeatures_CUBE_TEXTURES_ARRAY = 0x40,
	NagaGLSLBackFeatures_COMPUTE_SHADER = 0x80,
	NagaGLSLBackFeatures_IMAGE_LOAD_STORE = 0x100,
	NagaGLSLBackFeatures_CONSERVATIVE_DEPTH = 0x200,
	NagaGLSLBackFeatures_NOPERSPECTIVE_QUALIFIER = 0x800,
	NagaGLSLBackFeatures_SAMPLE_QUALIFIER = 0x1000,
	NagaGLSLBackFeatures_CLIP_DISTANCE = 0x2000,
	NagaGLSLBackFeatures_CULL_DISTANCE = 0x4000,
	NagaGLSLBackFeatures_SAMPLE_VARIABLES = 0x8000,
	NagaGLSLBackFeatures_DYNAMIC_ARRAY_SIZE = 0x10000,
	NagaGLSLBackFeatures_MULTI_VIEW = 0x20000,
	NagaGLSLBackFeatures_TEXTURE_SAMPLES = 0x40000,
	NagaGLSLBackFeatures_TEXTURE_LEVELS = 0x80000,
	NagaGLSLBackFeatures_IMAGE_SIZE = 0x100000,
	NagaGLSLBackFeatures_DUAL_SOURCE_BLENDING = 0x200000,
	NagaGLSLBackFeatures_INSTANCE_INDEX = 0x400000,
	NagaGLSLBackFeatures_TEXTURE_SHADOW_LOD = 0x800000,
	NagaGLSLBackFeatures_SUBGROUP_OPERATIONS = 0x1000000,
	NagaGLSLBackFeatures_TEXTURE_ATOMICS = 0x2000000,
	NagaGLSLBackFeatures_SHADER_BARYCENTRICS = 0x4000000,
} NagaGLSLBackFeatures;

typedef enum NagaGLSLBackErrorTag {
	NagaGLSLBackErrorTag_FmtError,
	NagaGLSLBackErrorTag_MissingFeatures,
	NagaGLSLBackErrorTag_MultipleImmediateData,
	NagaGLSLBackErrorTag_VersionNotSupported,
	NagaGLSLBackErrorTag_EntryPointNotFound,
	NagaGLSLBackErrorTag_UnsupportedExternal,
	NagaGLSLBackErrorTag_UnsupportedScalar,
	NagaGLSLBackErrorTag_ImageMultipleSamplers,
	NagaGLSLBackErrorTag_Custom,
	NagaGLSLBackErrorTag_Override,
	NagaGLSLBackErrorTag_FirstSamplingNotSupported,
	NagaGLSLBackErrorTag_ResolveArraySizeError,
} NagaGLSLBackErrorTag;

typedef struct NagaGLSLBackError {
	NagaGLSLBackErrorTag tag;
	union {
		char *fmt_error;
		NagaGLSLBackFeaturesFlags missing_features;
		char *unsupported_external;
		NagaScalar unsupported_scalar;
		char *custom;
		NagaResolveArraySizeError resolve_array_size_error;
	} data;
} NagaGLSLBackError;

typedef struct NagaEmpty NAGA_UNIMPLEMENTED *NagaGLSLBackReflectionInfo;

// --- naga::back::hlsl ---

typedef enum NagaHLSLBackShaderModel {
	NagaHLSLBackShaderModel_V5_0,
	NagaHLSLBackShaderModel_V5_1,
	NagaHLSLBackShaderModel_V6_0,
	NagaHLSLBackShaderModel_V6_1,
	NagaHLSLBackShaderModel_V6_2,
	NagaHLSLBackShaderModel_V6_3,
	NagaHLSLBackShaderModel_V6_4,
	NagaHLSLBackShaderModel_V6_5,
	NagaHLSLBackShaderModel_V6_6,
	NagaHLSLBackShaderModel_V6_7,
} NagaHLSLBackShaderModel;

typedef struct NagaHLSLBackBindTarget {
	uint8_t space;
	uint32_t register_;
	DEFINE_OPTIONAL(uint32_t)
	binding_array_size;
	DEFINE_OPTIONAL(uint32_t)
	dynamic_storage_buffer_offsets_index;
	NagaBool restrict_indexing;
} NagaHLSLBackBindTarget;

typedef struct NagaHLSLBackBindingMapEntry {
	NagaResourceBinding key;
	NagaHLSLBackBindTarget value;
} NagaHLSLBackBindingMapEntry;

typedef struct NagaHLSLBackBindingMap {
	NagaHLSLBackBindingMapEntry *entries;
	size_t entries_len;
} NagaHLSLBackBindingMap;

typedef struct NagaHLSLBackSamplerHeapBindTargets {
	NagaHLSLBackBindTarget standard_samplers;
	NagaHLSLBackBindTarget comparison_samplers;
} NagaHLSLBackSamplerHeapBindTargets;

typedef struct NagaHLSLBackSamplerIndexBufferKey {
	uint32_t group;
} NagaHLSLBackSamplerIndexBufferKey;

typedef struct NagaHLSLBackSamplerIndexBufferBindingMapEntry {
	NagaHLSLBackSamplerIndexBufferKey key;
	NagaHLSLBackBindTarget value;
} NagaHLSLBackSamplerIndexBufferBindingMapEntry;

typedef struct NagaHLSLBackSamplerIndexBufferBindingMap {
	NagaHLSLBackSamplerIndexBufferBindingMapEntry *entries;
	size_t entries_len;
} NagaHLSLBackSamplerIndexBufferBindingMap;

typedef struct NagaHLSLBackOffsetsBindTarget {
	uint8_t space;
	uint32_t register_;
	uint32_t size;
} NagaHLSLBackOffsetsBindTarget;

typedef struct NagaHLSLBackDynamicStorageBufferOffsetsTargetsEntry {
	uint32_t key;
	NagaHLSLBackOffsetsBindTarget value;
} NagaHLSLBackDynamicStorageBufferOffsetsTargetsEntry;

typedef struct NagaHLSLBackDynamicStorageBufferOffsetsTargets {
	NagaHLSLBackDynamicStorageBufferOffsetsTargetsEntry *entries;
	size_t entries_len;
} NagaHLSLBackDynamicStorageBufferOffsetsTargets;

typedef struct NagaHLSLBackExternalTextureBindTarget {
	NagaHLSLBackBindTarget planes[3];
	NagaHLSLBackBindTarget params;
} NagaHLSLBackExternalTextureBindTarget;

typedef struct NagaHLSLBackExternalTextureBindingMapEntry {
	NagaResourceBinding key;
	NagaHLSLBackExternalTextureBindTarget value;
} NagaHLSLBackExternalTextureBindingMapEntry;

typedef struct NagaHLSLBackExternalTextureBindingMap {
	NagaHLSLBackExternalTextureBindingMapEntry *entries;
	size_t entries_len;
} NagaHLSLBackExternalTextureBindingMap;

typedef struct NagaHLSLBackOptions {
	NagaHLSLBackShaderModel shader_model;
	NagaHLSLBackBindingMap binding_map;
	NagaBool fake_missing_bindings;
	DEFINE_OPTIONAL(NagaHLSLBackBindTarget)
	special_constants_binding;
	DEFINE_OPTIONAL(NagaHLSLBackBindTarget)
	immediates_target;
	NagaHLSLBackSamplerHeapBindTargets sampler_heap_target;
	NagaHLSLBackSamplerIndexBufferBindingMap sampler_buffer_binding_map;
	NagaHLSLBackDynamicStorageBufferOffsetsTargets dynamic_storage_buffer_offsets_targets;
	NagaHLSLBackExternalTextureBindingMap external_texture_binding_map;
	NagaBool zero_initialize_workgroup_memory;
	NagaBool restrict_indexing;
	NagaBool force_loop_bounding;
} NagaHLSLBackOptions;

typedef struct NagaShaderStageString {
	NagaShaderStage shader_stage;
	char *string;

} NagaShaderStageString;

typedef struct NagaHLSLBackPipelineOptions {
	DEFINE_OPTIONAL(NagaShaderStageString)
	entry_point;
} NagaHLSLBackPipelineOptions;

typedef enum NagaHLSLBackErrorTag {
	NagaHLSLBackErrorTag_IoError,
	NagaHLSLBackErrorTag_UnsupportedScalar,
	NagaHLSLBackErrorTag_Unimplemented,
	NagaHLSLBackErrorTag_Custom,
	NagaHLSLBackErrorTag_Override,
	NagaHLSLBackErrorTag_ResolveArraySizeError,
	NagaHLSLBackErrorTag_EntryPointNotFound,
	NagaHLSLBackErrorTag_ShaderModelTooLow,
} NagaHLSLBackErrorTag;

typedef struct NagaHLSLBackError {
	NagaHLSLBackErrorTag tag;
	union {
		char *io_error;
		NagaScalar unsupported_scalar;
		char *unimplemented;
		char *custom;
		NagaResolveArraySizeError resolve_array_size_error;
		struct {
			NagaShaderStage stage;
			char *name;
		} entry_point_not_found;
		struct {
			char *message;
			NagaHLSLBackShaderModel model;
		} shader_model_too_low;
	} data;
} NagaHLSLBackError;

typedef struct NagaHLSLBackFragmentEntryPoint {
	struct NagaEmpty *NAGA_UNIMPLEMENTED module;
	struct NagaEmpty *NAGA_UNIMPLEMENTED func;
} NagaHLSLBackFragmentEntryPoint;

typedef struct NagaEmpty NAGA_UNIMPLEMENTED *NagaHLSLBackReflectionInfo;

// --- naga::back::msl ---

typedef enum NagaMSLBackSamplerAddress {
	NagaMSLBackSamplerAddress_Repeat,
	NagaMSLBackSamplerAddress_MirroredRepeat,
	NagaMSLBackSamplerAddress_ClampToEdge,
	NagaMSLBackSamplerAddress_ClampToZero,
	NagaMSLBackSamplerAddress_ClampToBorder,
} NagaMSLBackSamplerAddress;

typedef enum NagaMSLBackSamplerBorderColor {
	NagaMSLBackSamplerBorderColor_TransparentBlack,
	NagaMSLBackSamplerBorderColor_OpaqueBlack,
	NagaMSLBackSamplerBorderColor_OpaqueWhite,
} NagaMSLBackSamplerBorderColor;

typedef enum NagaMSLBackSamplerCompareFunc {
	NagaMSLBackSamplerCompareFunc_Never,
	NagaMSLBackSamplerCompareFunc_Less,
	NagaMSLBackSamplerCompareFunc_LessEqual,
	NagaMSLBackSamplerCompareFunc_Greater,
	NagaMSLBackSamplerCompareFunc_GreaterEqual,
	NagaMSLBackSamplerCompareFunc_Equal,
	NagaMSLBackSamplerCompareFunc_NotEqual,
	NagaMSLBackSamplerCompareFunc_Always,
} NagaMSLBackSamplerCompareFunc;

typedef enum NagaMSLBackSamplerCoord {
	NagaMSLBackSamplerCoord_Normalized,
	NagaMSLBackSamplerCoord_Pixel,
} NagaMSLBackSamplerCoord;

typedef enum NagaMSLBackSamplerFilter {
	NagaMSLBackSamplerFilter_Nearest,
	NagaMSLBackSamplerFilter_Linear,
} NagaMSLBackSamplerFilter;

typedef struct NagaMSLBackFloatRange {
	float start;
	float end;
} NagaMSLBackFloatFloat;

typedef struct NagaMSLBackInlineSampler {
	NagaMSLBackSamplerCoord coord;
	NagaMSLBackSamplerAddress address[3];
	NagaMSLBackSamplerBorderColor border_color;
	NagaMSLBackSamplerFilter mag_filter;
	NagaMSLBackSamplerFilter min_filter;
	DEFINE_OPTIONAL(NagaMSLBackSamplerFilter)
	mip_filter;
	DEFINE_OPTIONAL(NagaMSLBackFloatFloat)
	lod_clamp;
	DEFINE_OPTIONAL(uint32_t)
	max_anisotropy;
	NagaMSLBackSamplerCompareFunc compare_func;
} NagaMSLBackInlineSampler;

typedef uint8_t NagaMSLBackSlot;

typedef enum NagaMSLBackVertexFormat {
	NagaMSLBackVertexFormat_Uint8 = 0,
	NagaMSLBackVertexFormat_Uint8x2 = 1,
	NagaMSLBackVertexFormat_Uint8x4 = 2,
	NagaMSLBackVertexFormat_Sint8 = 3,
	NagaMSLBackVertexFormat_Sint8x2 = 4,
	NagaMSLBackVertexFormat_Sint8x4 = 5,
	NagaMSLBackVertexFormat_Unorm8 = 6,
	NagaMSLBackVertexFormat_Unorm8x2 = 7,
	NagaMSLBackVertexFormat_Unorm8x4 = 8,
	NagaMSLBackVertexFormat_Snorm8 = 9,
	NagaMSLBackVertexFormat_Snorm8x2 = 10,
	NagaMSLBackVertexFormat_Snorm8x4 = 11,
	NagaMSLBackVertexFormat_Uint16 = 12,
	NagaMSLBackVertexFormat_Uint16x2 = 13,
	NagaMSLBackVertexFormat_Uint16x4 = 14,
	NagaMSLBackVertexFormat_Sint16 = 15,
	NagaMSLBackVertexFormat_Sint16x2 = 16,
	NagaMSLBackVertexFormat_Sint16x4 = 17,
	NagaMSLBackVertexFormat_Unorm16 = 18,
	NagaMSLBackVertexFormat_Unorm16x2 = 19,
	NagaMSLBackVertexFormat_Unorm16x4 = 20,
	NagaMSLBackVertexFormat_Snorm16 = 21,
	NagaMSLBackVertexFormat_Snorm16x2 = 22,
	NagaMSLBackVertexFormat_Snorm16x4 = 23,
	NagaMSLBackVertexFormat_Float16 = 24,
	NagaMSLBackVertexFormat_Float16x2 = 25,
	NagaMSLBackVertexFormat_Float16x4 = 26,
	NagaMSLBackVertexFormat_Float32 = 27,
	NagaMSLBackVertexFormat_Float32x2 = 28,
	NagaMSLBackVertexFormat_Float32x3 = 29,
	NagaMSLBackVertexFormat_Float32x4 = 30,
	NagaMSLBackVertexFormat_Uint32 = 31,
	NagaMSLBackVertexFormat_Uint32x2 = 32,
	NagaMSLBackVertexFormat_Uint32x3 = 33,
	NagaMSLBackVertexFormat_Uint32x4 = 34,
	NagaMSLBackVertexFormat_Sint32 = 35,
	NagaMSLBackVertexFormat_Sint32x2 = 36,
	NagaMSLBackVertexFormat_Sint32x3 = 37,
	NagaMSLBackVertexFormat_Sint32x4 = 38,
	NagaMSLBackVertexFormat_Unorm10_10_10_2 = 43,
	NagaMSLBackVertexFormat_Unorm8x4Bgra = 44,
} NagaMSLBackVertexFormat;

typedef enum NagaMSLBackVertexBufferStepMode {
	NagaMSLBackVertexBufferStepMode_Constant,
	NagaMSLBackVertexBufferStepMode_ByVertex,
	NagaMSLBackVertexBufferStepMode_ByInstance,
} NagaMSLBackVertexBufferStepMode;

typedef struct NagaMSLBackAttributeMapping {
	uint32_t shader_location;
	uint32_t offset;
	NagaMSLBackVertexFormat format;
} NagaMSLBackAttributeMapping;

typedef struct NagaMSLBackBindExternalTextureTarget {
	NagaMSLBackSlot planes[3];
	NagaMSLBackSlot params;
} NagaMSLBackBindExternalTextureTarget;

typedef enum NagaMSLBackBindSamplerTargetTag {
	NagaMSLBackBindSamplerTargetTag_Resource,
	NagaMSLBackBindSamplerTargetTag_Inline,
} NagaMSLBackBindSamplerTargetTag;

typedef struct NagaMSLBackBindSamplerTarget {
	NagaMSLBackBindSamplerTargetTag tag;
	union {
		NagaMSLBackSlot resource;
		uint8_t inline_;
	} data;
} NagaMSLBackBindSamplerTarget;

typedef struct NagaMSLBackBindTarget {
	DEFINE_OPTIONAL(NagaMSLBackSlot)
	buffer;
	DEFINE_OPTIONAL(NagaMSLBackSlot)
	texture;
	DEFINE_OPTIONAL(NagaMSLBackBindSamplerTarget)
	sampler;
	DEFINE_OPTIONAL(NagaMSLBackBindExternalTextureTarget)
	external_texture;
	NagaBool mutable_;
} NagaMSLBackBindTarget;

typedef struct NagaMSLBackBindingMapEntry {
	NagaResourceBinding key;
	NagaMSLBackBindTarget value;
} NagaMSLBackBindingMapEntry;

typedef struct NagaMSLBackBindingMap {
	NagaMSLBackBindingMapEntry *entries;
	size_t entries_len;
} NagaMSLBackBindingMap;

typedef struct NagaMSLBackEntryPointResources {
	NagaMSLBackBindingMap resources;
	DEFINE_OPTIONAL(NagaMSLBackSlot)
	immediates_buffer;
	DEFINE_OPTIONAL(NagaMSLBackSlot)
	sizes_buffer;
} NagaMSLBackEntryPointResources;

typedef struct NagaMSLBackEntryPointResourceMapEntry {
	char *key;
	NagaMSLBackEntryPointResources value;
} NagaMSLBackEntryPointResourceMapEntry;

typedef struct NagaMSLBackEntryPointResourceMap {
	NagaMSLBackEntryPointResourceMapEntry *entries;
	size_t entries_len;
} NagaMSLBackEntryPointResourceMap;

typedef struct NagaMSLBackOptions {
	uint8_t lang_version[2];
	NagaMSLBackEntryPointResourceMap per_entry_point_map;
	NagaMSLBackInlineSampler *inline_samplers;
	size_t inline_samplers_len;
	NagaBool spirv_cross_compatibility;
	NagaBool fake_missing_bindings;
	NagaBoundsCheckPolicies bounds_check_policies;
	NagaBool zero_initialize_workgroup_memory;
	NagaBool force_loop_bounding;
} NagaMSLBackOptions;

typedef struct NagaMSLBackVertexBufferMapping {
	uint32_t id;
	uint32_t stride;
	NagaMSLBackVertexBufferStepMode step_mode;
	NagaMSLBackAttributeMapping *attributes;
	size_t attributes_len;
} NagaMSLBackVertexBufferMapping;

typedef struct NagaMSLBackPipelineOptions {
	DEFINE_OPTIONAL(NagaShaderStageString)
	entry_point;
	NagaBool allow_and_force_point_size;
	NagaBool vertex_pulling_transform;
	NagaMSLBackVertexBufferMapping *vertex_buffer_mappings;
	size_t vertex_buffer_mappings_len;
} NagaMSLBackPipelineOptions;

typedef enum NagaMSLBackEntryPointErrorTag {
	NagaMSLBackEntryPointErrorTag_MissingBinding,
	NagaMSLBackEntryPointErrorTag_MissingBindTarget,
	NagaMSLBackEntryPointErrorTag_MissingImmediateData,
	NagaMSLBackEntryPointErrorTag_MissingSizesBuffer,
} NagaMSLBackEntryPointErrorTag;

typedef struct NagaMSLBackEntryPointError {
	NagaMSLBackEntryPointErrorTag tag;
	union {
		char *missing_binding;
		NagaResourceBinding missing_bind_target;
	} data;
} NagaMSLBackEntryPointError;

typedef enum NagaMSLBackErrorTag {
	NagaMSLBackErrorTag_Format,
	NagaMSLBackErrorTag_UnimplementedBindTarget,
	NagaMSLBackErrorTag_UnsupportedCompose,
	NagaMSLBackErrorTag_UnsupportedBinaryOp,
	NagaMSLBackErrorTag_UnsupportedCall,
	NagaMSLBackErrorTag_FeatureNotImplemented,
	NagaMSLBackErrorTag_GenericValidation,
	NagaMSLBackErrorTag_UnsupportedBuiltIn,
	NagaMSLBackErrorTag_CapabilityNotSupported,
	NagaMSLBackErrorTag_UnsupportedAttribute,
	NagaMSLBackErrorTag_UnsupportedFunction,
	NagaMSLBackErrorTag_UnsupportedWriteableStorageBuffer,
	NagaMSLBackErrorTag_UnsupportedWriteableStorageTexture,
	NagaMSLBackErrorTag_UnsupportedRWStorageTexture,
	NagaMSLBackErrorTag_UnsupportedArrayOf,
	NagaMSLBackErrorTag_UnsupportedArrayOfType,
	NagaMSLBackErrorTag_UnsupportedRayTracing,
	NagaMSLBackErrorTag_Override,
	NagaMSLBackErrorTag_UnsupportedBitCast,
	NagaMSLBackErrorTag_ResolveArraySizeError,
	NagaMSLBackErrorTag_EntryPointNotFound,
} NagaMSLBackErrorTag;

typedef struct NagaMSLBackError {
	NagaMSLBackErrorTag tag;
	union {
		char *format;
		NagaMSLBackBindTarget unimplemented_bind_target;
		DEFINE_HANDLE_INDEX(NagaType)
		unsupported_compose;
		struct NagaEmpty *NAGA_UNIMPLEMENTED unsupported_binary_op;
		char *unsupported_call;
		char *feature_not_implemented;
		char *generic_validation;
		NagaBuiltIn unsupported_built_in;
		NagaCapabilitiesFlags capability_not_supported;
		char *unsupported_attribute;
		char *unsupported_function;
		NagaShaderStage unsupported_writeable_storage_texture;
		char *unsupported_array_of;
		DEFINE_HANDLE_INDEX(NagaType)
		unsupported_array_of_type;
		NagaTypeInner unsupported_bit_cast;
		NagaResolveArraySizeError resolve_array_size_error;
		NagaShaderStageString entry_point_not_found;
	} data;
} NagaMSLBackError;

typedef struct NagaEmpty NAGA_UNIMPLEMENTED *NagaMSLBackTranslationInfo;

// --- naga::back::spv ---

typedef enum NagaSPVBackCapability {
	NagaSPVBackCapability_Matrix = 0,
	NagaSPVBackCapability_Shader = 1,
	NagaSPVBackCapability_Geometry = 2,
	NagaSPVBackCapability_Tessellation = 3,
	NagaSPVBackCapability_Addresses = 4,
	NagaSPVBackCapability_Linkage = 5,
	NagaSPVBackCapability_Kernel = 6,
	NagaSPVBackCapability_Vector16 = 7,
	NagaSPVBackCapability_Float16Buffer = 8,
	NagaSPVBackCapability_Float16 = 9,
	NagaSPVBackCapability_Float64 = 10,
	NagaSPVBackCapability_Int64 = 11,
	NagaSPVBackCapability_Int64Atomics = 12,
	NagaSPVBackCapability_ImageBasic = 13,
	NagaSPVBackCapability_ImageReadWrite = 14,
	NagaSPVBackCapability_ImageMipmap = 15,
	NagaSPVBackCapability_Pipes = 17,
	NagaSPVBackCapability_Groups = 18,
	NagaSPVBackCapability_DeviceEnqueue = 19,
	NagaSPVBackCapability_LiteralSampler = 20,
	NagaSPVBackCapability_AtomicStorage = 21,
	NagaSPVBackCapability_Int16 = 22,
	NagaSPVBackCapability_TessellationPointSize = 23,
	NagaSPVBackCapability_GeometryPointSize = 24,
	NagaSPVBackCapability_ImageGatherExtended = 25,
	NagaSPVBackCapability_StorageImageMultisample = 27,
	NagaSPVBackCapability_UniformBufferArrayDynamicIndexing = 28,
	NagaSPVBackCapability_SampledImageArrayDynamicIndexing = 29,
	NagaSPVBackCapability_StorageBufferArrayDynamicIndexing = 30,
	NagaSPVBackCapability_StorageImageArrayDynamicIndexing = 31,
	NagaSPVBackCapability_ClipDistance = 32,
	NagaSPVBackCapability_CullDistance = 33,
	NagaSPVBackCapability_ImageCubeArray = 34,
	NagaSPVBackCapability_SampleRateShading = 35,
	NagaSPVBackCapability_ImageRect = 36,
	NagaSPVBackCapability_SampledRect = 37,
	NagaSPVBackCapability_GenericPointer = 38,
	NagaSPVBackCapability_Int8 = 39,
	NagaSPVBackCapability_InputAttachment = 40,
	NagaSPVBackCapability_SparseResidency = 41,
	NagaSPVBackCapability_MinLod = 42,
	NagaSPVBackCapability_Sampled1D = 43,
	NagaSPVBackCapability_Image1D = 44,
	NagaSPVBackCapability_SampledCubeArray = 45,
	NagaSPVBackCapability_SampledBuffer = 46,
	NagaSPVBackCapability_ImageBuffer = 47,
	NagaSPVBackCapability_ImageMSArray = 48,
	NagaSPVBackCapability_StorageImageExtendedFormats = 49,
	NagaSPVBackCapability_ImageQuery = 50,
	NagaSPVBackCapability_DerivativeControl = 51,
	NagaSPVBackCapability_InterpolationFunction = 52,
	NagaSPVBackCapability_TransformFeedback = 53,
	NagaSPVBackCapability_GeometryStreams = 54,
	NagaSPVBackCapability_StorageImageReadWithoutFormat = 55,
	NagaSPVBackCapability_StorageImageWriteWithoutFormat = 56,
	NagaSPVBackCapability_MultiViewport = 57,
	NagaSPVBackCapability_SubgroupDispatch = 58,
	NagaSPVBackCapability_NamedBarrier = 59,
	NagaSPVBackCapability_PipeStorage = 60,
	NagaSPVBackCapability_GroupNonUniform = 61,
	NagaSPVBackCapability_GroupNonUniformVote = 62,
	NagaSPVBackCapability_GroupNonUniformArithmetic = 63,
	NagaSPVBackCapability_GroupNonUniformBallot = 64,
	NagaSPVBackCapability_GroupNonUniformShuffle = 65,
	NagaSPVBackCapability_GroupNonUniformShuffleRelative = 66,
	NagaSPVBackCapability_GroupNonUniformClustered = 67,
	NagaSPVBackCapability_GroupNonUniformQuad = 68,
	NagaSPVBackCapability_ShaderLayer = 69,
	NagaSPVBackCapability_ShaderViewportIndex = 70,
	NagaSPVBackCapability_UniformDecoration = 71,
	NagaSPVBackCapability_CoreBuiltinsARM = 4165,
	NagaSPVBackCapability_TileImageColorReadAccessEXT = 4166,
	NagaSPVBackCapability_TileImageDepthReadAccessEXT = 4167,
	NagaSPVBackCapability_TileImageStencilReadAccessEXT = 4168,
	NagaSPVBackCapability_FragmentShadingRateKHR = 4422,
	NagaSPVBackCapability_SubgroupBallotKHR = 4423,
	NagaSPVBackCapability_DrawParameters = 4427,
	NagaSPVBackCapability_WorkgroupMemoryExplicitLayoutKHR = 4428,
	NagaSPVBackCapability_WorkgroupMemoryExplicitLayout8BitAccessKHR = 4429,
	NagaSPVBackCapability_WorkgroupMemoryExplicitLayout16BitAccessKHR = 4430,
	NagaSPVBackCapability_SubgroupVoteKHR = 4431,
	NagaSPVBackCapability_StorageBuffer16BitAccess = 4433,
	NagaSPVBackCapability_UniformAndStorageBuffer16BitAccess = 4434,
	NagaSPVBackCapability_StoragePushConstant16 = 4435,
	NagaSPVBackCapability_StorageInputOutput16 = 4436,
	NagaSPVBackCapability_DeviceGroup = 4437,
	NagaSPVBackCapability_MultiView = 4439,
	NagaSPVBackCapability_VariablePointersStorageBuffer = 4441,
	NagaSPVBackCapability_VariablePointers = 4442,
	NagaSPVBackCapability_AtomicStorageOps = 4445,
	NagaSPVBackCapability_SampleMaskPostDepthCoverage = 4447,
	NagaSPVBackCapability_StorageBuffer8BitAccess = 4448,
	NagaSPVBackCapability_UniformAndStorageBuffer8BitAccess = 4449,
	NagaSPVBackCapability_StoragePushConstant8 = 4450,
	NagaSPVBackCapability_DenormPreserve = 4464,
	NagaSPVBackCapability_DenormFlushToZero = 4465,
	NagaSPVBackCapability_SignedZeroInfNanPreserve = 4466,
	NagaSPVBackCapability_RoundingModeRTE = 4467,
	NagaSPVBackCapability_RoundingModeRTZ = 4468,
	NagaSPVBackCapability_RayQueryProvisionalKHR = 4471,
	NagaSPVBackCapability_RayQueryKHR = 4472,
	NagaSPVBackCapability_RayTraversalPrimitiveCullingKHR = 4478,
	NagaSPVBackCapability_RayTracingKHR = 4479,
	NagaSPVBackCapability_TextureSampleWeightedQCOM = 4484,
	NagaSPVBackCapability_TextureBoxFilterQCOM = 4485,
	NagaSPVBackCapability_TextureBlockMatchQCOM = 4486,
	NagaSPVBackCapability_Float16ImageAMD = 5008,
	NagaSPVBackCapability_ImageGatherBiasLodAMD = 5009,
	NagaSPVBackCapability_FragmentMaskAMD = 5010,
	NagaSPVBackCapability_StencilExportEXT = 5013,
	NagaSPVBackCapability_ImageReadWriteLodAMD = 5015,
	NagaSPVBackCapability_Int64ImageEXT = 5016,
	NagaSPVBackCapability_ShaderClockKHR = 5055,
	NagaSPVBackCapability_ShaderEnqueueAMDX = 5067,
	NagaSPVBackCapability_SampleMaskOverrideCoverageNV = 5249,
	NagaSPVBackCapability_GeometryShaderPassthroughNV = 5251,
	NagaSPVBackCapability_ShaderViewportIndexLayerEXT = 5254,
	NagaSPVBackCapability_ShaderViewportMaskNV = 5255,
	NagaSPVBackCapability_ShaderStereoViewNV = 5259,
	NagaSPVBackCapability_PerViewAttributesNV = 5260,
	NagaSPVBackCapability_FragmentFullyCoveredEXT = 5265,
	NagaSPVBackCapability_MeshShadingNV = 5266,
	NagaSPVBackCapability_ImageFootprintNV = 5282,
	NagaSPVBackCapability_MeshShadingEXT = 5283,
	NagaSPVBackCapability_FragmentBarycentricKHR = 5284,
	NagaSPVBackCapability_ComputeDerivativeGroupQuadsNV = 5288,
	NagaSPVBackCapability_FragmentDensityEXT = 5291,
	NagaSPVBackCapability_GroupNonUniformPartitionedNV = 5297,
	NagaSPVBackCapability_ShaderNonUniform = 5301,
	NagaSPVBackCapability_RuntimeDescriptorArray = 5302,
	NagaSPVBackCapability_InputAttachmentArrayDynamicIndexing = 5303,
	NagaSPVBackCapability_UniformTexelBufferArrayDynamicIndexing = 5304,
	NagaSPVBackCapability_StorageTexelBufferArrayDynamicIndexing = 5305,
	NagaSPVBackCapability_UniformBufferArrayNonUniformIndexing = 5306,
	NagaSPVBackCapability_SampledImageArrayNonUniformIndexing = 5307,
	NagaSPVBackCapability_StorageBufferArrayNonUniformIndexing = 5308,
	NagaSPVBackCapability_StorageImageArrayNonUniformIndexing = 5309,
	NagaSPVBackCapability_InputAttachmentArrayNonUniformIndexing = 5310,
	NagaSPVBackCapability_UniformTexelBufferArrayNonUniformIndexing = 5311,
	NagaSPVBackCapability_StorageTexelBufferArrayNonUniformIndexing = 5312,
	NagaSPVBackCapability_RayTracingPositionFetchKHR = 5336,
	NagaSPVBackCapability_RayTracingNV = 5340,
	NagaSPVBackCapability_RayTracingMotionBlurNV = 5341,
	NagaSPVBackCapability_VulkanMemoryModel = 5345,
	NagaSPVBackCapability_VulkanMemoryModelDeviceScope = 5346,
	NagaSPVBackCapability_PhysicalStorageBufferAddresses = 5347,
	NagaSPVBackCapability_ComputeDerivativeGroupLinearNV = 5350,
	NagaSPVBackCapability_RayTracingProvisionalKHR = 5353,
	NagaSPVBackCapability_CooperativeMatrixNV = 5357,
	NagaSPVBackCapability_FragmentShaderSampleInterlockEXT = 5363,
	NagaSPVBackCapability_FragmentShaderShadingRateInterlockEXT = 5372,
	NagaSPVBackCapability_ShaderSMBuiltinsNV = 5373,
	NagaSPVBackCapability_FragmentShaderPixelInterlockEXT = 5378,
	NagaSPVBackCapability_DemoteToHelperInvocation = 5379,
	NagaSPVBackCapability_DisplacementMicromapNV = 5380,
	NagaSPVBackCapability_RayTracingOpacityMicromapEXT = 5381,
	NagaSPVBackCapability_ShaderInvocationReorderNV = 5383,
	NagaSPVBackCapability_BindlessTextureNV = 5390,
	NagaSPVBackCapability_RayQueryPositionFetchKHR = 5391,
	NagaSPVBackCapability_RayTracingDisplacementMicromapNV = 5409,
	NagaSPVBackCapability_SubgroupShuffleINTEL = 5568,
	NagaSPVBackCapability_SubgroupBufferBlockIOINTEL = 5569,
	NagaSPVBackCapability_SubgroupImageBlockIOINTEL = 5570,
	NagaSPVBackCapability_SubgroupImageMediaBlockIOINTEL = 5579,
	NagaSPVBackCapability_RoundToInfinityINTEL = 5582,
	NagaSPVBackCapability_FloatingPointModeINTEL = 5583,
	NagaSPVBackCapability_IntegerFunctions2INTEL = 5584,
	NagaSPVBackCapability_FunctionPointersINTEL = 5603,
	NagaSPVBackCapability_IndirectReferencesINTEL = 5604,
	NagaSPVBackCapability_AsmINTEL = 5606,
	NagaSPVBackCapability_AtomicFloat32MinMaxEXT = 5612,
	NagaSPVBackCapability_AtomicFloat64MinMaxEXT = 5613,
	NagaSPVBackCapability_AtomicFloat16MinMaxEXT = 5616,
	NagaSPVBackCapability_VectorComputeINTEL = 5617,
	NagaSPVBackCapability_VectorAnyINTEL = 5619,
	NagaSPVBackCapability_ExpectAssumeKHR = 5629,
	NagaSPVBackCapability_SubgroupAvcMotionEstimationINTEL = 5696,
	NagaSPVBackCapability_SubgroupAvcMotionEstimationIntraINTEL = 5697,
	NagaSPVBackCapability_SubgroupAvcMotionEstimationChromaINTEL = 5698,
	NagaSPVBackCapability_VariableLengthArrayINTEL = 5817,
	NagaSPVBackCapability_FunctionFloatControlINTEL = 5821,
	NagaSPVBackCapability_FPGAMemoryAttributesINTEL = 5824,
	NagaSPVBackCapability_FPFastMathModeINTEL = 5837,
	NagaSPVBackCapability_ArbitraryPrecisionIntegersINTEL = 5844,
	NagaSPVBackCapability_ArbitraryPrecisionFloatingPointINTEL = 5845,
	NagaSPVBackCapability_UnstructuredLoopControlsINTEL = 5886,
	NagaSPVBackCapability_FPGALoopControlsINTEL = 5888,
	NagaSPVBackCapability_KernelAttributesINTEL = 5892,
	NagaSPVBackCapability_FPGAKernelAttributesINTEL = 5897,
	NagaSPVBackCapability_FPGAMemoryAccessesINTEL = 5898,
	NagaSPVBackCapability_FPGAClusterAttributesINTEL = 5904,
	NagaSPVBackCapability_LoopFuseINTEL = 5906,
	NagaSPVBackCapability_FPGADSPControlINTEL = 5908,
	NagaSPVBackCapability_MemoryAccessAliasingINTEL = 5910,
	NagaSPVBackCapability_FPGAInvocationPipeliningAttributesINTEL = 5916,
	NagaSPVBackCapability_FPGABufferLocationINTEL = 5920,
	NagaSPVBackCapability_ArbitraryPrecisionFixedPointINTEL = 5922,
	NagaSPVBackCapability_USMStorageClassesINTEL = 5935,
	NagaSPVBackCapability_RuntimeAlignedAttributeINTEL = 5939,
	NagaSPVBackCapability_IOPipesINTEL = 5943,
	NagaSPVBackCapability_BlockingPipesINTEL = 5945,
	NagaSPVBackCapability_FPGARegINTEL = 5948,
	NagaSPVBackCapability_DotProductInputAll = 6016,
	NagaSPVBackCapability_DotProductInput4x8Bit = 6017,
	NagaSPVBackCapability_DotProductInput4x8BitPacked = 6018,
	NagaSPVBackCapability_DotProduct = 6019,
	NagaSPVBackCapability_RayCullMaskKHR = 6020,
	NagaSPVBackCapability_CooperativeMatrixKHR = 6022,
	NagaSPVBackCapability_BitInstructions = 6025,
	NagaSPVBackCapability_GroupNonUniformRotateKHR = 6026,
	NagaSPVBackCapability_AtomicFloat32AddEXT = 6033,
	NagaSPVBackCapability_AtomicFloat64AddEXT = 6034,
	NagaSPVBackCapability_LongConstantCompositeINTEL = 6089,
	NagaSPVBackCapability_OptNoneINTEL = 6094,
	NagaSPVBackCapability_AtomicFloat16AddEXT = 6095,
	NagaSPVBackCapability_DebugInfoModuleINTEL = 6114,
	NagaSPVBackCapability_BFloat16ConversionINTEL = 6115,
	NagaSPVBackCapability_SplitBarrierINTEL = 6141,
	NagaSPVBackCapability_GlobalVariableFPGADecorationsINTEL = 6146,
	NagaSPVBackCapability_FPGAKernelAttributesv2INTEL = 6161,
	NagaSPVBackCapability_GlobalVariableHostAccessINTEL = 6167,
	NagaSPVBackCapability_FPMaxErrorINTEL = 6169,
	NagaSPVBackCapability_FPGALatencyControlINTEL = 6171,
	NagaSPVBackCapability_FPGAArgumentInterfacesINTEL = 6174,
	NagaSPVBackCapability_GroupUniformArithmeticKHR = 6400,
	NagaSPVBackCapability_CacheControlsINTEL = 6441,
} NagaSPVBackCapability;

typedef struct NagaSPVBackCapabilitySet {
	NagaSPVBackCapability *capabilities;
	size_t capabilities_len;
} NagaSPVBackCapabilities;

typedef uint32_t NagaSPVBackWriterFlagsFlags;
typedef enum NagaSPVBackWriterFlags {
	NagaSPVBackWriterFlags_DEBUG = 0x1,
	NagaSPVBackWriterFlags_ADJUST_COORDINATE_SPACE = 0x2,
	NagaSPVBackWriterFlags_LABEL_VARYINGS = 0x4,
	NagaSPVBackWriterFlags_FORCE_POINT_SIZE = 0x8,
	NagaSPVBackWriterFlags_CLAMP_FRAG_DEPTH = 0x10,
	NagaSPVBackWriterFlags_PRINT_ON_RAY_QUERY_INITIALIZATION_FAIL = 0x20,
} NagaSPVBackWriterFlags;

typedef struct NagaSPVBackBindingInfo {
	uint32_t descriptor_set;
	uint32_t binding;
	DEFINE_OPTIONAL(uint32_t)
	binding_array_size;
} NagaSPVBackBindingInfo;

typedef struct NagaSPVBackBindingMapEntry {
	NagaResourceBinding key;
	NagaSPVBackBindingInfo value;
} NagaSPVBackBindingMapEntry;

typedef struct NagaSPVBackBindingMap {
	NagaSPVBackBindingMapEntry *entries;
	size_t entries_len;
} NagaSPVBackBindingMap;

typedef enum NagaSPVBackZeroInitializeWorkgroupMemoryMode {
	NagaSPVBackZeroInitializeWorkgroupMemoryMode_Native,
	NagaSPVBackZeroInitializeWorkgroupMemoryMode_Polyfill,
	NagaSPVBackZeroInitializeWorkgroupMemoryMode_None,
} NagaSPVBackZeroInitializeWorkgroupMemoryMode;

typedef enum NagaSPVBackSourceLanguage {
	NagaSPVBackSourceLanguage_Unknown = 0,
	NagaSPVBackSourceLanguage_ESSL = 1,
	NagaSPVBackSourceLanguage_GLSL = 2,
	NagaSPVBackSourceLanguage_OpenCL_C = 3,
	NagaSPVBackSourceLanguage_OpenCL_CPP = 4,
	NagaSPVBackSourceLanguage_HLSL = 5,
	NagaSPVBackSourceLanguage_CPP_for_OpenCL = 6,
	NagaSPVBackSourceLanguage_SYCL = 7,
	NagaSPVBackSourceLanguage_HERO_C = 8,
	NagaSPVBackSourceLanguage_NZSL = 9,
	NagaSPVBackSourceLanguage_WGSL = 10,
	NagaSPVBackSourceLanguage_Slang = 11,
} NagaSPVBackSourceLanguage;

typedef struct NagaSPVBackDebugInfo {
	char *source_code;
	char *file_name;
	NagaSPVBackSourceLanguage language;
} NagaSPVBackDebugInfo;

typedef struct NagaSPVBackOptions {
	uint8_t lang_version[2];
	NagaSPVBackWriterFlagsFlags flags;
	NagaBool fake_missing_bindings;
	NagaSPVBackBindingMap binding_map;
	DEFINE_OPTIONAL(NagaSPVBackCapabilities)
	capabilities;
	NagaBoundsCheckPolicies bounds_check_policies;
	NagaSPVBackZeroInitializeWorkgroupMemoryMode zero_initialize_workgroup_memory;
	NagaBool force_loop_bounding;
	NagaBool ray_query_initialization_tracking;
	NagaBool use_storage_input_output_16;
	// DEFINE_OPTIONAL(NagaSPVBackDebugInfo)
	// debug_info;
	// NOTE: This type has an awkward lifetime on a borrowed string slice.
	struct NagaEmpty *debug_info;
} NagaSPVBackOptions;

typedef struct NagaSPVBackPipelineOptions {
	NagaShaderStage shader_stage;
	char *entry_point;
} NagaSPVBackPipelineOptions;

typedef enum NagaSPVBackErrorTag {
	NagaSPVBackErrorTag_EntryPointNotFound,
	NagaSPVBackErrorTag_UnsupportedVersion,
	NagaSPVBackErrorTag_MissingCapabilities,
	NagaSPVBackErrorTag_FeatureNotImplemented,
	NagaSPVBackErrorTag_Validation,
	NagaSPVBackErrorTag_Override,
	NagaSPVBackErrorTag_ResolveArraySizeError,
	NagaSPVBackErrorTag_SpirvVersionTooLow,
	NagaSPVBackErrorTag_MissingBinding,
} NagaSPVBackErrorTag;

typedef struct NagaSPVBackError {
	NagaSPVBackErrorTag tag;
	union {
		uint8_t unsupported_version[2];
		struct {
			char *error;
			NagaSPVBackCapability *capabilities;
			uint32_t capabilities_len;
		} missing_capabilities;
		char *feature_not_implemented;
		char *validation;
		NagaResolveArraySizeError resolve_array_size_error;
		uint8_t spirv_version_too_low[2];
		NagaResourceBinding missing_binding;
	} data;
} NagaSPVBackError;

// --- naga::back::wgsl ---

typedef uint32_t NagaWGSLBackWriterFlagsFlags;
typedef enum NagaWGSLBackWriterFlags {
	NagaWGSLBackWriterFlags_EXPLICIT_TYPES = 0x1,
} NagaWGSLBackWriterFlags;

typedef enum NagaWGSLBackErrorTag {
	NagaWGSLBackErrorTag_FmtError,
	NagaWGSLBackErrorTag_Custom,
	NagaWGSLBackErrorTag_Unimplemented,
	NagaWGSLBackErrorTag_UnsupportedRelationalFunction,
	NagaWGSLBackErrorTag_Unsupported,
} NagaWGSLBackErrorTag;

typedef struct NagaWGSLBackError {
	NagaWGSLBackErrorTag tag;
	union {
		char *fmt_error;
		char *custom;
		char *unimplemented;
		NagaRelationalFunction unsupported_relational_function;
		struct {
			char *kind;
			char *value;
		} unsupported;
	} data;
} NagaWGSLBackError;

// -- naga::back::pipeline_constants ---

typedef struct NagaPiplineConstant {
	const char *key;
	double value;
} NagaPipelineConstant;

typedef enum NagaPipelineConstantErrorTag {
	NagaPipelineConstantErrorTag_MissingValue,
	NagaPipelineConstantErrorTag_SrcNeedsToBeFinite,
	NagaPipelineConstantErrorTag_DstRangeTooSmall,
	NagaPipelineConstantErrorTag_ConstantEvaluatorError,
	NagaPipelineConstantErrorTag_ValidationError,
	NagaPipelineConstantErrorTag_NegativeWorkgroupSize,
	NagaPipelineConstantErrorTag_NegativeMeshOutputMax,
} NagaPipelineConstantErrorTag;

typedef struct NagaPipelineConstantError {
	NagaPipelineConstantErrorTag tag;
	union {
		char *missing_value;
		NagaConstantEvaluatorError constant_evaluator_error;
		struct NagaEmpty *NAGA_UNIMPLEMENTED validation_error;
	} data;
} NagaPipelineConstantError;

// --- naga::front::atmoic_upgrade ---

typedef enum NagaAtomicUpgradeFrontError {
	NagaAtomicUpgradeFront_Unsupported,
	NagaAtomicUpgradeFront_UnexpectedEndOfIndices,
	NagaAtomicUpgradeFront_GlobalInitUnsupported,
	NagaAtomicUpgradeFront_GlobalVariableMissing,
	NagaAtomicUpgradeFront_CompareExchangeNonScalarBaseType,
} NagaAtomicUpgradeFrontError;

// --- naga::front::glsl ---

typedef struct NagaGLSLFrontDefinesEntry {
	char *key;
	char *value;
} NagaGLSLFrontDefinesEntry;

typedef struct NagaGLSLFrontDefines {
	NagaGLSLFrontDefinesEntry *entries;
	size_t entries_len;
} NagaGLSLFrontDefines;

typedef struct NagaGLSLFrontOptions {
	NagaShaderStage stage;
	NagaGLSLFrontDefines defines;
} NagaGLSLFrontOptions;

typedef enum NagaGLSLFrontPrecision {
	NagaGLSLFrontPrecision_Low,
	NagaGLSLFrontPrecision_Medium,
	NagaGLSLFrontPrecision_High,
} NagaGLSLFrontPrecision;

typedef struct NagaGLSLFrontInteger {
	uint64_t value;
	NagaBool signed_;
	int32_t width;

} NagaGLSLFrontInteger;

typedef struct NagaGLSLFrontFloat {
	float value;
	int32_t width;
} NagaGLSLFrontFloat;

typedef enum NagaGLSLFrontTokenValueTag {
	NagaGLSLFrontTokenValueTag_Identifier,
	NagaGLSLFrontTokenValueTag_FloatConstant,
	NagaGLSLFrontTokenValueTag_IntConstant,
	NagaGLSLFrontTokenValueTag_BoolConstant,
	NagaGLSLFrontTokenValueTag_Layout,
	NagaGLSLFrontTokenValueTag_In,
	NagaGLSLFrontTokenValueTag_Out,
	NagaGLSLFrontTokenValueTag_InOut,
	NagaGLSLFrontTokenValueTag_Uniform,
	NagaGLSLFrontTokenValueTag_Buffer,
	NagaGLSLFrontTokenValueTag_Const,
	NagaGLSLFrontTokenValueTag_Shared,
	NagaGLSLFrontTokenValueTag_Restrict,
	NagaGLSLFrontTokenValueTag_MemoryQualifier,
	NagaGLSLFrontTokenValueTag_Invariant,
	NagaGLSLFrontTokenValueTag_Interpolation,
	NagaGLSLFrontTokenValueTag_Sampling,
	NagaGLSLFrontTokenValueTag_Precision,
	NagaGLSLFrontTokenValueTag_PrecisionQualifier,
	NagaGLSLFrontTokenValueTag_Continue,
	NagaGLSLFrontTokenValueTag_Break,
	NagaGLSLFrontTokenValueTag_Return,
	NagaGLSLFrontTokenValueTag_Discard,
	NagaGLSLFrontTokenValueTag_If,
	NagaGLSLFrontTokenValueTag_Else,
	NagaGLSLFrontTokenValueTag_Switch,
	NagaGLSLFrontTokenValueTag_Case,
	NagaGLSLFrontTokenValueTag_Default,
	NagaGLSLFrontTokenValueTag_While,
	NagaGLSLFrontTokenValueTag_Do,
	NagaGLSLFrontTokenValueTag_For,
	NagaGLSLFrontTokenValueTag_Void,
	NagaGLSLFrontTokenValueTag_Struct,
	NagaGLSLFrontTokenValueTag_TypeName,
	NagaGLSLFrontTokenValueTag_Assign,
	NagaGLSLFrontTokenValueTag_AddAssign,
	NagaGLSLFrontTokenValueTag_SubAssign,
	NagaGLSLFrontTokenValueTag_MulAssign,
	NagaGLSLFrontTokenValueTag_DivAssign,
	NagaGLSLFrontTokenValueTag_ModAssign,
	NagaGLSLFrontTokenValueTag_LeftShiftAssign,
	NagaGLSLFrontTokenValueTag_RightShiftAssign,
	NagaGLSLFrontTokenValueTag_AndAssign,
	NagaGLSLFrontTokenValueTag_XorAssign,
	NagaGLSLFrontTokenValueTag_OrAssign,
	NagaGLSLFrontTokenValueTag_Increment,
	NagaGLSLFrontTokenValueTag_Decrement,
	NagaGLSLFrontTokenValueTag_LogicalOr,
	NagaGLSLFrontTokenValueTag_LogicalAnd,
	NagaGLSLFrontTokenValueTag_LogicalXor,
	NagaGLSLFrontTokenValueTag_LessEqual,
	NagaGLSLFrontTokenValueTag_GreaterEqual,
	NagaGLSLFrontTokenValueTag_Equal,
	NagaGLSLFrontTokenValueTag_NotEqual,
	NagaGLSLFrontTokenValueTag_LeftShift,
	NagaGLSLFrontTokenValueTag_RightShift,
	NagaGLSLFrontTokenValueTag_LeftBrace,
	NagaGLSLFrontTokenValueTag_RightBrace,
	NagaGLSLFrontTokenValueTag_LeftParen,
	NagaGLSLFrontTokenValueTag_RightParen,
	NagaGLSLFrontTokenValueTag_LeftBracket,
	NagaGLSLFrontTokenValueTag_RightBracket,
	NagaGLSLFrontTokenValueTag_LeftAngle,
	NagaGLSLFrontTokenValueTag_RightAngle,
	NagaGLSLFrontTokenValueTag_Comma,
	NagaGLSLFrontTokenValueTag_Semicolon,
	NagaGLSLFrontTokenValueTag_Colon,
	NagaGLSLFrontTokenValueTag_Dot,
	NagaGLSLFrontTokenValueTag_Bang,
	NagaGLSLFrontTokenValueTag_Dash,
	NagaGLSLFrontTokenValueTag_Tilde,
	NagaGLSLFrontTokenValueTag_Plus,
	NagaGLSLFrontTokenValueTag_Star,
	NagaGLSLFrontTokenValueTag_Slash,
	NagaGLSLFrontTokenValueTag_Percent,
	NagaGLSLFrontTokenValueTag_VerticalBar,
	NagaGLSLFrontTokenValueTag_Caret,
	NagaGLSLFrontTokenValueTag_Ampersand,
	NagaGLSLFrontTokenValueTag_Question,
} NagaGLSLFrontTokenValueTag;

typedef struct NagaGLSLFrontTokenValue {
	NagaGLSLFrontTokenValueTag tag;
	union {
		char *identifier;
		NagaGLSLFrontFloat float_constant;
		NagaGLSLFrontInteger int_constant;
		NagaBool bool_constant;
		NagaStorageAccessFlags memory_qualifier;
		NagaInterpolation interpolation;
		NagaSampling sampling;
		NagaGLSLFrontPrecision precision_qualifier;
		NagaType type_name;
	} data;
} NagaGLSLFrontTokenValue;

typedef enum NagaGLSLFrontExpectedTokenTag {
	NagaGLSLFrontExpectedTokenTag_Token,
	NagaGLSLFrontExpectedTokenTag_TypeName,
	NagaGLSLFrontExpectedTokenTag_Identifier,
	NagaGLSLFrontExpectedTokenTag_IntLiteral,
	NagaGLSLFrontExpectedTokenTag_FloatLiteral,
	NagaGLSLFrontExpectedTokenTag_BoolLiteral,
	NagaGLSLFrontExpectedTokenTag_Eof,
} NagaGLSLFrontExpectedTokenTag;

typedef struct NagaGLSLFrontExpectedToken {
	NagaGLSLFrontExpectedTokenTag tag;
	union {
		NagaGLSLFrontTokenValue token;
	} data;
} NagaGLSLFrontExpectedToken;

typedef enum NagaGLSLFrontPreprocessorError {
	NagaGLSLFrontPreprocessorError_IntegerOverflow,
	NagaGLSLFrontPreprocessorError_FloatParsingError,
	NagaGLSLFrontPreprocessorError_UnexpectedCharacter,
	NagaGLSLFrontPreprocessorError_UnexpectedToken,
	NagaGLSLFrontPreprocessorError_UnexpectedHash,
	NagaGLSLFrontPreprocessorError_UnexpectedNewLine,
	NagaGLSLFrontPreprocessorError_UnexpectedEndOfInput,
	NagaGLSLFrontPreprocessorError_TooFewDefineArguments,
	NagaGLSLFrontPreprocessorError_TooManyDefineArguments,
	NagaGLSLFrontPreprocessorError_ErrorDirective,
	NagaGLSLFrontPreprocessorError_DuplicateParameter,
	NagaGLSLFrontPreprocessorError_UnknownDirective,
	NagaGLSLFrontPreprocessorError_DefineRedefined,
	NagaGLSLFrontPreprocessorError_ElifOutsideOfBlock,
	NagaGLSLFrontPreprocessorError_ElseOutsideOfBlock,
	NagaGLSLFrontPreprocessorError_EndifOutsideOfBlock,
	NagaGLSLFrontPreprocessorError_ElifAfterElse,
	NagaGLSLFrontPreprocessorError_MoreThanOneElse,
	NagaGLSLFrontPreprocessorError_UnfinishedBlock,
	NagaGLSLFrontPreprocessorError_LineOverflow,
	NagaGLSLFrontPreprocessorError_NotSupported16BitLiteral,
	NagaGLSLFrontPreprocessorError_NotSupported64BitLiteral,
	NagaGLSLFrontPreprocessorError_MacroNotDefined,
	NagaGLSLFrontPreprocessorError_RecursionLimitReached,
	NagaGLSLFrontPreprocessorError_DivisionByZero,
	NagaGLSLFrontPreprocessorError_RemainderByZero,
} NagaGLSLFrontPreprocessorError;

typedef enum NagaGLSLFrontErrorKindTag {
	NagaGLSLFrontErrorKindTag_EndOfFile,
	NagaGLSLFrontErrorKindTag_InvalidProfile,
	NagaGLSLFrontErrorKindTag_InvalidVersion,
	NagaGLSLFrontErrorKindTag_InvalidToken,
	NagaGLSLFrontErrorKindTag_NotImplemented,
	NagaGLSLFrontErrorKindTag_UnknownVariable,
	NagaGLSLFrontErrorKindTag_UnknownType,
	NagaGLSLFrontErrorKindTag_UnknownField,
	NagaGLSLFrontErrorKindTag_UnknownLayoutQualifier,
	NagaGLSLFrontErrorKindTag_UnsupportedMatrixWithTwoRowsInStd140,
	NagaGLSLFrontErrorKindTag_UnsupportedF16MatrixInStd140,
	NagaGLSLFrontErrorKindTag_VariableAlreadyDeclared,
	NagaGLSLFrontErrorKindTag_SemanticError,
	NagaGLSLFrontErrorKindTag_PreprocessorError,
	NagaGLSLFrontErrorKindTag_InternalError,
} NagaGLSLFrontErrorKindTag;

typedef struct NagaGLSLFrontErrorKind {
	NagaGLSLFrontErrorKindTag tag;
	union {
		char *invalid_profile;
		uint64_t invalid_version;
		struct {
			NagaGLSLFrontTokenValue token;
			NagaGLSLFrontExpectedToken *expected;
			size_t expected_len;
		} invalid_token;
		char *not_implemented;
		char *unknown_variable;
		char *unknown_type;
		char *unknown_field;
		char *unknown_layout_qualifier;
		struct {
			uint8_t columns;
		} unsupported_matrix_with_two_rows_in_std140;
		struct {
			uint8_t columns;
			uint8_t rows;
		} unsupported_f16_matrix_in_std140;
		char *variable_already_declared;
		char *semantic_error;
		// Requires questionable `pp_rs` dependency
		struct NagaEmpty *NAGA_UNIMPLEMENTED preprocessor_error;
		char *internal_error;
	} data;
} NagaGLSLFrontErrorKind;

typedef struct NagaGLSLFrontParseError {
	NagaGLSLFrontErrorKind kind;
	NagaSpan span;
} NagaGLSLFrontParseError;

typedef struct NagaGLSLFrontParseErrors {
	NagaGLSLFrontParseError *errors;
	size_t errors_len;
} NagaGLSLFrontParseErrors;

// --- naga::front::spv ---

typedef struct NagaSPVFrontOptions {
	NagaBool adjust_coordinate_space;
	NagaBool strict_capabilities;
	char *NAGA_NULLABLE block_ctx_dump_prefix;
} NagaSPVFrontOptions;

typedef enum NagaSPVFrontModuleState {
	NagaSPVFrontModuleState_Empty,
	NagaSPVFrontModuleState_Capability,
	NagaSPVFrontModuleState_Extension,
	NagaSPVFrontModuleState_ExtInstImport,
	NagaSPVFrontModuleState_MemoryModel,
	NagaSPVFrontModuleState_EntryPoint,
	NagaSPVFrontModuleState_ExecutionMode,
	NagaSPVFrontModuleState_Source,
	NagaSPVFrontModuleState_Name,
	NagaSPVFrontModuleState_ModuleProcessed,
	NagaSPVFrontModuleState_Annotation,
	NagaSPVFrontModuleState_Type,
	NagaSPVFrontModuleState_Function,
} NagaSPVFrontModuleState;

// private fields
//
// typedef struct NagaSPVFrontInstruction {
// 	uint16_t op;
// 	uint16_t wc;
// } NagaSPVFrontInstruction;

typedef enum NagaSPVFrontErrorTag {
	NagaSPVFrontErrorTag_InvalidHeader,
	NagaSPVFrontErrorTag_InvalidWordCount,
	NagaSPVFrontErrorTag_UnknownInstruction,
	NagaSPVFrontErrorTag_UnknownCapability,
	NagaSPVFrontErrorTag_UnsupportedInstruction,
	NagaSPVFrontErrorTag_UnsupportedCapability,
	NagaSPVFrontErrorTag_UnsupportedExtension,
	NagaSPVFrontErrorTag_UnsupportedExtSet,
	NagaSPVFrontErrorTag_UnsupportedExtInstSet,
	NagaSPVFrontErrorTag_UnsupportedExtInst,
	NagaSPVFrontErrorTag_UnsupportedType,
	NagaSPVFrontErrorTag_UnsupportedExecutionModel,
	NagaSPVFrontErrorTag_UnsupportedExecutionMode,
	NagaSPVFrontErrorTag_UnsupportedStorageClass,
	NagaSPVFrontErrorTag_UnsupportedImageDim,
	NagaSPVFrontErrorTag_UnsupportedImageFormat,
	NagaSPVFrontErrorTag_UnsupportedBuiltIn,
	NagaSPVFrontErrorTag_UnsupportedControlFlow,
	NagaSPVFrontErrorTag_UnsupportedBinaryOperator,
	NagaSPVFrontErrorTag_UnsupportedRuntimeArrayStorageClass,
	NagaSPVFrontErrorTag_UnsupportedMatrixStride,
	NagaSPVFrontErrorTag_UnknownBinaryOperator,
	NagaSPVFrontErrorTag_UnknownRelationalFunction,
	NagaSPVFrontErrorTag_UnsupportedGroupOperation,
	NagaSPVFrontErrorTag_InvalidParameter,
	NagaSPVFrontErrorTag_InvalidOperandCount,
	NagaSPVFrontErrorTag_InvalidOperand,
	NagaSPVFrontErrorTag_InvalidId,
	NagaSPVFrontErrorTag_InvalidDecoration,
	NagaSPVFrontErrorTag_InvalidTypeWidth,
	NagaSPVFrontErrorTag_InvalidSign,
	NagaSPVFrontErrorTag_InvalidInnerType,
	NagaSPVFrontErrorTag_InvalidVectorSize,
	NagaSPVFrontErrorTag_InvalidAccessType,
	NagaSPVFrontErrorTag_InvalidAccess,
	NagaSPVFrontErrorTag_InvalidAccessIndex,
	NagaSPVFrontErrorTag_InvalidIndexType,
	NagaSPVFrontErrorTag_InvalidBinding,
	NagaSPVFrontErrorTag_InvalidGlobalVar,
	NagaSPVFrontErrorTag_InvalidImageExpression,
	NagaSPVFrontErrorTag_InvalidImageWriteType,
	NagaSPVFrontErrorTag_InvalidImageBaseType,
	NagaSPVFrontErrorTag_InvalidImage,
	NagaSPVFrontErrorTag_InvalidAsType,
	NagaSPVFrontErrorTag_InvalidVectorType,
	NagaSPVFrontErrorTag_InconsistentComparisonSampling,
	NagaSPVFrontErrorTag_WrongFunctionResultType,
	NagaSPVFrontErrorTag_WrongFunctionArgumentType,
	NagaSPVFrontErrorTag_MissingDecoration,
	NagaSPVFrontErrorTag_BadString,
	NagaSPVFrontErrorTag_IncompleteData,
	NagaSPVFrontErrorTag_InvalidTerminator,
	NagaSPVFrontErrorTag_InvalidEdgeClassification,
	NagaSPVFrontErrorTag_ControlFlowGraphCycle,
	NagaSPVFrontErrorTag_FunctionCallCycle,
	NagaSPVFrontErrorTag_InvalidArraySize,
	NagaSPVFrontErrorTag_InvalidBarrierScope,
	NagaSPVFrontErrorTag_InvalidBarrierMemorySemantics,
	NagaSPVFrontErrorTag_NonBindingArrayOfImageOrSamplers,
	NagaSPVFrontErrorTag_SpecIdTooHigh,
	NagaSPVFrontErrorTag_AtomicUpgradeError,
} NagaSPVFrontErrorTag;

typedef struct NagaSPVFrontError {
	NagaSPVFrontErrorTag tag;
	union {
		uint16_t unknown_instruction;
		uint32_t unknown_capability;
		struct {
			NagaSPVFrontModuleState module_state;
			uint16_t op;
		} unsupported_instruction;
		uint32_t unsupported_capability;
		char *unsupported_extension;
		char *unsupported_ext_set;
		uint32_t unsupported_ext_inst_set;
		uint32_t unsupported_ext_inst;
		DEFINE_HANDLE_INDEX(NagaType)
		unsupported_type;
		uint32_t unsupported_execution_model;
		uint32_t unsupported_execution_mode;
		uint32_t unsupported_storage_class;
		uint32_t unsupported_image_dim;
		uint32_t unsupported_image_format;
		uint32_t unsupported_built_in;
		uint32_t unsupported_control_flow;
		uint32_t unsupported_binary_operator;
		struct {
			uint32_t stride;
			uint8_t columns;
			uint8_t rows;
			uint8_t width;
		} unsupported_matrix_stride;
		uint16_t unknown_binary_operator;
		uint16_t unknown_relational_function;
		uint32_t unsupported_group_operation;
		uint16_t invalid_parameter;
		struct {
			uint16_t op;
			uint16_t count;
		} invalid_operand_count;
		uint32_t invalid_id;
		uint32_t invalid_decoration;
		uint32_t invalid_type_width;
		uint32_t invalid_sign;
		uint32_t invalid_inner_type;
		uint32_t invalid_vector_size;
		uint32_t invalid_access_type;
		struct NagaEmpty *NAGA_UNIMPLEMENTED invalid_access;
		uint32_t invalid_access_index;
		uint32_t invalid_index_type;
		uint32_t invalid_binding;
		struct NagaEmpty *NAGA_UNIMPLEMENTED invalid_global_var;
		struct NagaEmpty *NAGA_UNIMPLEMENTED invalid_image_expression;
		DEFINE_HANDLE_INDEX(NagaType)
		invalid_image_base_type;
		DEFINE_HANDLE_INDEX(NagaType)
		invalid_image;
		DEFINE_HANDLE_INDEX(NagaType)
		invalid_as_type;
		DEFINE_HANDLE_INDEX(NagaType)
		invalid_vector_type;
		DEFINE_HANDLE_INDEX(NagaGlobalVariable)
		inconsistent_comparison_sampling;
		uint32_t wrong_function_result_type;
		uint32_t wrong_function_argument_type;
		uint32_t missing_decoration;
		uint32_t control_flow_graph_cycle;
		uint32_t function_call_cycle;
		uint32_t invalid_array_size;
		uint32_t invalid_barrier_scope;
		uint32_t invalid_barrier_memory_semantics;
		uint32_t spec_id_too_high;
		NagaAtomicUpgradeFrontError atomic_upgrade_error;
	} data;
} NagaSPVFrontError;

// --- naga::front::wgsl ---

typedef struct NagaWGSLFrontOptions {
	NagaBool parse_doc_comments;
} NagaWGSLFrontOptions;

typedef struct NagaWGSLFrontParseErrorLabel {
	NagaSpan span;
	char *message;
} NagaWGSLFrontParseErrorLabel;

typedef struct NagaWGSLFrontParseError {
	char *message;
	NagaWGSLFrontParseErrorLabel *labels;
	size_t labels_len;
} NagaWGSLFrontParseError;

// --- naga::compact

typedef enum NagaKeepUnused {
	NagaKeepUnused_No,
	NagaKeepUnused_Yes,
} NagaKeepUnused;

// --- Wrapper Methods ---

typedef uint32_t NagaFrontResultOptionFlags;
typedef enum NagaFrontResultOption {
	NagaFrontResultOption_FormattedErrorOnly = 0x1
} NagaFrontResultOption;

typedef struct NagaGLSLFrontResult {
	NagaFrontResultOptionFlags flags;
	NagaModule module;
	union {
		NagaGLSLFrontParseErrors errors;
		char *fmt_error;
	};
} NagaGLSLFrontResult;

typedef struct NagaSPVFrontResult {
	NagaFrontResultOptionFlags flags;
	NagaModule module;
	union {
		NagaSPVFrontError error;
		char *fmt_error;
	};
} NagaSPVFrontResult;

typedef struct NagaWGSLFrontResult {
	NagaFrontResultOptionFlags flags;
	NagaModule module;
	union {
		NagaWGSLFrontParseError error;
		char *fmt_error;
	};
} NagaWGSLFrontResult;

#ifndef NAGA_FFI_NO_METHODS

// Methods return true on success.
NagaBool naga_front_glsl_parse(
		NagaGLSLFrontOptions options,
		const char *source,
		NagaModuleFillFlags fill_flags,
		NagaGLSLFrontResult *out_result);
NagaBool naga_front_spv_parse(
		NagaSPVFrontOptions options,
		const uint32_t *source,
		uint32_t source_length,
		NagaModuleFillFlags fill_flags,
		NagaSPVFrontResult *out_result);
NagaBool naga_front_wgsl_parse(
		NagaWGSLFrontOptions options,
		const char *source,
		NagaModuleFillFlags fill_flags,
		NagaWGSLFrontResult *out_result);

#endif

typedef uint32_t NagaValidateResultOptionFlags;
typedef enum NagaValidateResultOption {
	NagaValidateResultOption_FormattedErrorOnly = 0x1
} NagaValidateResultOption;

typedef struct NagaValidateResult {
	NagaValidateResultOptionFlags flags;
	NagaModuleInfo module_info;
	union {
		struct NagaEmpty *NAGA_UNIMPLEMENTED error;
		char *fmt_error;
	};
} NagaValidateResult;

#ifndef NAGA_FFI_NO_METHODS

// Methods return true on success.
NagaValidator naga_valid_validator_new(NagaValidationFlagsFlags flags, NagaCapabilitiesFlags capabilities);
void naga_valid_validator_reset(NagaValidator *validator);
NagaBool naga_valid_validator_validate(NagaValidator *validator, NagaModule *const module, NagaValidateResult *out_result);
NagaBool naga_valid_validator_validate_resolved_overrides(NagaValidator *validator, NagaModule *const module, NagaValidateResult *out_result);

#endif

#ifndef NAGA_FFI_NO_METHODS

void naga_compact_compact(NagaModule *module, NagaKeepUnused keep_unused);

#endif

typedef uint32_t NagaWriteResultOptionFlags;
typedef enum NagaWriteResultOption {
	NagaWriteResultOption_FormattedErrorOnly = 0x1
} NagaWriteResultOption;

typedef struct NagaDOTWriteResult {
	NagaWriteResultOptionFlags flags;
	char *output;
	char *error;
} NagaDOTWriteResult;

typedef struct NagaGLSLWriteResult {
	NagaWriteResultOptionFlags flags;
	NagaGLSLBackReflectionInfo reflection_info;
	char *output;
	union {
		NagaGLSLBackError error;
		char *fmt_error;
	};
} NagaGLSLWriteResult;

typedef struct NagaHLSLWriteResult {
	NagaWriteResultOptionFlags flags;
	NagaHLSLBackReflectionInfo reflection_info;
	char *output;
	union {
		NagaHLSLBackError error;
		char *fmt_error;
	};
} NagaHLSLWriteResult;

typedef struct NagaMSLWriteResult {
	NagaWriteResultOptionFlags flags;
	NagaMSLBackTranslationInfo translation_info;
	char *output;
	union {
		NagaMSLBackError error;
		char *fmt_error;
	};
} NagaMSLWriteResult;

typedef struct NagaSPVWriteResult {
	NagaWriteResultOptionFlags flags;
	uint32_t *output;
	uint32_t output_count;
	union {
		NagaSPVBackError error;
		char *fmt_error;
	};
} NagaSPVWriteResult;

typedef struct NagaWGSLWriteResult {
	NagaWriteResultOptionFlags flags;
	char *output;
	union {
		NagaWGSLBackError error;
		char *fmt_error;
	};
} NagaWGSLWriteResult;

typedef struct NagaProcessOverridesResult {
	NagaWriteResultOptionFlags flags;
	NagaModule *module;
	NagaModuleInfo *module_info;
	union {
		NagaPipelineConstantError error;
		char *fmt_error;
	};
} NagaProcessOverridesResult;

#ifndef NAGA_FFI_NO_METHODS

// Methods return true on success.
NagaBool naga_back_dot_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaDOTBackOptions options,
		NagaDOTWriteResult *out_result);
NagaBool naga_back_glsl_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaGLSLBackOptions options,
		NagaGLSLBackPipelineOptions pipeline_options,
		NagaBoundsCheckPolicies policies,
		NagaGLSLWriteResult *out_result);
NagaBool naga_back_hlsl_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaHLSLBackOptions options,
		NagaHLSLBackPipelineOptions pipeline_options,
		NagaHLSLBackFragmentEntryPoint *NAGA_NULLABLE fragment_entry_point,
		NagaHLSLWriteResult *out_result);
NagaBool naga_back_msl_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaMSLBackOptions options,
		NagaMSLBackPipelineOptions pipeline_options,
		NagaMSLWriteResult *out_result);
NagaBool Naganaga_back_spv_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaSPVBackOptions options,
		NagaSPVBackPipelineOptions *NAGA_NULLABLE pipeline_options,
		NagaSPVWriteResult *out_result);
NagaBool naga_back_wgsl_write(
		NagaModule *const module,
		NagaModuleInfo *const module_info,
		NagaWGSLBackWriterFlagsFlags writer_flags,
		NagaWGSLWriteResult *out_result);
NagaBool naga_back_process_overrides(
		NagaModule *const module,
		NagaModuleFillFlags module_flags,
		NagaModuleInfo *const module_info,
		NagaModuleInfoFillFlags module_fill_flags,
		NagaShaderStage NAGA_NULLABLE entry_point_stage,
		const char *NAGA_NULLABLE entry_point_name,
		NagaPipelineConstant *const constants,
		uint32_t constants_count,
		NagaProcessOverridesResult *out_result);

#endif

#ifdef __cplusplus
}
#endif

#endif // NAGA_FFI_H
