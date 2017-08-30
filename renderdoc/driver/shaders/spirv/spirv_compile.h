/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#undef min
#undef max

#include "3rdparty/glslang/SPIRV/spirv.hpp"
#include "3rdparty/glslang/glslang/Public/ShaderLang.h"
#include "api/replay/renderdoc_replay.h"

using std::string;
using std::vector;

enum class SPIRVShaderStage : uint32_t
{
  Vertex,
  TessControl,
  TessEvaluation,
  Geometry,
  Fragment,
  Compute,
  Invalid,
};

enum class SPIRVSourceLanguage : uint32_t
{
  Unknown,
  OpenGLGLSL,
  VulkanGLSL,
  VulkanHLSL,
};

struct SPIRVCompilationSettings
{
  SPIRVSourceLanguage lang = SPIRVSourceLanguage::Unknown;
  SPIRVShaderStage stage = SPIRVShaderStage::Invalid;
  std::string entryPoint;
  std::string sourceEntryPoint;
  std::vector<std::string> resourceSetBinding;
  std::string definitionPreamble;

  struct
  {
    uint32_t sampler = ~0U;
    uint32_t texture = ~0U;
    uint32_t image = ~0U;
    uint32_t ubo = ~0U;
    uint32_t ssbo = ~0U;
    uint32_t uav = ~0U;
  } bindingShifts;

  bool autoMapBindings = false;
  bool autoMapLocations = false;
  bool flattenUniformArrays = false;
  bool noStorageFormat = false;
  bool hlslOffsets = false;
  bool useStorageBuffer = false;
  bool hlslIoMapping = false;
  bool relaxedErrors = false;
  bool suppressWarnings = false;
  bool keepUncalled = false;
  uint32_t spvVersion = SPV_VERSION;
  uint32_t vulkanVersion = 0;
  uint32_t openglVersion = 0;
};

ShaderCompileFlags EncodeSPIRVSettings(const SPIRVCompilationSettings &settings);
SPIRVCompilationSettings DecodeSPIRVSettings(const ShaderCompileFlags &flags);

string CompileSPIRV(const SPIRVCompilationSettings &settings, const vector<string> &sources,
                    vector<uint32_t> &spirv);

void InitSPIRVCompiler();
void ShutdownSPIRVCompiler();
