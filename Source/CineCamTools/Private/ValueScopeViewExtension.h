#pragma once

#include "CoreMinimal.h"
#include "RenderGraphFwd.h"
#include "SceneViewExtension.h"
#include "ValueScopeComponent.h"

struct FPostProcessMaterialInputs;
struct FScreenPassTexture;
class FRDGBuilder;
class FSceneViewStateInterface;

class FValueScopeViewExtension : public FSceneViewExtensionBase
{
	// A few copies in flight per view, since each takes 2 to 3 frames to reach the CPU.
	static constexpr int32 NumReadbackSlots = 4;

public:
	FValueScopeViewExtension(const FAutoRegister& AutoRegister);
	virtual ~FValueScopeViewExtension();

	// Game thread
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	// Render thread
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
		FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	// Game thread. Canvas text on every editor and game viewport: the console override
	// notice and the clip percentages. Canvas text never lands in HighResShot.
	void DrawCanvas(class UCanvas* Canvas, class APlayerController* PC);
	void DrawOverrideNotice(class UCanvas* Canvas);
	void DrawHDRNotice(class UCanvas* Canvas);

	// Game thread only. Views whose value tools were turned off because the output is HDR.
	TSet<const FSceneViewStateInterface*> HDRBlocked;
	void DrawClipPercentages(class UCanvas* Canvas);
	FDelegateHandle DebugDrawHandle;

	// Game thread only. Each view's resolved settings, for the canvas text.
	TMap<const FSceneViewStateInterface*, FValueScopeSettings> CanvasSettings;

	// Spot meter points of one view, in view UV (0 to 1). Index 0 is the live point (cursor or center).
	using FSpotPoints = TArray<FVector2f, TInlineAllocator<5>>;

	// Game thread only. Where each view's meter boxes are, for the canvas.
	TMap<const FSceneViewStateInterface*, FSpotPoints> CanvasSpots;
	void DrawSpotMeter(class UCanvas* Canvas);

	FScreenPassTexture AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs, FValueScopeSettings Settings, bool bHighResShot, FSpotPoints SpotPoints);

	// Render thread. Averages LumaLevel() in a box at each point and queues the result back to the CPU.
	void AddSpotMeterPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor, const FSpotPoints& Points);

	struct FSpotReadbacks
	{
		TUniquePtr<class FRHIGPUBufferReadback> Readback[NumReadbackSlots];
		int32 NumPoints[NumReadbackSlots] = {};
		bool bPending[NumReadbackSlots] = {};
		int32 Next = 0;
	};
	TMap<const FSceneViewStateInterface*, FSpotReadbacks> SpotReadbacks; // Render thread only.
	TMap<const FSceneViewStateInterface*, TArray<int32>> LatestSpot;     // Levels per point, under LatestLock.

	// Game thread. Resolves one view's settings (component, then cvars) into Pending and CanvasSettings.
	void ResolveView(const FSceneView& InView);

	// Settings resolved on the game thread in BeginRenderViewFamily, consumed once on the render thread.
	// Keyed by view state, not view pointer: the renderer copies each FSceneView into an
	// FViewInfo, so the view pointer changes but the State pointer carries over.
	struct FPendingView
	{
		FValueScopeSettings Settings;
		bool bHighResShot = false; // This frame is a HighResShot capture.
		FSpotPoints SpotPoints;    // Empty unless the spot meter is on.
	};
	FCriticalSection PendingLock;
	TMap<const FSceneViewStateInterface*, FPendingView> Pending;

public:
	// Game thread. Writes the latest histogram of every view to Saved/ValueScope as CSV.
	void DumpHistograms();

private:
	// Render thread. Builds this frame's histogram, queues its copy back to the CPU, and returns it for the panel.
	FRDGBufferRef AddHistogramPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor, bool bHighResShot, const FString& Source);

	struct FReadbackSlot
	{
		TUniquePtr<class FRHIGPUBufferReadback> Readback;
		FIntPoint Size = FIntPoint::ZeroValue;
		bool bPending = false;
		bool bHighResShot = false;
		FString Source;
	};
	struct FViewReadbacks
	{
		FReadbackSlot Slots[NumReadbackSlots];
		int32 Next = 0;
	};
	TMap<const FSceneViewStateInterface*, FViewReadbacks> Readbacks; // Render thread only.

	struct FHistogram
	{
		TArray<uint32> Bins;
		FIntPoint Size = FIntPoint::ZeroValue;
		FString Source; // Input and output formats, and whether other passes follow ours.
	};
	FCriticalSection LatestLock;
	TMap<const FSceneViewStateInterface*, FHistogram> Latest; // Written on render thread, read on game thread.
	TOptional<FHistogram> LatestHighResShot; // The last HighResShot frame, for an exact check against its PNG.
};
