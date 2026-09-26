#include "CineCamToolsShaders.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

IMPLEMENT_GLOBAL_SHADER(FValueScopePS, "/Plugin/CineCamTools/Private/ValueScope.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FValueScopeHistogramCS, "/Plugin/CineCamTools/Private/ValueScope.usf", "HistogramCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FValueScopeWaveformCS, "/Plugin/CineCamTools/Private/ValueScope.usf", "WaveformCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FValueScopeHistogramMaxCS, "/Plugin/CineCamTools/Private/ValueScope.usf", "HistogramMaxCS", SF_Compute);

void FValueScopePS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("WAVEFORM_COLUMNS"), FValueScopeWaveformCS::NumColumns);
}

void FCineCamToolsShadersModule::StartupModule()
{
	// Must run at PostConfigInit, before the global shader map compiles.
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CineCamTools"));
	check(Plugin.IsValid());
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/CineCamTools"), FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
}

IMPLEMENT_MODULE(FCineCamToolsShadersModule, CineCamToolsShaders)
