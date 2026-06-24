// ShaderTranslator.h: flat C-ABI wrapper around ANGLE's shader translator (sh::* in
// GLSLANG/ShaderLang.h), built as a standalone shared library (libshadertranslator.{dylib,so,dll})
// that gfxstream dlopen()s at runtime via angle_shader_parser.cpp. The real ANGLE translator
// (~20MB static lib, built via Bazel from third_party/angle) is linked only into this small
// library, never into the main gfxstream binary -- keeping the heavy ANGLE dependency isolated.
//
// All types here are plain PODs with no ANGLE/STL types in their public layout, so this header
// can be included by gfxstream host code that never links against ANGLE itself (it only calls
// through the function pointers in STDispatch, resolved via dlsym at runtime).

#pragma once

#include <stdint.h>

#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* ST_Handle;

typedef enum ST_ShaderSpec {
    ST_GLES2_SPEC,
    ST_GLES3_SPEC,
    ST_GLES3_1_SPEC,
} ST_ShaderSpec;

typedef enum ST_ShaderOutput {
    ST_GLSL_COMPATIBILITY_OUTPUT,
    ST_GLSL_150_CORE_OUTPUT,
    ST_GLSL_330_CORE_OUTPUT,
    ST_GLSL_430_CORE_OUTPUT,
} ST_ShaderOutput;

enum {
    ST_OBJECT_CODE = 1 << 0,
    ST_VARIABLES = 1 << 1,
};

// Mirrors GLSLANG/ShaderLang.h's ShBuiltInResources field-for-field. Kept as an independent,
// trivially-constructible POD (no ANGLE ctor dependency) so gfxstream host code can default- and
// zero-construct it without linking ANGLE; the dylib converts to/from the real ShBuiltInResources
// internally.
typedef struct ST_BuiltInResources {
    int MaxVertexAttribs;
    int MaxVertexUniformVectors;
    int MaxVaryingVectors;
    int MaxVertexTextureImageUnits;
    int MaxCombinedTextureImageUnits;
    int MaxTextureImageUnits;
    int MaxFragmentUniformVectors;
    int MaxDrawBuffers;

    int OES_standard_derivatives;
    int OES_EGL_image_external;
    int OES_EGL_image_external_essl3;
    int NV_EGL_stream_consumer_external;
    int ARB_texture_rectangle;
    int EXT_blend_func_extended;
    int EXT_conservative_depth;
    int EXT_draw_buffers;
    int EXT_frag_depth;
    int EXT_shader_texture_lod;
    int EXT_shader_framebuffer_fetch;
    int EXT_shader_framebuffer_fetch_non_coherent;
    int NV_shader_framebuffer_fetch;
    int NV_shader_noperspective_interpolation;
    int ARM_shader_framebuffer_fetch;
    int ARM_shader_framebuffer_fetch_depth_stencil;
    int OVR_multiview;
    int OVR_multiview2;
    int EXT_multisampled_render_to_texture;
    int EXT_multisampled_render_to_texture2;
    int EXT_YUV_target;
    int EXT_geometry_shader;
    int OES_geometry_shader;
    int OES_shader_io_blocks;
    int EXT_shader_io_blocks;
    int EXT_gpu_shader5;
    int OES_gpu_shader5;
    int EXT_shader_non_constant_global_initializers;
    int OES_texture_storage_multisample_2d_array;
    int OES_texture_3D;
    int ANGLE_shader_pixel_local_storage;
    int ANGLE_texture_multisample;
    int ANGLE_multi_draw;
    int ANGLE_base_vertex_base_instance;
    int WEBGL_video_texture;
    int APPLE_clip_distance;
    int OES_texture_cube_map_array;
    int EXT_texture_cube_map_array;
    int EXT_texture_query_lod;
    int EXT_texture_shadow_lod;
    int EXT_shadow_samplers;
    int OES_shader_multisample_interpolation;
    int OES_shader_image_atomic;
    int EXT_tessellation_shader;
    int OES_tessellation_shader;
    int OES_texture_buffer;
    int EXT_texture_buffer;
    int OES_sample_variables;
    int EXT_clip_cull_distance;
    int ANGLE_clip_cull_distance;
    int EXT_primitive_bounding_box;
    int OES_primitive_bounding_box;
    int EXT_separate_shader_objects;
    int ANGLE_base_vertex_base_instance_shader_builtin;
    int ANDROID_extension_pack_es31a;
    int KHR_blend_equation_advanced;

    int NV_draw_buffers;
    int FragmentPrecisionHigh;

    int MaxVertexOutputVectors;
    int MaxFragmentInputVectors;
    int MinProgramTexelOffset;
    int MaxProgramTexelOffset;

    int MaxDualSourceDrawBuffers;

    // GLES 3.1 / compute constants.
    int MaxProgramTextureGatherOffset;
    int MinProgramTextureGatherOffset;
    int MaxImageUnits;
    int MaxComputeImageUniforms;
    int MaxVertexImageUniforms;
    int MaxFragmentImageUniforms;
    int MaxCombinedImageUniforms;
    int MaxCombinedShaderOutputResources;
    int MaxUniformLocations;
    int MaxComputeWorkGroupCount[3];
    int MaxComputeWorkGroupSize[3];
    int MaxComputeUniformComponents;
    int MaxComputeTextureImageUnits;
    int MaxComputeAtomicCounters;
    int MaxComputeAtomicCounterBuffers;
    int MaxVertexAtomicCounters;
    int MaxFragmentAtomicCounters;
    int MaxCombinedAtomicCounters;
    int MaxAtomicCounterBindings;
    int MaxVertexAtomicCounterBuffers;
    int MaxFragmentAtomicCounterBuffers;
    int MaxCombinedAtomicCounterBuffers;
    int MaxAtomicCounterBufferSize;
    int MaxUniformBufferBindings;
    int MaxShaderStorageBufferBindings;
} ST_BuiltInResources;

// Flat, heap-owned mirror of sh::ShaderVariable. Created by STCopyVariable, released by
// STDestroyVariable. `pFields`/`pArraySizes` are owned by this instance and freed recursively.
typedef struct ST_ShaderVariable {
    GLenum type;
    GLenum precision;
    char* name;
    int location;
    bool staticUse;
    bool isRowMajorLayout;

    uint32_t arraySizeCount;
    unsigned int* pArraySizes;

    uint32_t fieldsCount;
    struct ST_ShaderVariable* pFields;
} ST_ShaderVariable;

// Flat, heap-owned mirror of sh::InterfaceBlock. Created by STCopyInterfaceBlock, released by
// STDestroyInterfaceBlock.
typedef struct ST_InterfaceBlock {
    char* name;
    int layout;
    bool isRowMajorLayout;
    int binding;

    uint32_t fieldsCount;
    ST_ShaderVariable* pFields;
} ST_InterfaceBlock;

typedef struct ST_NameHashingMap {
    uint32_t entryCount;
    const char** ppUserNames;
    const char** ppCompiledNames;
} ST_NameHashingMap;

typedef struct ST_ShaderCompileInfo {
    ST_Handle handle;
    GLenum shaderType;
    ST_ShaderSpec spec;
    ST_ShaderOutput output;
    uint32_t compileOptions;
    ST_BuiltInResources* resources;
    const char* shaderSource;
} ST_ShaderCompileInfo;

// Owned by the dylib; released via STFreeShaderResolveState. infoLog/translatedSource and all
// pointee arrays are valid until that call.
typedef struct ST_ShaderCompileResult {
    ST_Handle outputHandle;
    char* infoLog;
    char* translatedSource;
    int compileStatus;

    ST_NameHashingMap* nameHashingMap;

    uint32_t uniformsCount;
    ST_ShaderVariable* pUniforms;
    uint32_t inputVaryingsCount;
    ST_ShaderVariable* pInputVaryings;
    uint32_t outputVaryingsCount;
    ST_ShaderVariable* pOutputVaryings;
    uint32_t allAttributesCount;
    ST_ShaderVariable* pAllAttributes;
    uint32_t activeOutputVariablesCount;
    ST_ShaderVariable* pActiveOutputVariables;
    uint32_t uniformBlocksCount;
    ST_InterfaceBlock* pUniformBlocks;
} ST_ShaderCompileResult;

// Exported symbols, resolved via dlsym/findSymbol by angle_shader_parser.cpp. Signatures must
// match the *_t typedefs below exactly.
bool STInitialize(void);
bool STFinalize(void);
void STGenerateResources(ST_BuiltInResources* outResources);
void STCompileAndResolve(const ST_ShaderCompileInfo* info, ST_ShaderCompileResult** outResult);
void STFreeShaderResolveState(ST_ShaderCompileResult* result);
ST_ShaderVariable STCopyVariable(const ST_ShaderVariable* var);
ST_InterfaceBlock STCopyInterfaceBlock(const ST_InterfaceBlock* block);
void STDestroyVariable(ST_ShaderVariable* var);
void STDestroyInterfaceBlock(ST_InterfaceBlock* block);

#ifdef __cplusplus
}  // extern "C"
#endif

typedef bool (*STInitialize_t)(void);
typedef bool (*STFinalize_t)(void);
typedef void (*STGenerateResources_t)(ST_BuiltInResources*);
typedef void (*STCompileAndResolve_t)(const ST_ShaderCompileInfo*, ST_ShaderCompileResult**);
typedef void (*STFreeShaderResolveState_t)(ST_ShaderCompileResult*);
typedef ST_ShaderVariable (*STCopyVariable_t)(const ST_ShaderVariable*);
typedef ST_InterfaceBlock (*STCopyInterfaceBlock_t)(const ST_InterfaceBlock*);
typedef void (*STDestroyVariable_t)(ST_ShaderVariable*);
typedef void (*STDestroyInterfaceBlock_t)(ST_InterfaceBlock*);

typedef struct STDispatch {
    STInitialize_t initialize;
    STFinalize_t finalize;
    STGenerateResources_t generateResources;
    STCompileAndResolve_t compileAndResolve;
    STFreeShaderResolveState_t freeShaderResolveState;
    STCopyVariable_t copyVariable;
    STCopyInterfaceBlock_t copyInterfaceBlock;
    STDestroyVariable_t destroyVariable;
    STDestroyInterfaceBlock_t destroyInterfaceBlock;
} STDispatch;
