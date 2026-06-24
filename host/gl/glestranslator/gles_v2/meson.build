# Copyright 2023 Android Open Source Project
# SPDX-License-Identifier: Apache-2.0

files_lib_glesv2_translator = files(
  'angle_shader_parser.cpp',
  'gles_v2_context.cpp',
  'gles_v2_imp.cpp',
  'gles_v2_validate.cpp',
  'program_data.cpp',
  'sampler_data.cpp',
  'shader_parser.cpp',
  'shader_validator.cpp',
  'transform_feedback_data.cpp',
)

lib_glesv2_translator = static_library(
  'glesv2_translator',
  files_lib_glesv2_translator,
  cpp_args: gfxstream_host_args,
  include_directories: [
    inc_common_base,
    inc_etc,
    inc_gfxstream_server,
    inc_gl_common,
    inc_gl_openglesdispatch,
    inc_host_common,
    inc_host_library,
    inc_host_decoder_common,
    inc_common_logging,
    inc_host_snapshot,
    inc_include,
    inc_opengl_headers,
    inc_stream_servers,
  ],
  link_with: [
    lib_common_base,
    lib_host_common,
    lib_host_decoder_common,
    lib_common_logging,
    lib_host_snapshot,
  ],
)

# ANGLE's shader translator is only specified as a Bazel target (third_party/angle's
# BUILD.angle.bazel) and gfxstream itself builds with Meson, so the translator + the ST_* C-ABI
# shim (ShaderTranslator.{h,cpp}, wrapping ANGLE's sh::* API) are built separately with Bazel and
# the resulting libshadertranslator.{dylib,so,dll} is dlopen()ed at runtime by
# angle_shader_parser.cpp -- it is never linked into the gfxstream binary itself. Dropped next to
# libgfxstream_backend so it's on the same DYLD_LIBRARY_PATH/LD_LIBRARY_PATH entry (see
# scripts/run-capy.sh).
if use_gles
  if host_machine.system() == 'darwin'
    shadertranslator_dylib_name = 'libshadertranslator.dylib'
  elif host_machine.system() == 'windows'
    shadertranslator_dylib_name = 'shadertranslator.dll'
  else
    shadertranslator_dylib_name = 'libshadertranslator.so'
  endif
  custom_target('shadertranslator_dylib',
    output: 'shadertranslator_dylib.stamp',
    command: [
      find_program('bash'), '-c',
      'cd "' + meson.project_source_root() + '" && ' +
      'if command -v bazelisk >/dev/null 2>&1; then BAZEL=bazelisk; else BAZEL=bazel; fi && ' +
      '"$BAZEL" build //host/gl/glestranslator/gles_v2:shadertranslator && ' +
      'cp -f "bazel-bin/host/gl/glestranslator/gles_v2/' + shadertranslator_dylib_name + '" "' +
      meson.global_build_root() + '/host/' + shadertranslator_dylib_name + '" && ' +
      'touch @OUTPUT@',
    ],
    build_by_default: true,
  )
endif
