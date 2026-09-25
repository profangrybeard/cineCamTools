#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "ValueScopeComponent.h"

struct FPostProcessMaterialInputs;
struct FScreenPassTexture;
class FRDGBuilder;
class FSceneViewStateInterface;

class FValueScopeViewExtension : public FSceneViewExtensionBase
{
public:
	FValueScopeViewExtension(const FAutoRegister& AutoRegister);
	virtual ~FValueScopeViewExtension();

	// Game thread
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	// Render thread
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& InView,
		FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	// Game thread. Draws the console override notice on every editor and game viewport.
	void DrawOverrideNotice(class UCanvas* Canvas, class APlayerController* PC);
	FDelegateHandle DebugDrawHandle;

	FScreenPassTexture AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs, FValueScopeSettings Settings, bool bHighResShot);

	// Settings resolved on the game thread in SetupView, consumed once on the render thread.
	// Keyed by view state, not view pointer: the renderer copies each FSceneView into an
	// FViewInfo, so the view pointer changes but the State pointer carries over.
	struct FPendingView
	{
		FValueScopeSettings Settings;
		bool bHighResShot = false; // This frame is a HighResShot capture.
	};
	FCriticalSection PendingLock;
	TMap<const FSceneViewStateInterface*, FPendingView> Pending;

public:
	// Game thread. Writes the latest histogram of every view to Saved/ValueScope as CSV.
	void DumpHistograms();

private:
	// Render thread. Builds this frame's histogram and queues its copy back to the CPU.
	void AddHistogramPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor, bool bHighResShot, const FString& Source);

	// A few copies in flight per view, since each takes 2 to 3 frames to reach the CPU.
	static constexpr int32 NumReadbackSlots = 4;
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
