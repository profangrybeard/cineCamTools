#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"

class FCineCamToolsShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};

// Value Scope overlay. One pixel shader, mode picked by a uniform.
class CINECAMTOOLSSHADERS_API FValueScopePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FValueScopePS);
	SHADER_USE_PARAMETER_STRUCT(FValueScopePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(int32, Mode)
		SHADER_PARAMETER(FVector2f, NotanThresholds) // x = shadow max, y = highlight min
		SHADER_PARAMETER(FVector2f, ClipLevels)      // x = black clip, y = white clip
		SHADER_PARAMETER(int32, ClipZebras)
		SHADER_PARAMETER(int32, ThirdsGuide)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

// Luma histogram of the post-tonemap image, 256 bins (levels 0 to 255).
// One thread group is 16 x 16 = 256 threads, so each thread owns one bin when merging.
class CINECAMTOOLSSHADERS_API FValueScopeHistogramCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FValueScopeHistogramCS);
	SHADER_USE_PARAMETER_STRUCT(FValueScopeHistogramCS, FGlobalShader);

	static constexpr int32 GroupSize = 16;
	static constexpr int32 NumBins = 256;
	static_assert(GroupSize * GroupSize == NumBins, "One thread per bin when merging group bins.");

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, HistogramOut)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), GroupSize);
	}
};
