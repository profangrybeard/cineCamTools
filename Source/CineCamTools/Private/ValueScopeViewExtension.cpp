#include "ValueScopeViewExtension.h"

#include "CanvasItem.h"
#include "CineCamToolsShaders.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"
#include "ScreenPass.h"
#include "SceneView.h"

#if WITH_EDITOR
#include "EditorSupportDelegates.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogValueScope, Log, All);

// The one live extension, for console commands. Set and cleared by the extension itself.
static FValueScopeViewExtension* GValueScopeExtension = nullptr;

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

static TAutoConsoleVariable<int32> CVarValueScopeHistogram(
	TEXT("r.ValueScope.Histogram"),
	-1,
	TEXT("-1: use the Value Scope component (default)\n")
	TEXT(" 0: force the histogram off\n")
	TEXT(" 1: force the histogram on, on every view"),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarValueScopeClipPercent(
	TEXT("r.ValueScope.ClipPercent"),
	-1,
	TEXT("-1: use the Value Scope component (default)\n")
	TEXT(" 0: force clip percentages off\n")
	TEXT(" 1: force clip percentages on, on every view"),
	FConsoleVariableDelegate::CreateStatic(&OnValueScopeCVarChanged),
	ECVF_Default);

static FAutoConsoleCommand CmdValueScopeDumpHistogram(
	TEXT("r.ValueScope.DumpHistogram"),
	TEXT("Writes the latest luma histogram of each view to Saved/ValueScope as CSV (level,count). The histogram must be on."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		if (GValueScopeExtension)
		{
			GValueScopeExtension->DumpHistograms();
		}
	}));

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
		FDebugDrawDelegate::CreateRaw(this, &FValueScopeViewExtension::DrawCanvas));
	GValueScopeExtension = this;
}

FValueScopeViewExtension::~FValueScopeViewExtension()
{
	UDebugDrawService::Unregister(DebugDrawHandle);
	if (GValueScopeExtension == this)
	{
		GValueScopeExtension = nullptr;
	}
}

void FValueScopeViewExtension::DrawCanvas(UCanvas* Canvas, APlayerController* PC)
{
	if (!Canvas || !Canvas->Canvas || !GEngine || GIsHighResScreenshot)
	{
		return;
	}
	DrawOverrideNotice(Canvas);
	DrawClipPercentages(Canvas);
}

void FValueScopeViewExtension::DrawOverrideNotice(UCanvas* Canvas)
{
	// Console overrides beat the camera's component, which is easy to forget, so say so
	// on screen while any are set. Follows DisableAllScreenMessages and stays out of HighResShot.
	if (!GAreScreenMessagesEnabled || CVarValueScopeOverrideMessage.GetValueOnGameThread() == 0)
	{
		return;
	}

	const int32 ForcedMode = CVarValueScopeMode.GetValueOnGameThread();
	const int32 ForcedZebras = CVarValueScopeZebras.GetValueOnGameThread();
	const int32 ForcedThirds = CVarValueScopeThirds.GetValueOnGameThread();
	const int32 ForcedHistogram = CVarValueScopeHistogram.GetValueOnGameThread();
	const int32 ForcedClipPercent = CVarValueScopeClipPercent.GetValueOnGameThread();

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
	if (ForcedHistogram >= 0)
	{
		Parts.Add(FString::Printf(TEXT("Histogram %d"), ForcedHistogram));
	}
	if (ForcedClipPercent >= 0)
	{
		Parts.Add(FString::Printf(TEXT("ClipPercent %d"), ForcedClipPercent));
	}
	if (Parts.Num() == 0)
	{
		return;
	}

	const FString Message = FString::Printf(
		TEXT("Value Scope console override: %s. Set to -1 to use the camera's settings. Hide this: r.ValueScope.OverrideMessage 0"),
		*FString::Join(Parts, TEXT(", ")));

	// Top left, below the editor viewport toolbar. Canvas units already include the DPI scale.
	FCanvasTextItem Text(FVector2D(20.0f, 50.0f), FText::FromString(Message), GEngine->GetSmallFont(), FLinearColor::Yellow);
	Text.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Text);
}

void FValueScopeViewExtension::DrawClipPercentages(UCanvas* Canvas)
{
	const FSceneView* View = Canvas->SceneView;
	if (!View || !View->State)
	{
		return;
	}
	const FValueScopeSettings* Settings = CanvasSettings.Find(View->State);
	if (!Settings || !Settings->bClipPercentages)
	{
		return;
	}

	TArray<uint32> Bins;
	{
		FScopeLock Lock(&LatestLock);
		const FHistogram* Histogram = Latest.Find(View->State);
		if (!Histogram)
		{
			return; // First readback is still 2 to 3 frames away.
		}
		Bins = Histogram->Bins;
	}

	// Whole levels, same as the zebras and value_report.py.
	const int32 BlackLevel = FMath::RoundToInt(Settings->BlackClip * 255.0f);
	const int32 WhiteLevel = FMath::RoundToInt(Settings->WhiteClip * 255.0f);
	uint64 Total = 0, Crushed = 0, Blown = 0;
	for (int32 Level = 0; Level < Bins.Num(); ++Level)
	{
		Total += Bins[Level];
		Crushed += Level <= BlackLevel ? Bins[Level] : 0;
		Blown += Level >= WhiteLevel ? Bins[Level] : 0;
	}
	if (Total == 0)
	{
		return;
	}

	// Any clipping shows, even a sliver, so "0%" really means none.
	auto Percent = [Total](uint64 Count)
	{
		const double Value = 100.0 * double(Count) / double(Total);
		return Count == 0 ? FString(TEXT("0%")) : (Value < 0.1 ? FString(TEXT("under 0.1%")) : FString::Printf(TEXT("%.1f%%"), Value));
	};

	// Same panel rect as the shader, from the view rect, then into canvas units.
	const FIntRect ViewRect = View->UnscaledViewRect;
	const FIntPoint Size = ViewRect.Size();
	const float Scale = FMath::Max(1.0f, FMath::RoundToFloat(Size.Y / 540.0f));
	const float Width = Size.X * 0.30f;
	const float Height = Width * 0.4f;
	const float Left = ViewRect.Min.X + Size.X - 16.0f * Scale - Width;
	const float Top = ViewRect.Min.Y + Size.Y * 0.06f + (Settings->bHistogram ? Height + 4.0f * Scale : 0.0f);
	const float ToCanvas = 1.0f / Canvas->Canvas->GetDPIScale();

	FCanvasTextItem Text(FVector2D(Left, Top) * ToCanvas,
		FText::FromString(FString::Printf(TEXT("Crushed %s"), *Percent(Crushed))),
		GEngine->GetSmallFont(), FLinearColor(0.45f, 0.7f, 1.0f));
	Text.Scale = FVector2D(Scale * 0.5f); // 1 at 1080p, 2 at 4K
	Text.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Text);

	Text.Position = FVector2D(Left + Width * 0.5f, Top) * ToCanvas;
	Text.Text = FText::FromString(FString::Printf(TEXT("Blown %s"), *Percent(Blown)));
	Text.SetColor(FLinearColor(1.0f, 0.45f, 0.45f));
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
		Settings.bHistogram = false;
		Settings.bClipPercentages = false;
	}

	const int32 ForcedMode = CVarValueScopeMode.GetValueOnGameThread();
	const int32 ForcedZebras = CVarValueScopeZebras.GetValueOnGameThread();
	const int32 ForcedThirds = CVarValueScopeThirds.GetValueOnGameThread();
	const int32 ForcedHistogram = CVarValueScopeHistogram.GetValueOnGameThread();
	const int32 ForcedClipPercent = CVarValueScopeClipPercent.GetValueOnGameThread();

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
		if (ForcedHistogram >= 0)
		{
			Settings.bHistogram = ForcedHistogram != 0;
			bActive = true;
		}
		if (ForcedClipPercent >= 0)
		{
			Settings.bClipPercentages = ForcedClipPercent != 0;
			bActive = true;
		}
	}

	// Views without a state (some scene captures) have no stable key, so they get no overlay.
	if (!InView.State)
	{
		return;
	}

	if (bActive && Settings.IsActive())
	{
		CanvasSettings.Add(InView.State, Settings);
	}
	else
	{
		CanvasSettings.Remove(InView.State);
	}

	FScopeLock Lock(&PendingLock);
	if (bActive && Settings.IsActive())
	{
		// HighResShot sets this while it draws its frame, and SetupView runs inside that draw.
		Pending.Add(InView.State, FPendingView{ Settings, GIsHighResScreenshot });
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

	FPendingView PendingView;
	{
		FScopeLock Lock(&PendingLock);
		if (!InView.State || !Pending.RemoveAndCopyValue(InView.State, PendingView))
		{
			return;
		}
	}

	InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(
		this, &FValueScopeViewExtension::AfterTonemap_RenderThread, PendingView.Settings, PendingView.bHighResShot));
}

FScreenPassTexture FValueScopeViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder,
	const FSceneView& View, const FPostProcessMaterialInputs& Inputs, FValueScopeSettings Settings, bool bHighResShot)
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

	// Measured from the untouched image, before any overlay draws over it.
	// The clip percentages read the same histogram, so either one turns it on.
	FRDGBufferRef HistogramBuffer = nullptr;
	if (Settings.bHistogram || Settings.bClipPercentages)
	{
		const FString Source = FString::Printf(TEXT("in %s, out %s, %s"),
			GetPixelFormatString(SceneColor.Texture->Desc.Format),
			GetPixelFormatString(Output.Texture->Desc.Format),
			Inputs.OverrideOutput.IsValid() ? TEXT("ours is the last pass") : TEXT("other passes follow ours"));
		HistogramBuffer = AddHistogramPass(GraphBuilder, View, SceneColor, bHighResShot, Source);
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

	const bool bHistogramPanel = HistogramBuffer && Settings.bHistogram;
	if (bHistogramPanel)
	{
		// Height reference for the panel: tallest bin in levels 1 to 254.
		FRDGBufferRef MaxBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 1), TEXT("ValueScope.HistogramMax"));
		FRDGBufferSRVRef HistogramSRV = GraphBuilder.CreateSRV(HistogramBuffer, PF_R32_UINT);

		FValueScopeHistogramMaxCS::FParameters* MaxParams = GraphBuilder.AllocParameters<FValueScopeHistogramMaxCS::FParameters>();
		MaxParams->HistogramIn = HistogramSRV;
		MaxParams->HistogramMaxOut = GraphBuilder.CreateUAV(MaxBuffer, PF_R32_UINT);
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ValueScope HistogramMax"),
			TShaderMapRef<FValueScopeHistogramMaxCS>(ShaderMap), MaxParams, FIntVector(1, 1, 1));

		// Top right, 30% of view width, Photoshop's 256 x 100 proportions. The top margin
		// clears the editor viewport toolbar, which sits over the view.
		const FIntPoint ViewSize = Output.ViewRect.Size();
		const float Scale = FMath::Max(1.0f, FMath::RoundToFloat(ViewSize.Y / 540.0f));
		const float Width = ViewSize.X * 0.30f;
		const float Height = Width * 0.4f;
		const float Side = 16.0f * Scale;
		const float Top = ViewSize.Y * 0.06f;
		Params->HistogramIn = HistogramSRV;
		Params->HistogramMax = GraphBuilder.CreateSRV(MaxBuffer, PF_R32_UINT);
		Params->HistogramRect = FVector4f(ViewSize.X - Side - Width, Top, Width, Height);
	}

	FValueScopePS::FPermutationDomain Permutation;
	Permutation.Set<FValueScopePS::FHistogramDim>(bHistogramPanel);
	TShaderMapRef<FValueScopePS> PixelShader(ShaderMap, Permutation);

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		ShaderMap,
		RDG_EVENT_NAME("ValueScope (Mode %d)", Params->Mode),
		PixelShader,
		Params,
		Output.ViewRect);

	return FScreenPassTexture(Output);
}

FRDGBufferRef FValueScopeViewExtension::AddHistogramPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor, bool bHighResShot, const FString& Source)
{
	constexpr int32 NumBins = FValueScopeHistogramCS::NumBins;
	constexpr uint32 NumBytes = NumBins * sizeof(uint32);

	FRDGBufferRef HistogramBuffer = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), NumBins), TEXT("ValueScope.Histogram"));
	FRDGBufferUAVRef HistogramUAV = GraphBuilder.CreateUAV(HistogramBuffer, PF_R32_UINT);
	AddClearUAVPass(GraphBuilder, HistogramUAV, 0u);

	FValueScopeHistogramCS::FParameters* Params = GraphBuilder.AllocParameters<FValueScopeHistogramCS::FParameters>();
	Params->Input = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneColor));
	Params->InputTexture = SceneColor.Texture;
	Params->HistogramOut = HistogramUAV;

	TShaderMapRef<FValueScopeHistogramCS> ComputeShader(GetGlobalShaderMap(View.GetFeatureLevel()));
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("ValueScope Histogram %dx%d", SceneColor.ViewRect.Width(), SceneColor.ViewRect.Height()),
		ComputeShader,
		Params,
		FComputeShaderUtils::GetGroupCount(SceneColor.ViewRect.Size(), FValueScopeHistogramCS::GroupSize));

	FViewReadbacks& ViewReadbacks = Readbacks.FindOrAdd(View.State);

	// Collect earlier copies that have reached the CPU, oldest first, so the newest is published last.
	for (int32 i = 0; i < NumReadbackSlots; ++i)
	{
		FReadbackSlot& Slot = ViewReadbacks.Slots[(ViewReadbacks.Next + i) % NumReadbackSlots];
		if (Slot.bPending && Slot.Readback->IsReady())
		{
			FHistogram Result;
			Result.Size = Slot.Size;
			Result.Source = Slot.Source;
			Result.Bins.SetNumUninitialized(NumBins);
			const uint32* Data = static_cast<const uint32*>(Slot.Readback->Lock(NumBytes));
			FMemory::Memcpy(Result.Bins.GetData(), Data, NumBytes);
			Slot.Readback->Unlock();
			Slot.bPending = false;

			FScopeLock Lock(&LatestLock);
			if (Slot.bHighResShot)
			{
				LatestHighResShot = Result;
			}
			Latest.Add(View.State, MoveTemp(Result));
		}
	}

	// Queue this frame's copy in a free slot. If all are still in flight, skip this frame,
	// unless it's a HighResShot frame: then take the oldest slot and drop its copy instead.
	int32 Chosen = INDEX_NONE;
	for (int32 i = 0; i < NumReadbackSlots && Chosen == INDEX_NONE; ++i)
	{
		const int32 Index = (ViewReadbacks.Next + i) % NumReadbackSlots;
		if (!ViewReadbacks.Slots[Index].bPending)
		{
			Chosen = Index;
		}
	}
	if (Chosen == INDEX_NONE && bHighResShot)
	{
		Chosen = ViewReadbacks.Next; // Oldest copy in flight.
	}
	if (Chosen != INDEX_NONE)
	{
		FReadbackSlot& Slot = ViewReadbacks.Slots[Chosen];
		if (!Slot.Readback)
		{
			Slot.Readback = MakeUnique<FRHIGPUBufferReadback>(TEXT("ValueScope.HistogramReadback"));
		}
		AddEnqueueCopyPass(GraphBuilder, Slot.Readback.Get(), HistogramBuffer, NumBytes);
		Slot.Size = SceneColor.ViewRect.Size();
		Slot.bPending = true;
		Slot.bHighResShot = bHighResShot;
		Slot.Source = Source;
		ViewReadbacks.Next = (Chosen + 1) % NumReadbackSlots;
	}

	return HistogramBuffer;
}

void FValueScopeViewExtension::DumpHistograms()
{
	TArray<FHistogram> Histograms;
	TArray<FString> Labels;
	{
		FScopeLock Lock(&LatestLock);
		for (const TPair<const FSceneViewStateInterface*, FHistogram>& Pair : Latest)
		{
			Labels.Add(FString::Printf(TEXT("v%d"), Histograms.Num()));
			Histograms.Add(Pair.Value);
		}
		if (LatestHighResShot.IsSet())
		{
			Labels.Add(TEXT("highresshot"));
			Histograms.Add(LatestHighResShot.GetValue());
		}
	}

	if (Histograms.Num() == 0)
	{
		UE_LOG(LogValueScope, Warning, TEXT("Value Scope: no histogram yet. Turn it on first (r.ValueScope.Histogram 1), then try again."));
		return;
	}

	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ValueScope"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));

	for (int32 ViewIndex = 0; ViewIndex < Histograms.Num(); ++ViewIndex)
	{
		const FHistogram& Histogram = Histograms[ViewIndex];

		uint64 Total = 0;
		FString Csv = TEXT("level,count\n");
		for (int32 Level = 0; Level < Histogram.Bins.Num(); ++Level)
		{
			Total += Histogram.Bins[Level];
			Csv += FString::Printf(TEXT("%d,%u\n"), Level, Histogram.Bins[Level]);
		}

		const FString Path = FPaths::Combine(Dir, FString::Printf(TEXT("Histogram_%dx%d_%s_%s.csv"),
			Histogram.Size.X, Histogram.Size.Y, *Stamp, *Labels[ViewIndex]));
		FFileHelper::SaveStringToFile(Csv, *Path);

		// Every pixel of the view lands in exactly one bin, so these must match.
		const uint64 Expected = uint64(Histogram.Size.X) * uint64(Histogram.Size.Y);
		UE_LOG(LogValueScope, Display, TEXT("Value Scope: histogram %dx%d, %llu pixels counted, %llu expected%s (%s). Wrote %s"),
			Histogram.Size.X, Histogram.Size.Y, Total, Expected, Total == Expected ? TEXT("") : TEXT(" (MISMATCH)"),
			*Histogram.Source, *FPaths::ConvertRelativePathToFull(Path));
	}
}
