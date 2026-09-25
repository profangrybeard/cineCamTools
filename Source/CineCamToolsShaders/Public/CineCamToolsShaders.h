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
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
