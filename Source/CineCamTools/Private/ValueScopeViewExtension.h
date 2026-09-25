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
		const FPostProcessMaterialInputs& Inputs, FValueScopeSettings Settings);

	// Settings resolved on the game thread in SetupView, consumed once on the render thread.
	// Keyed by view state, not view pointer: the renderer copies each FSceneView into an
	// FViewInfo, so the view pointer changes but the State pointer carries over.
	FCriticalSection PendingLock;
	TMap<const FSceneViewStateInterface*, FValueScopeSettings> Pending;
};
