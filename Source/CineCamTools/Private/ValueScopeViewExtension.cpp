#include "ValueScopeViewExtension.h"

#include "CineCamToolsShaders.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "ScreenPass.h"
#include "SceneView.h"

static TAutoConsoleVariable<int32> CVarValueScopeMode(
	TEXT("r.ValueScope.Mode"),
	-1,
	TEXT("-1: use the Value Scope component on the view target (default)\n")
	TEXT(" 0: force off everywhere\n")
	TEXT(" 1: force plumbing check on every view (magenta frame)\n")
	TEXT(" 2: force notan on every view\n")
	TEXT("Use 1 or 2 in the editor viewport, where the view target may not be the camera."),
	ECVF_Default);

FValueScopeViewExtension::FValueScopeViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FValueScopeViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	FValueScopeSettings Settings;
	bool bActive = false;

	if (const AActor* ViewActor = InView.ViewActor)
	{
		if (const UValueScopeComponent* Scope = ViewActor->FindComponentByClass<UValueScopeComponent>())
		{
			if (Scope->bEnabled)
			{
				Settings = Scope->Settings;
				bActive = true;
			}
		}
	}

	const int32 Forced = CVarValueScopeMode.GetValueOnGameThread();
	if (Forced >= 0)
	{
		if (!bActive)
		{
			Settings = GetDefault<UValueScopeComponent>()->Settings;
		}
		Settings.Mode = static_cast<EValueScopeMode>(FMath::Clamp(Forced, 0, 2));
		bActive = true;
	}

	FScopeLock Lock(&PendingLock);
	if (bActive && Settings.Mode != EValueScopeMode::Off)
	{
		Pending.Add(&InView, Settings);
	}
	else
	{
		Pending.Remove(&InView);
	}
}

void FValueScopeViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass != EPostProcessingPass::Tonemap || !bIsPassEnabled)
	{
		return;
	}

	FValueScopeSettings Settings;
	{
		FScopeLock Lock(&PendingLock);
		if (!Pending.RemoveAndCopyValue(&InView, Settings))
		{
			return;
		}
	}

	InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(
		this, &FValueScopeViewExtension::AfterTonemap_RenderThread, Settings));
}

FScreenPassTexture FValueScopeViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder,
	const FSceneView& View, const FPostProcessMaterialInputs& Inputs, FValueScopeSettings Settings)
{
	const FScreenPassTexture SceneColor = FScreenPassTexture::CopyFromSlice(
		GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

	if (!SceneColor.IsValid())
	{
		return SceneColor;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "ValueScope");

	// If Tonemap is the last pass, the engine hands us the backbuffer and we MUST write to it.
	FScreenPassRenderTarget Output = Inputs.OverrideOutput;
	if (!Output.IsValid())
	{
		FRDGTextureDesc Desc = SceneColor.Texture->Desc;
		Desc.Flags |= TexCreate_RenderTargetable | TexCreate_ShaderResource;
		Desc.ClearValue = FClearValueBinding::Black;
		Output = FScreenPassRenderTarget(
			GraphBuilder.CreateTexture(Desc, TEXT("ValueScope.Output")),
			SceneColor.ViewRect,
			ERenderTargetLoadAction::ENoAction);
	}

	const FScreenPassTextureViewport InputViewport(SceneColor);
	const FScreenPassTextureViewport OutputViewport(Output);

	FValueScopePS::FParameters* Params = GraphBuilder.AllocParameters<FValueScopePS::FParameters>();
	Params->Input = GetScreenPassTextureViewportParameters(InputViewport);
	Params->Output = GetScreenPassTextureViewportParameters(OutputViewport);
	Params->InputTexture = SceneColor.Texture;
	Params->InputSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	Params->Mode = static_cast<int32>(Settings.Mode);
	Params->NotanThresholds = FVector2f(Settings.ShadowThreshold, Settings.HighlightThreshold);
	Params->RenderTargets[0] = Output.GetRenderTargetBinding();

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	TShaderMapRef<FValueScopePS> PixelShader(ShaderMap);

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		ShaderMap,
		RDG_EVENT_NAME("ValueScope (Mode %d)", Params->Mode),
		PixelShader,
		Params,
		Output.ViewRect);

	return FScreenPassTexture(Output);
}
