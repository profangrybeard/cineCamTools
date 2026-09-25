#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValueScopeComponent.generated.h"

UENUM(BlueprintType)
enum class EValueScopeMode : uint8
{
	Off,
	PlumbingCheck UMETA(ToolTip = "Image unchanged, magenta frame on the view edges. Proves the render hook works."),
	Notan         UMETA(ToolTip = "Black, grey, white. Shows value structure.")
};

USTRUCT(BlueprintType)
struct FValueScopeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	EValueScopeMode Mode = EValueScopeMode::Notan;

	/** Below this is black in the notan. 0..1, display encoded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1))
	float ShadowThreshold = 0.25f;

	/** Above this is white in the notan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1))
	float HighlightThreshold = 0.75f;
};

/**
 * Add to a CineCameraActor. When that actor is the view target, the overlay draws
 * after tonemapping. Movie Render Queue will bake it in, so turn it off for final renders.
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent, DisplayName = "Value Scope"))
class CINECAMTOOLS_API UValueScopeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ShowOnlyInnerProperties))
	FValueScopeSettings Settings;
};
