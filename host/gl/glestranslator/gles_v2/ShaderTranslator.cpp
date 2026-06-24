// ShaderTranslator.cpp: implementation of the ST_* C-ABI declared in ShaderTranslator.h, wrapping
// ANGLE's real public API (sh::* in GLSLANG/ShaderLang.h). Built into a standalone shared library
// (libshadertranslator.{dylib,so,dll}) linked against ANGLE's `translator` static lib; never
// linked into the main gfxstream binary.

#include "ShaderTranslator.h"

#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include <GLSLANG/ShaderLang.h>

namespace {

ShShaderSpec convertSpec(ST_ShaderSpec spec) {
    switch (spec) {
        case ST_GLES2_SPEC:
            return SH_GLES2_SPEC;
        case ST_GLES3_SPEC:
            return SH_GLES3_SPEC;
        case ST_GLES3_1_SPEC:
            return SH_GLES3_1_SPEC;
    }
    return SH_GLES3_1_SPEC;
}

ShShaderOutput convertOutput(ST_ShaderOutput output) {
    switch (output) {
        case ST_GLSL_COMPATIBILITY_OUTPUT:
            return SH_GLSL_COMPATIBILITY_OUTPUT;
        case ST_GLSL_150_CORE_OUTPUT:
            return SH_GLSL_150_CORE_OUTPUT;
        case ST_GLSL_330_CORE_OUTPUT:
            return SH_GLSL_330_CORE_OUTPUT;
        case ST_GLSL_430_CORE_OUTPUT:
            return SH_GLSL_430_CORE_OUTPUT;
    }
    return SH_GLSL_430_CORE_OUTPUT;
}

// Overrides the subset of fields gfxstream actually edits; everything else keeps whatever
// sh::InitBuiltInResources() already set as ANGLE's own defaults.
void applyResourceOverrides(const ST_BuiltInResources& in, ShBuiltInResources* out) {
    out->MaxVertexAttribs = in.MaxVertexAttribs;
    out->MaxVertexUniformVectors = in.MaxVertexUniformVectors;
    out->MaxVaryingVectors = in.MaxVaryingVectors;
    out->MaxVertexTextureImageUnits = in.MaxVertexTextureImageUnits;
    out->MaxCombinedTextureImageUnits = in.MaxCombinedTextureImageUnits;
    out->MaxTextureImageUnits = in.MaxTextureImageUnits;
    out->MaxFragmentUniformVectors = in.MaxFragmentUniformVectors;
    out->MaxDrawBuffers = in.MaxDrawBuffers;

    out->OES_standard_derivatives = in.OES_standard_derivatives;
    out->OES_EGL_image_external = in.OES_EGL_image_external;
    out->OES_EGL_image_external_essl3 = in.OES_EGL_image_external_essl3;
    out->NV_EGL_stream_consumer_external = in.NV_EGL_stream_consumer_external;
    out->ARB_texture_rectangle = in.ARB_texture_rectangle;
    out->EXT_blend_func_extended = in.EXT_blend_func_extended;
    out->EXT_conservative_depth = in.EXT_conservative_depth;
    out->EXT_draw_buffers = in.EXT_draw_buffers;
    out->EXT_frag_depth = in.EXT_frag_depth;
    out->EXT_shader_texture_lod = in.EXT_shader_texture_lod;
    out->EXT_shader_framebuffer_fetch = in.EXT_shader_framebuffer_fetch;
    out->EXT_shader_framebuffer_fetch_non_coherent = in.EXT_shader_framebuffer_fetch_non_coherent;
    out->NV_shader_framebuffer_fetch = in.NV_shader_framebuffer_fetch;
    out->NV_shader_noperspective_interpolation = in.NV_shader_noperspective_interpolation;
    out->ARM_shader_framebuffer_fetch = in.ARM_shader_framebuffer_fetch;
    out->ARM_shader_framebuffer_fetch_depth_stencil = in.ARM_shader_framebuffer_fetch_depth_stencil;
    out->OVR_multiview = in.OVR_multiview;
    out->OVR_multiview2 = in.OVR_multiview2;
    out->EXT_multisampled_render_to_texture = in.EXT_multisampled_render_to_texture;
    out->EXT_multisampled_render_to_texture2 = in.EXT_multisampled_render_to_texture2;
    out->EXT_YUV_target = in.EXT_YUV_target;
    out->EXT_geometry_shader = in.EXT_geometry_shader;
    out->OES_geometry_shader = in.OES_geometry_shader;
    out->OES_shader_io_blocks = in.OES_shader_io_blocks;
    out->EXT_shader_io_blocks = in.EXT_shader_io_blocks;
    out->EXT_gpu_shader5 = in.EXT_gpu_shader5;
    out->OES_gpu_shader5 = in.OES_gpu_shader5;
    out->EXT_shader_non_constant_global_initializers = in.EXT_shader_non_constant_global_initializers;
    out->OES_texture_storage_multisample_2d_array = in.OES_texture_storage_multisample_2d_array;
    out->OES_texture_3D = in.OES_texture_3D;
    out->ANGLE_shader_pixel_local_storage = in.ANGLE_shader_pixel_local_storage;
    out->ANGLE_texture_multisample = in.ANGLE_texture_multisample;
    out->ANGLE_multi_draw = in.ANGLE_multi_draw;
    out->ANGLE_base_vertex_base_instance = in.ANGLE_base_vertex_base_instance;
    out->WEBGL_video_texture = in.WEBGL_video_texture;
    out->APPLE_clip_distance = in.APPLE_clip_distance;
    out->OES_texture_cube_map_array = in.OES_texture_cube_map_array;
    out->EXT_texture_cube_map_array = in.EXT_texture_cube_map_array;
    out->EXT_texture_query_lod = in.EXT_texture_query_lod;
    out->EXT_texture_shadow_lod = in.EXT_texture_shadow_lod;
    out->EXT_shadow_samplers = in.EXT_shadow_samplers;
    out->OES_shader_multisample_interpolation = in.OES_shader_multisample_interpolation;
    out->OES_shader_image_atomic = in.OES_shader_image_atomic;
    out->EXT_tessellation_shader = in.EXT_tessellation_shader;
    out->OES_tessellation_shader = in.OES_tessellation_shader;
    out->OES_texture_buffer = in.OES_texture_buffer;
    out->EXT_texture_buffer = in.EXT_texture_buffer;
    out->OES_sample_variables = in.OES_sample_variables;
    out->EXT_clip_cull_distance = in.EXT_clip_cull_distance;
    out->ANGLE_clip_cull_distance = in.ANGLE_clip_cull_distance;
    out->EXT_primitive_bounding_box = in.EXT_primitive_bounding_box;
    out->OES_primitive_bounding_box = in.OES_primitive_bounding_box;
    out->EXT_separate_shader_objects = in.EXT_separate_shader_objects;
    out->ANGLE_base_vertex_base_instance_shader_builtin = in.ANGLE_base_vertex_base_instance_shader_builtin;
    out->ANDROID_extension_pack_es31a = in.ANDROID_extension_pack_es31a;
    out->KHR_blend_equation_advanced = in.KHR_blend_equation_advanced;

    out->NV_draw_buffers = in.NV_draw_buffers;
    out->FragmentPrecisionHigh = in.FragmentPrecisionHigh;

    out->MaxVertexOutputVectors = in.MaxVertexOutputVectors;
    out->MaxFragmentInputVectors = in.MaxFragmentInputVectors;
    out->MinProgramTexelOffset = in.MinProgramTexelOffset;
    out->MaxProgramTexelOffset = in.MaxProgramTexelOffset;

    out->MaxDualSourceDrawBuffers = in.MaxDualSourceDrawBuffers;

    out->MaxProgramTextureGatherOffset = in.MaxProgramTextureGatherOffset;
    out->MinProgramTextureGatherOffset = in.MinProgramTextureGatherOffset;
    out->MaxImageUnits = in.MaxImageUnits;
    out->MaxComputeImageUniforms = in.MaxComputeImageUniforms;
    out->MaxVertexImageUniforms = in.MaxVertexImageUniforms;
    out->MaxFragmentImageUniforms = in.MaxFragmentImageUniforms;
    out->MaxCombinedImageUniforms = in.MaxCombinedImageUniforms;
    out->MaxCombinedShaderOutputResources = in.MaxCombinedShaderOutputResources;
    out->MaxUniformLocations = in.MaxUniformLocations;
    for (int i = 0; i < 3; ++i) {
        out->MaxComputeWorkGroupCount[i] = in.MaxComputeWorkGroupCount[i];
        out->MaxComputeWorkGroupSize[i] = in.MaxComputeWorkGroupSize[i];
    }
    out->MaxComputeUniformComponents = in.MaxComputeUniformComponents;
    out->MaxComputeTextureImageUnits = in.MaxComputeTextureImageUnits;
    out->MaxComputeAtomicCounters = in.MaxComputeAtomicCounters;
    out->MaxComputeAtomicCounterBuffers = in.MaxComputeAtomicCounterBuffers;
    out->MaxVertexAtomicCounters = in.MaxVertexAtomicCounters;
    out->MaxFragmentAtomicCounters = in.MaxFragmentAtomicCounters;
    out->MaxCombinedAtomicCounters = in.MaxCombinedAtomicCounters;
    out->MaxAtomicCounterBindings = in.MaxAtomicCounterBindings;
    out->MaxVertexAtomicCounterBuffers = in.MaxVertexAtomicCounterBuffers;
    out->MaxFragmentAtomicCounterBuffers = in.MaxFragmentAtomicCounterBuffers;
    out->MaxCombinedAtomicCounterBuffers = in.MaxCombinedAtomicCounterBuffers;
    out->MaxAtomicCounterBufferSize = in.MaxAtomicCounterBufferSize;
    out->MaxUniformBufferBindings = in.MaxUniformBufferBindings;
    out->MaxShaderStorageBufferBindings = in.MaxShaderStorageBufferBindings;
}

void copyResourcesFromSh(const ShBuiltInResources& in, ST_BuiltInResources* out) {
    memset(out, 0, sizeof(*out));
    out->MaxVertexAttribs = in.MaxVertexAttribs;
    out->MaxVertexUniformVectors = in.MaxVertexUniformVectors;
    out->MaxVaryingVectors = in.MaxVaryingVectors;
    out->MaxVertexTextureImageUnits = in.MaxVertexTextureImageUnits;
    out->MaxCombinedTextureImageUnits = in.MaxCombinedTextureImageUnits;
    out->MaxTextureImageUnits = in.MaxTextureImageUnits;
    out->MaxFragmentUniformVectors = in.MaxFragmentUniformVectors;
    out->MaxDrawBuffers = in.MaxDrawBuffers;
    out->FragmentPrecisionHigh = in.FragmentPrecisionHigh;
    out->MaxVertexOutputVectors = in.MaxVertexOutputVectors;
    out->MaxFragmentInputVectors = in.MaxFragmentInputVectors;
    out->MinProgramTexelOffset = in.MinProgramTexelOffset;
    out->MaxProgramTexelOffset = in.MaxProgramTexelOffset;
    out->MaxDualSourceDrawBuffers = in.MaxDualSourceDrawBuffers;
    out->OES_standard_derivatives = in.OES_standard_derivatives;
    out->OES_EGL_image_external = in.OES_EGL_image_external;
    out->EXT_gpu_shader5 = in.EXT_gpu_shader5;
    out->EXT_shader_framebuffer_fetch = in.EXT_shader_framebuffer_fetch;
    out->MaxProgramTextureGatherOffset = in.MaxProgramTextureGatherOffset;
    out->MinProgramTextureGatherOffset = in.MinProgramTextureGatherOffset;
    out->MaxImageUnits = in.MaxImageUnits;
    out->MaxComputeImageUniforms = in.MaxComputeImageUniforms;
    out->MaxVertexImageUniforms = in.MaxVertexImageUniforms;
    out->MaxFragmentImageUniforms = in.MaxFragmentImageUniforms;
    out->MaxCombinedImageUniforms = in.MaxCombinedImageUniforms;
    out->MaxCombinedShaderOutputResources = in.MaxCombinedShaderOutputResources;
    out->MaxUniformLocations = in.MaxUniformLocations;
    for (int i = 0; i < 3; ++i) {
        out->MaxComputeWorkGroupCount[i] = in.MaxComputeWorkGroupCount[i];
        out->MaxComputeWorkGroupSize[i] = in.MaxComputeWorkGroupSize[i];
    }
    out->MaxComputeUniformComponents = in.MaxComputeUniformComponents;
    out->MaxComputeTextureImageUnits = in.MaxComputeTextureImageUnits;
    out->MaxComputeAtomicCounters = in.MaxComputeAtomicCounters;
    out->MaxComputeAtomicCounterBuffers = in.MaxComputeAtomicCounterBuffers;
    out->MaxVertexAtomicCounters = in.MaxVertexAtomicCounters;
    out->MaxFragmentAtomicCounters = in.MaxFragmentAtomicCounters;
    out->MaxCombinedAtomicCounters = in.MaxCombinedAtomicCounters;
    out->MaxAtomicCounterBindings = in.MaxAtomicCounterBindings;
    out->MaxVertexAtomicCounterBuffers = in.MaxVertexAtomicCounterBuffers;
    out->MaxFragmentAtomicCounterBuffers = in.MaxFragmentAtomicCounterBuffers;
    out->MaxCombinedAtomicCounterBuffers = in.MaxCombinedAtomicCounterBuffers;
    out->MaxAtomicCounterBufferSize = in.MaxAtomicCounterBufferSize;
    out->MaxUniformBufferBindings = in.MaxUniformBufferBindings;
    out->MaxShaderStorageBufferBindings = in.MaxShaderStorageBufferBindings;
}

ST_ShaderVariable convertVariable(const sh::ShaderVariable& var) {
    ST_ShaderVariable out;
    memset(&out, 0, sizeof(out));
    out.type = var.type;
    out.precision = var.precision;
    out.name = strdup(var.name.c_str());
    out.location = var.location;
    out.staticUse = var.staticUse;
    out.isRowMajorLayout = var.isRowMajorLayout;

    out.arraySizeCount = static_cast<uint32_t>(var.arraySizes.size());
    if (out.arraySizeCount) {
        out.pArraySizes = static_cast<unsigned int*>(malloc(sizeof(unsigned int) * out.arraySizeCount));
        for (uint32_t i = 0; i < out.arraySizeCount; ++i) {
            out.pArraySizes[i] = var.arraySizes[i];
        }
    }

    out.fieldsCount = static_cast<uint32_t>(var.fields.size());
    if (out.fieldsCount) {
        out.pFields = static_cast<ST_ShaderVariable*>(
            malloc(sizeof(ST_ShaderVariable) * out.fieldsCount));
        for (uint32_t i = 0; i < out.fieldsCount; ++i) {
            out.pFields[i] = convertVariable(var.fields[i]);
        }
    }
    return out;
}

ST_InterfaceBlock convertInterfaceBlock(const sh::InterfaceBlock& block) {
    ST_InterfaceBlock out;
    memset(&out, 0, sizeof(out));
    out.name = strdup(block.name.c_str());
    out.layout = static_cast<int>(block.layout);
    out.isRowMajorLayout = block.isRowMajorLayout;
    out.binding = block.binding;

    out.fieldsCount = static_cast<uint32_t>(block.fields.size());
    if (out.fieldsCount) {
        out.pFields = static_cast<ST_ShaderVariable*>(
            malloc(sizeof(ST_ShaderVariable) * out.fieldsCount));
        for (uint32_t i = 0; i < out.fieldsCount; ++i) {
            out.pFields[i] = convertVariable(block.fields[i]);
        }
    }
    return out;
}

template <class T, class ConvertFn>
void convertVectorToArray(const std::vector<T>& in, uint32_t* outCount, ST_ShaderVariable** outArr,
                           ConvertFn convert) {
    *outCount = static_cast<uint32_t>(in.size());
    if (*outCount == 0) {
        *outArr = nullptr;
        return;
    }
    *outArr = static_cast<ST_ShaderVariable*>(malloc(sizeof(ST_ShaderVariable) * (*outCount)));
    for (uint32_t i = 0; i < *outCount; ++i) {
        (*outArr)[i] = convert(in[i]);
    }
}

void destroyVariableContents(ST_ShaderVariable* var) {
    if (!var) return;
    free(var->name);
    free(var->pArraySizes);
    if (var->pFields) {
        for (uint32_t i = 0; i < var->fieldsCount; ++i) {
            destroyVariableContents(&var->pFields[i]);
        }
        free(var->pFields);
    }
    memset(var, 0, sizeof(*var));
}

void destroyVariableArray(ST_ShaderVariable* arr, uint32_t count) {
    if (!arr) return;
    for (uint32_t i = 0; i < count; ++i) {
        destroyVariableContents(&arr[i]);
    }
    free(arr);
}

}  // namespace

extern "C" {

bool STInitialize(void) { return sh::Initialize(); }

bool STFinalize(void) { return sh::Finalize(); }

void STGenerateResources(ST_BuiltInResources* outResources) {
    ShBuiltInResources shRes;
    sh::InitBuiltInResources(&shRes);
    copyResourcesFromSh(shRes, outResources);
}

void STCompileAndResolve(const ST_ShaderCompileInfo* info, ST_ShaderCompileResult** outResult) {
    ShHandle handle = static_cast<ShHandle>(info->handle);

    if (!handle) {
        ShBuiltInResources shRes;
        sh::InitBuiltInResources(&shRes);
        if (info->resources) {
            applyResourceOverrides(*info->resources, &shRes);
        }
        handle = sh::ConstructCompiler(info->shaderType, convertSpec(info->spec),
                                        convertOutput(info->output), &shRes);
    }

    ShCompileOptions opts;
    opts.objectCode = (info->compileOptions & ST_OBJECT_CODE) ? 1 : 0;
    opts.initOutputVariables = 0;

    const char* shaderStrings[] = {info->shaderSource};
    bool ok = handle && sh::Compile(handle, shaderStrings, 1, opts);

    ST_ShaderCompileResult* result = new ST_ShaderCompileResult();
    memset(result, 0, sizeof(*result));
    result->outputHandle = static_cast<ST_Handle>(handle);
    result->compileStatus = ok ? 1 : 0;

    if (handle) {
        result->infoLog = strdup(sh::GetInfoLog(handle).c_str());
        result->translatedSource = strdup(sh::GetObjectCode(handle).c_str());

        const std::map<std::string, std::string>* nameMap = sh::GetNameHashingMap(handle);
        ST_NameHashingMap* outMap = new ST_NameHashingMap();
        memset(outMap, 0, sizeof(*outMap));
        if (nameMap && !nameMap->empty()) {
            outMap->entryCount = static_cast<uint32_t>(nameMap->size());
            outMap->ppUserNames = static_cast<const char**>(
                malloc(sizeof(char*) * outMap->entryCount));
            outMap->ppCompiledNames = static_cast<const char**>(
                malloc(sizeof(char*) * outMap->entryCount));
            uint32_t i = 0;
            for (const auto& entry : *nameMap) {
                outMap->ppUserNames[i] = strdup(entry.first.c_str());
                outMap->ppCompiledNames[i] = strdup(entry.second.c_str());
                ++i;
            }
        }
        result->nameHashingMap = outMap;

        if (ok && (info->compileOptions & ST_VARIABLES)) {
            auto convert = [](const sh::ShaderVariable& v) { return convertVariable(v); };
            if (auto* uniforms = sh::GetUniforms(handle)) {
                convertVectorToArray(*uniforms, &result->uniformsCount, &result->pUniforms, convert);
            }
            if (auto* inputVaryings = sh::GetInputVaryings(handle)) {
                convertVectorToArray(*inputVaryings, &result->inputVaryingsCount,
                                      &result->pInputVaryings, convert);
            }
            if (auto* outputVaryings = sh::GetOutputVaryings(handle)) {
                convertVectorToArray(*outputVaryings, &result->outputVaryingsCount,
                                      &result->pOutputVaryings, convert);
            }
            if (auto* attributes = sh::GetAttributes(handle)) {
                convertVectorToArray(*attributes, &result->allAttributesCount,
                                      &result->pAllAttributes, convert);
            }
            if (auto* outputVars = sh::GetOutputVariables(handle)) {
                convertVectorToArray(*outputVars, &result->activeOutputVariablesCount,
                                      &result->pActiveOutputVariables, convert);
            }
            if (auto* uniformBlocks = sh::GetUniformBlocks(handle)) {
                result->uniformBlocksCount = static_cast<uint32_t>(uniformBlocks->size());
                if (result->uniformBlocksCount) {
                    result->pUniformBlocks = static_cast<ST_InterfaceBlock*>(
                        malloc(sizeof(ST_InterfaceBlock) * result->uniformBlocksCount));
                    for (uint32_t i = 0; i < result->uniformBlocksCount; ++i) {
                        result->pUniformBlocks[i] = convertInterfaceBlock((*uniformBlocks)[i]);
                    }
                }
            }
        }
    } else {
        result->infoLog = strdup("ShaderTranslator: failed to construct ANGLE compiler");
        result->translatedSource = strdup("");
        result->nameHashingMap = new ST_NameHashingMap();
        memset(result->nameHashingMap, 0, sizeof(*result->nameHashingMap));
    }

    *outResult = result;
}

void STFreeShaderResolveState(ST_ShaderCompileResult* result) {
    if (!result) return;

    free(result->infoLog);
    free(result->translatedSource);

    if (result->nameHashingMap) {
        for (uint32_t i = 0; i < result->nameHashingMap->entryCount; ++i) {
            free(const_cast<char*>(result->nameHashingMap->ppUserNames[i]));
            free(const_cast<char*>(result->nameHashingMap->ppCompiledNames[i]));
        }
        free(result->nameHashingMap->ppUserNames);
        free(result->nameHashingMap->ppCompiledNames);
        delete result->nameHashingMap;
    }

    destroyVariableArray(result->pUniforms, result->uniformsCount);
    destroyVariableArray(result->pInputVaryings, result->inputVaryingsCount);
    destroyVariableArray(result->pOutputVaryings, result->outputVaryingsCount);
    destroyVariableArray(result->pAllAttributes, result->allAttributesCount);
    destroyVariableArray(result->pActiveOutputVariables, result->activeOutputVariablesCount);

    if (result->pUniformBlocks) {
        for (uint32_t i = 0; i < result->uniformBlocksCount; ++i) {
            ST_InterfaceBlock* block = &result->pUniformBlocks[i];
            free(block->name);
            destroyVariableArray(block->pFields, block->fieldsCount);
        }
        free(result->pUniformBlocks);
    }

    delete result;
}

ST_ShaderVariable STCopyVariable(const ST_ShaderVariable* var) {
    ST_ShaderVariable out;
    memset(&out, 0, sizeof(out));
    if (!var) return out;

    out.type = var->type;
    out.precision = var->precision;
    out.name = var->name ? strdup(var->name) : nullptr;
    out.location = var->location;
    out.staticUse = var->staticUse;
    out.isRowMajorLayout = var->isRowMajorLayout;

    out.arraySizeCount = var->arraySizeCount;
    if (out.arraySizeCount) {
        out.pArraySizes = static_cast<unsigned int*>(malloc(sizeof(unsigned int) * out.arraySizeCount));
        memcpy(out.pArraySizes, var->pArraySizes, sizeof(unsigned int) * out.arraySizeCount);
    }

    out.fieldsCount = var->fieldsCount;
    if (out.fieldsCount) {
        out.pFields = static_cast<ST_ShaderVariable*>(
            malloc(sizeof(ST_ShaderVariable) * out.fieldsCount));
        for (uint32_t i = 0; i < out.fieldsCount; ++i) {
            out.pFields[i] = STCopyVariable(&var->pFields[i]);
        }
    }
    return out;
}

ST_InterfaceBlock STCopyInterfaceBlock(const ST_InterfaceBlock* block) {
    ST_InterfaceBlock out;
    memset(&out, 0, sizeof(out));
    if (!block) return out;

    out.name = block->name ? strdup(block->name) : nullptr;
    out.layout = block->layout;
    out.isRowMajorLayout = block->isRowMajorLayout;
    out.binding = block->binding;

    out.fieldsCount = block->fieldsCount;
    if (out.fieldsCount) {
        out.pFields = static_cast<ST_ShaderVariable*>(
            malloc(sizeof(ST_ShaderVariable) * out.fieldsCount));
        for (uint32_t i = 0; i < out.fieldsCount; ++i) {
            out.pFields[i] = STCopyVariable(&block->pFields[i]);
        }
    }
    return out;
}

void STDestroyVariable(ST_ShaderVariable* var) { destroyVariableContents(var); }

void STDestroyInterfaceBlock(ST_InterfaceBlock* block) {
    if (!block) return;
    free(block->name);
    destroyVariableArray(block->pFields, block->fieldsCount);
    block->name = nullptr;
    block->pFields = nullptr;
    block->fieldsCount = 0;
}

}  // extern "C"
