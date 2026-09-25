#include "ValueScopeViewExtension.h"

#include "CanvasItem.h"
#include "CineCamToolsShaders.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "ScreenPass.h"
#include "SceneView.h"

#if WITH_EDITOR
#include "EditorSupportDelegates.h"
#endif

// Editor viewports that aren't in Realtime only repaint on input, so a cvar change
// would not show until the mouse moved. Ask every viewport to repaint instead.
static void OnValueScopeCVarChanged(IConsoleVariable* Var)
{
#if WITH_EDITOR
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
#endif
}

static TAutoConsoleVariable<int32> CVarValueScopeMode(
	TEXT("r.ValueScope.Mode"),
	-1,
	TEXT("-1: use the Value Scope component on the view target (default)\n")
	TEXT(" 0: force off everywhere\n")
	TEXT(" 1: force plumbing check on every view (magenta frame)\n")
	TEXT(" 2: force notan on every view\n")
	TEXT(" 3: force false color on every view\n")
	TEXT("Use 1 to 3 in the editor viewport, where the view target may not be the camera."),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarValueScopeZebras(
	TEXT("r.ValueScope.Zebras"),
	-1,
	TEXT("-1: use the Value Scope component (default)\n")
	TEXT(" 0: force clip zebras off\n")
	TEXT(" 1: force clip zebras on, on every view"),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarValueScopeThirds(
	TEXT("r.ValueScope.Thirds"),
	-1,
	TEXT("-1: use the Value Scope component (default)\n")
	TEXT(" 0: force the thirds guide off\n")
	TEXT(" 1: force the thirds guide on, on every view"),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarValueScopeOverrideMessage(
	TEXT("r.ValueScope.OverrideMessage"),
	1,
	TEXT("1: show an on-screen notice while any r.ValueScope console override is set (default)\n")
	TEXT("0: hide it\n")
	TEXT("DisableAllScreenMessages also hides it."),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

FValueScopeViewExtension::FValueScopeViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	// "Rendering" is on in every editor and game viewport, so the notice shows everywhere.
	DebugDrawHandle = UDebugDrawService::Register(TEXT("Rendering"),
		FDebugDrawDelegate::CreateRaw(this, &FValueScopeViewExtension::DrawOverrideNotice));
}

FValueScopeViewExtension::~FValueScopeViewExtension()
{
	UDebugDrawService::Unregister(DebugDrawHandle);
}

void FValueScopeViewExtension::DrawOverrideNotice(UCanvas* Canvas, APlayerController* PC)
{
	// Console overrides beat the camera's component, which is easy to forget, so say so
	// on screen while any are set. Follows DisableAllScreenMessages and stays out of HighResShot.
	if (!Canvas || !Canvas->Canvas || !GEngine || !GAreScreenMessagesEnabled || GIsHighResScreenshot
		|| CVarValueScopeOverrideMessage.GetValueOnGameThread() == 0)
	{
		return;
	}

	const int32 ForcedMode = CVarValueScopeMode.GetValueOnGameThread();
	const int32 ForcedZebras = CVarValueScopeZebras.GetValueOnGameThread();
	const int32 ForcedThirds = CVarValueScopeThirds.GetValueOnGameThread();

	TArray<FString> Parts;
	if (ForcedMode == 0)
	{
		Parts.Add(TEXT("Mode 0 (everything off)"));
	}
	else if (ForcedMode > 0)
	{
		Parts.Add(FString::Printf(TEXT("Mode %d"), ForcedMode));
	}
	if (ForcedZebras >= 0)
	{
		Parts.Add(FString::Printf(TEXT("Zebras %d"), ForcedZebras));
	}
	if (ForcedThirds >= 0)
	{
		Parts.Add(FString::Printf(TEXT("Thirds %d"), ForcedThirds));
	}
	if (Parts.Num() == 0)
	{
		return;
	}

	const FString Message = FString::Printf(
		TEXT("Value Scope console override: %s. Set to -1 to use the camera's settings. Hide this: r.ValueScope.OverrideMessage 0"),
		*FString::Join(Parts, TEXT(", ")));

	// Top left, below the editor viewport toolbar.
	const float DPIScale = Canvas->Canvas->GetDPIScale();
	FCanvasTextItem Text(FVector2D(20.0f, 50.0f) * DPIScale, FText::FromString(Message), GEngine->GetSmallFont(), FLinearColor::Yellow);
	Text.Scale = FVector2D(DPIScale);
	Text.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Text);
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

	if (!bActive)
	{
		// No component: start from the defaults with nothing drawn, so each cvar
		// below turns on only its own overlay.
		Settings = GetDefault<UValueScopeComponent>()->Settings;
		Settings.Mode = EValueScopeMode::Off;
		Settings.bClipZebras = false;
		Settings.bThirdsGuide = false;
	}

	const int32 ForcedMode = CVarValueScopeMode.GetValueOnGameThread();
	const int32 ForcedZebras = CVarValueScopeZebras.GetValueOnGameThread();
	const int32 ForcedThirds = CVarValueScopeThirds.GetValueOnGameThread();

	if (ForcedMode == 0)
	{
		bActive = false; // 0 means off everywhere, overlays included.
	}
	else
	{
		if (ForcedMode > 0)
		{
			Settings.Mode = static_cast<EValueScopeMode>(FMath::Clamp(ForcedMode, 1, 3));
			bActive = true;
		}
		if (ForcedZebras >= 0)
		{
			Settings.bClipZebras = ForcedZebras != 0;
			bActive = true;
		}
		if (ForcedThirds >= 0)
		{
			Settings.bThirdsGuide = ForcedThirds != 0;
			bActive = true;
		}
	}

	// Views without a state (some scene captures) have no stable key, so they get no overlay.
	if (!InView.State)
	{
		return;
	}

	FScopeLock Lock(&PendingLock);
	if (bActive && Settings.IsActive())
	{
		Pending.Add(InView.State, Settings);
	}
	else
	{
		Pending.Remove(InView.State);
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
		if (!InView.State || !Pending.RemoveAndCopyValue(InView.State, Settings))
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
	Params->ClipLevels = FVector2f(Settings.BlackClip, Settings.WhiteClip);
	Params->ClipZebras = Settings.bClipZebras ? 1 : 0;
	Params->ThirdsGuide = Settings.bThirdsGuide ? 1 : 0;
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
