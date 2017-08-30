/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2017 Baldur Karlsson
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

#include "spirv_compile.h"
#include "common/common.h"
#include "serialise/string_utils.h"
#include "spirv_common.h"

#undef min
#undef max

#include "3rdparty/glslang/SPIRV/GlslangToSpv.h"
#include "3rdparty/glslang/glslang/Public/ShaderLang.h"

static bool inited = false;

void InitSPIRVCompiler()
{
  if(!inited)
  {
    glslang::InitializeProcess();
    inited = true;
  }
}

void ShutdownSPIRVCompiler()
{
  if(inited)
  {
    glslang::FinalizeProcess();
  }
}

namespace
{
bool tobool(const rdctype::str &b)
{
  return b[0] == '1';
}

rdctype::str frombool(bool b)
{
  return b ? "1" : "0";
}

uint32_t touint(const rdctype::str &u)
{
  return strtoul(u.elems, NULL, 10);
}

rdctype::str fromuint(uint32_t u)
{
  return StringFormat::Fmt("%u", u);
}

rdctype::str tostr(const std::string &s)
{
  return s;
}

std::string fromstr(const rdctype::str &s)
{
  return s;
}
}

#define ENCODE(member, type)             \
  {                                      \
    #member, from##type(settings.member) \
  }
#define DECODE(member, type)         \
  if(f.first == #member)             \
  {                                  \
    ret.member = to##type(f.second); \
    continue;                        \
  }

ShaderCompileFlags EncodeSPIRVSettings(const SPIRVCompilationSettings &settings)
{
  ShaderCompileFlags ret;

  std::string mergedResourceSetBinding;
  merge(settings.resourceSetBinding, mergedResourceSetBinding, ',');

  ret.flags = {
      {"lang", fromuint(uint32_t(settings.lang))},
      {"stage", fromuint(uint32_t(settings.stage))},
      {"resourceSetBinding", mergedResourceSetBinding},
      ENCODE(entryPoint, str),
      ENCODE(sourceEntryPoint, str),
      ENCODE(definitionPreamble, str),
      ENCODE(bindingShifts.sampler, uint),
      ENCODE(bindingShifts.texture, uint),
      ENCODE(bindingShifts.image, uint),
      ENCODE(bindingShifts.ubo, uint),
      ENCODE(bindingShifts.ssbo, uint),
      ENCODE(bindingShifts.uav, uint),
      ENCODE(autoMapBindings, bool),
      ENCODE(autoMapLocations, bool),
      ENCODE(flattenUniformArrays, bool),
      ENCODE(noStorageFormat, bool),
      ENCODE(hlslOffsets, bool),
      ENCODE(useStorageBuffer, bool),
      ENCODE(hlslIoMapping, bool),
      ENCODE(relaxedErrors, bool),
      ENCODE(suppressWarnings, bool),
      ENCODE(keepUncalled, bool),
      ENCODE(spvVersion, uint),
      ENCODE(vulkanVersion, uint),
      ENCODE(openglVersion, uint),
  };

  return ret;
}

SPIRVCompilationSettings DecodeSPIRVSettings(const ShaderCompileFlags &flags)
{
  SPIRVCompilationSettings ret;

  for(const rdctype::pair<rdctype::str, rdctype::str> &f : flags.flags)
  {
    if(f.first == "lang")
    {
      ret.lang = (SPIRVSourceLanguage)atoi(f.second.c_str());
      continue;
    }
    else if(f.first == "stage")
    {
      ret.stage = (SPIRVShaderStage)atoi(f.second.c_str());
      continue;
    }
    else if(f.first == "resourceSetBinding")
    {
      std::string mergedResourceSetBinding = f.second;
      split(mergedResourceSetBinding, ret.resourceSetBinding, ',');
      continue;
    }

    DECODE(entryPoint, str);
    DECODE(sourceEntryPoint, str);
    DECODE(definitionPreamble, str);
    DECODE(bindingShifts.sampler, uint);
    DECODE(bindingShifts.texture, uint);
    DECODE(bindingShifts.image, uint);
    DECODE(bindingShifts.ubo, uint);
    DECODE(bindingShifts.ssbo, uint);
    DECODE(bindingShifts.uav, uint);
    DECODE(autoMapBindings, bool);
    DECODE(autoMapLocations, bool);
    DECODE(flattenUniformArrays, bool);
    DECODE(noStorageFormat, bool);
    DECODE(hlslOffsets, bool);
    DECODE(useStorageBuffer, bool);
    DECODE(hlslIoMapping, bool);
    DECODE(relaxedErrors, bool);
    DECODE(suppressWarnings, bool);
    DECODE(keepUncalled, bool);
    DECODE(spvVersion, uint);
    DECODE(vulkanVersion, uint);
    DECODE(openglVersion, uint);
  }

  return ret;
}

TBuiltInResource DefaultResources = {
    /*.maxLights =*/32,
    /*.maxClipPlanes =*/6,
    /*.maxTextureUnits =*/32,
    /*.maxTextureCoords =*/32,
    /*.maxVertexAttribs =*/64,
    /*.maxVertexUniformComponents =*/4096,
    /*.maxVaryingFloats =*/64,
    /*.maxVertexTextureImageUnits =*/32,
    /*.maxCombinedTextureImageUnits =*/80,
    /*.maxTextureImageUnits =*/32,
    /*.maxFragmentUniformComponents =*/4096,
    /*.maxDrawBuffers =*/32,
    /*.maxVertexUniformVectors =*/128,
    /*.maxVaryingVectors =*/8,
    /*.maxFragmentUniformVectors =*/16,
    /*.maxVertexOutputVectors =*/16,
    /*.maxFragmentInputVectors =*/15,
    /*.minProgramTexelOffset =*/-8,
    /*.maxProgramTexelOffset =*/7,
    /*.maxClipDistances =*/8,
    /*.maxComputeWorkGroupCountX =*/65535,
    /*.maxComputeWorkGroupCountY =*/65535,
    /*.maxComputeWorkGroupCountZ =*/65535,
    /*.maxComputeWorkGroupSizeX =*/1024,
    /*.maxComputeWorkGroupSizeY =*/1024,
    /*.maxComputeWorkGroupSizeZ =*/64,
    /*.maxComputeUniformComponents =*/1024,
    /*.maxComputeTextureImageUnits =*/16,
    /*.maxComputeImageUniforms =*/8,
    /*.maxComputeAtomicCounters =*/8,
    /*.maxComputeAtomicCounterBuffers =*/1,
    /*.maxVaryingComponents =*/60,
    /*.maxVertexOutputComponents =*/64,
    /*.maxGeometryInputComponents =*/64,
    /*.maxGeometryOutputComponents =*/128,
    /*.maxFragmentInputComponents =*/128,
    /*.maxImageUnits =*/8,
    /*.maxCombinedImageUnitsAndFragmentOutputs =*/8,
    /*.maxCombinedShaderOutputResources =*/8,
    /*.maxImageSamples =*/0,
    /*.maxVertexImageUniforms =*/0,
    /*.maxTessControlImageUniforms =*/0,
    /*.maxTessEvaluationImageUniforms =*/0,
    /*.maxGeometryImageUniforms =*/0,
    /*.maxFragmentImageUniforms =*/8,
    /*.maxCombinedImageUniforms =*/8,
    /*.maxGeometryTextureImageUnits =*/16,
    /*.maxGeometryOutputVertices =*/256,
    /*.maxGeometryTotalOutputComponents =*/1024,
    /*.maxGeometryUniformComponents =*/1024,
    /*.maxGeometryVaryingComponents =*/64,
    /*.maxTessControlInputComponents =*/128,
    /*.maxTessControlOutputComponents =*/128,
    /*.maxTessControlTextureImageUnits =*/16,
    /*.maxTessControlUniformComponents =*/1024,
    /*.maxTessControlTotalOutputComponents =*/4096,
    /*.maxTessEvaluationInputComponents =*/128,
    /*.maxTessEvaluationOutputComponents =*/128,
    /*.maxTessEvaluationTextureImageUnits =*/16,
    /*.maxTessEvaluationUniformComponents =*/1024,
    /*.maxTessPatchComponents =*/120,
    /*.maxPatchVertices =*/32,
    /*.maxTessGenLevel =*/64,
    /*.maxViewports =*/16,
    /*.maxVertexAtomicCounters =*/0,
    /*.maxTessControlAtomicCounters =*/0,
    /*.maxTessEvaluationAtomicCounters =*/0,
    /*.maxGeometryAtomicCounters =*/0,
    /*.maxFragmentAtomicCounters =*/8,
    /*.maxCombinedAtomicCounters =*/8,
    /*.maxAtomicCounterBindings =*/1,
    /*.maxVertexAtomicCounterBuffers =*/0,
    /*.maxTessControlAtomicCounterBuffers =*/0,
    /*.maxTessEvaluationAtomicCounterBuffers =*/0,
    /*.maxGeometryAtomicCounterBuffers =*/0,
    /*.maxFragmentAtomicCounterBuffers =*/1,
    /*.maxCombinedAtomicCounterBuffers =*/1,
    /*.maxAtomicCounterBufferSize =*/16384,
    /*.maxTransformFeedbackBuffers =*/4,
    /*.maxTransformFeedbackInterleavedComponents =*/64,
    /*.maxCullDistances =*/8,
    /*.maxCombinedClipAndCullDistances =*/8,
    /*.maxSamples =*/4,

    /*.limits*/
    {
        /*.limits.nonInductiveForLoops =*/1,
        /*.limits.whileLoops =*/1,
        /*.limits.doWhileLoops =*/1,
        /*.limits.generalUniformIndexing =*/1,
        /*.limits.generalAttributeMatrixVectorIndexing =*/1,
        /*.limits.generalVaryingIndexing =*/1,
        /*.limits.generalSamplerIndexing =*/1,
        /*.limits.generalVariableIndexing =*/1,
        /*.limits.generalConstantMatrixVectorIndexing =*/1,
    },
};

string CompileSPIRV(const SPIRVCompilationSettings &settings,
                    const std::vector<std::string> &sources, vector<uint32_t> &spirv)
{
  if(settings.stage == SPIRVShaderStage::Invalid)
    return "Invalid shader stage specified";

  string errors = "";

  const char **strs = new const char *[sources.size()];

  for(size_t i = 0; i < sources.size(); i++)
    strs[i] = sources[i].c_str();

  RDCCOMPILE_ASSERT((int)EShLangVertex == (int)SPIRVShaderStage::Vertex &&
                        (int)EShLangTessControl == (int)SPIRVShaderStage::TessControl &&
                        (int)EShLangTessEvaluation == (int)SPIRVShaderStage::TessEvaluation &&
                        (int)EShLangGeometry == (int)SPIRVShaderStage::Geometry &&
                        (int)EShLangCompute == (int)SPIRVShaderStage::Compute,
                    "Shader language enums don't match");

  {
    // these enums are matched
    EShLanguage lang = EShLanguage(settings.stage);

    glslang::TShader *shader = new glslang::TShader(lang);

    shader->setStrings(strs, (int)sources.size());

    if(!settings.entryPoint.empty())
      shader->setEntryPoint(settings.entryPoint.c_str());

    if(!settings.sourceEntryPoint.empty())
      shader->setSourceEntryPoint(settings.sourceEntryPoint.c_str());

    if(!settings.definitionPreamble.empty())
      shader->setPreamble(settings.definitionPreamble.c_str());

    if(!settings.resourceSetBinding.empty())
      shader->setResourceSetBinding(settings.resourceSetBinding);

    shader->setAutoMapBindings(settings.autoMapBindings);
    shader->setAutoMapLocations(settings.autoMapLocations);
    shader->setFlattenUniformArrays(settings.flattenUniformArrays);
    shader->setNoStorageFormat(settings.noStorageFormat);
    shader->setNoStorageFormat(settings.noStorageFormat);

    shader->setHlslIoMapping(settings.hlslIoMapping);

    if(settings.bindingShifts.sampler != ~0U)
      shader->setShiftSamplerBinding(settings.bindingShifts.sampler);
    if(settings.bindingShifts.texture != ~0U)
      shader->setShiftTextureBinding(settings.bindingShifts.texture);
    if(settings.bindingShifts.image != ~0U)
      shader->setShiftImageBinding(settings.bindingShifts.image);
    if(settings.bindingShifts.ubo != ~0U)
      shader->setShiftUboBinding(settings.bindingShifts.ubo);
    if(settings.bindingShifts.ssbo != ~0U)
      shader->setShiftSsboBinding(settings.bindingShifts.ssbo);
    if(settings.bindingShifts.uav != ~0U)
      shader->setShiftUavBinding(settings.bindingShifts.uav);

    EShMessages flags = EShMsgSpvRules;

    if(settings.lang == SPIRVSourceLanguage::VulkanGLSL)
    {
      flags = EShMessages(flags | EShMsgVulkanRules);
      shader->setEnvClient(glslang::EShClientVulkan, settings.vulkanVersion);
      shader->setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan,
                          settings.vulkanVersion);
    }

    if(settings.lang == SPIRVSourceLanguage::VulkanHLSL)
    {
      flags = EShMessages(flags | EShMsgVulkanRules | EShMsgReadHlsl);
      shader->setEnvClient(glslang::EShClientVulkan, settings.vulkanVersion);
      shader->setEnvInput(glslang::EShSourceHlsl, lang, glslang::EShClientVulkan,
                          settings.vulkanVersion);
    }

    if(settings.lang == SPIRVSourceLanguage::OpenGLGLSL)
    {
      shader->setEnvClient(glslang::EShClientOpenGL, settings.openglVersion);
      shader->setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientOpenGL,
                          settings.openglVersion);
    }

    shader->setEnvTarget(glslang::EshTargetSpv, settings.spvVersion);

    if(settings.relaxedErrors)
      flags = EShMessages(flags | EShMsgRelaxedErrors);
    if(settings.suppressWarnings)
      flags = EShMessages(flags | EShMsgSuppressWarnings);
    if(settings.keepUncalled)
      flags = EShMessages(flags | EShMsgKeepUncalled);
    if(settings.hlslOffsets)
      flags = EShMessages(flags | EShMsgHlslOffsets);
    // if(settings.useStorageBuffer)
    // shader->setUseStorageBuffer();

    bool success = shader->parse(&DefaultResources, 110, false, flags);

    if(!success)
    {
      errors = "Shader failed to compile:\n\n";
      errors += shader->getInfoLog();
      errors += "\n\n";
      errors += shader->getInfoDebugLog();
    }
    else
    {
      glslang::TProgram *program = new glslang::TProgram();

      program->addShader(shader);

      success = program->link(EShMsgDefault);

      if(!success)
      {
        errors = "Program failed to link:\n\n";
        errors += program->getInfoLog();
        errors += "\n\n";
        errors += program->getInfoDebugLog();
      }
      else
      {
        success = program->mapIO();

        if(!success)
        {
          errors = "Program failed to map IO:\n\n";
          errors += program->getInfoLog();
          errors += "\n\n";
          errors += program->getInfoDebugLog();
        }
        else
        {
          glslang::TIntermediate *intermediate = program->getIntermediate(lang);

          // if we successfully compiled and linked, we must have the stage we started with
          RDCASSERT(intermediate);

          glslang::GlslangToSpv(*intermediate, spirv);
        }
      }

      delete program;
    }

    delete shader;
  }

  delete[] strs;

  return errors;
}
