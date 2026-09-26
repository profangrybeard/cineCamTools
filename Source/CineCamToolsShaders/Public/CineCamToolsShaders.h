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

	// Histogram and waveform panels on or off. Off compiles a panel out, so its buffers needn't be bound.
	class FHistogramDim : SHADER_PERMUTATION_BOOL("VALUE_SCOPE_HISTOGRAM");
	class FWaveformDim : SHADER_PERMUTATION_BOOL("VALUE_SCOPE_WAVEFORM");
	using FPermutationDomain = TShaderPermutationDomain<FHistogramDim, FWaveformDim>;

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
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, HistogramIn)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, HistogramMax)
		SHADER_PARAMETER(FVector4f, HistogramRect)   // panel min x, min y, width, height, in view pixels
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, WaveformIn)
		SHADER_PARAMETER(FVector4f, WaveformRect)    // panel min x, min y, width, height, in view pixels
		SHADER_PARAMETER(float, WaveformRefCount)    // a cell with this many pixels draws at 63% brightness
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
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

// Luma waveform: image columns across, levels 0 to 255 up, each cell counts pixels.
// Buffer layout is Column * 256 + Level.
class CINECAMTOOLSSHADERS_API FValueScopeWaveformCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FValueScopeWaveformCS);
	SHADER_USE_PARAMETER_STRUCT(FValueScopeWaveformCS, FGlobalShader);

	static constexpr int32 GroupSize = 16;
	static constexpr int32 NumColumns = 512;
	static constexpr int32 NumLevels = 256;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, WaveformOut)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), GroupSize);
		OutEnvironment.SetDefine(TEXT("WAVEFORM_COLUMNS"), NumColumns);
	}
};

// Spot meter: sums LumaLevel() over a square box around each point. One group per point.
// Output layout is Point * 2 + 0 = sum of levels, Point * 2 + 1 = pixels counted.
class CINECAMTOOLSSHADERS_API FValueScopeSpotMeterCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FValueScopeSpotMeterCS);
	SHADER_USE_PARAMETER_STRUCT(FValueScopeSpotMeterCS, FGlobalShader);

	// The live point plus up to 4 pins (step 4.3).
	static constexpr int32 MaxPoints = 5;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_ARRAY(FVector4f, SpotPoints, [MaxPoints]) // xy = position in the view, 0 to 1
		SHADER_PARAMETER(int32, SpotHalfSize)                      // box is 2 * this + 1 input pixels wide
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, SpotOut)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SPOT_MAX_POINTS"), MaxPoints);
	}
};

// Tallest histogram bin in levels 1 to 254, the panel's height reference. One group of 256 threads.
class CINECAMTOOLSSHADERS_API FValueScopeHistogramMaxCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FValueScopeHistogramMaxCS);
	SHADER_USE_PARAMETER_STRUCT(FValueScopeHistogramMaxCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, HistogramIn)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, HistogramMaxOut)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
