#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValueScopeComponent.generated.h"

UENUM(BlueprintType)
enum class EValueScopeMode : uint8
{
	Off           UMETA(ToolTip = "Image unchanged. Zebras and the thirds guide still draw if they are on."),
	PlumbingCheck UMETA(ToolTip = "Image unchanged, magenta frame on the view edges. Proves the render hook works."),
	Notan         UMETA(ToolTip = "Black, grey, white. Shows value structure."),
	FalseColor    UMETA(DisplayName = "False Color", ToolTip = "Paints each of the 11 zones (0 to X) its own color. Same colors as value_report.py.")
};

/** Built-in presets. Anything a preset doesn't list is off, and the clip levels stay at 0.02 and 0.98. */
UENUM(BlueprintType)
enum class EValueScopeBuiltInPreset : uint8
{
	Notan       UMETA(ToolTip = "Notan only: black, grey, white. Everything else off."),
	ValueCheck  UMETA(DisplayName = "Value Check", ToolTip = "False color zones with the histogram."),
	Exposure    UMETA(ToolTip = "Image unchanged. Clip zebras, histogram, clip percentages and waveform."),
	Composition UMETA(ToolTip = "Notan with the thirds guide. Value masses against the grid.")
};

class UValueScopePreset;

USTRUCT(BlueprintType)
struct FValueScopeSettings
{
	GENERATED_BODY()

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	EValueScopeMode Mode = EValueScopeMode::Notan;

	/** Below this is black in the notan. 0..1, display encoded. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1))
	float ShadowThreshold = 0.25f;

	/** Above this is white in the notan. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1))
	float HighlightThreshold = 0.75f;

	/** Stripes over crushed blacks (blue) and blown whites (red). Works with any mode, including Off. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bClipZebras = false;

	/** At or below this is crushed black. Used as a whole level: 0.02 is level 5 of 255. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1, EditCondition = "bClipZebras || bClipPercentages"))
	float BlackClip = 0.02f;

	/** At or above this is blown white. Used as a whole level: 0.98 is level 250 of 255. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ClampMin = 0, ClampMax = 1, EditCondition = "bClipZebras || bClipPercentages"))
	float WhiteClip = 0.98f;

	/** Rule of thirds lines over the frame. Works with any mode, including Off. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bThirdsGuide = false;

	/** Luma histogram of the frame, 0 to 255, like Photoshop's Luminosity histogram. Works with any mode, including Off. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bHistogram = false;

	/** Share of the frame crushed to black and blown to white, as text under the histogram. Uses the Black Clip and White Clip levels. Not baked into screenshots. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bClipPercentages = false;

	/** Luma waveform, top left. Image columns left to right, dark at the bottom, bright at the top. Shows where in the frame each value sits. Works with any mode, including Off. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bWaveform = false;

	/** Reads the value under a small box: the cursor in editor viewports, the frame center in PIE and games. Shows level 0 to 255 and zone 0 to X. Not baked into screenshots. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bSpotMeter = false;

	/** True if anything would be drawn or measured. */
	bool IsActive() const
	{
		return Mode != EValueScopeMode::Off || bClipZebras || bThirdsGuide || bHistogram || bClipPercentages || bWaveform || bSpotMeter;
	}
};

/**
 * Add to a CineCameraActor. When that actor is the view target, the overlay draws
 * after tonemapping. Movie Render Queue will bake it in, so turn it off for final renders.
 * Every setting can be keyed in Sequencer, like the camera's focal length: switch the
 * scope per shot, or key Enabled off for the final render.
 * A preset is copied into Settings when applied, so the settings stay editable and keyable after.
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent, DisplayName = "Value Scope"))
class CINECAMTOOLS_API UValueScopeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope")
	bool bEnabled = true;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Value Scope", meta = (ShowOnlyInnerProperties))
	FValueScopeSettings Settings;

	/** Copies a built-in preset into Settings. Keyed Sequencer tracks still win on the next frame. */
	UFUNCTION(BlueprintCallable, Category = "Value Scope")
	void ApplyBuiltInPreset(EValueScopeBuiltInPreset Preset);

	/** Copies a preset asset into Settings. Does nothing if Preset is empty. */
	UFUNCTION(BlueprintCallable, Category = "Value Scope")
	void ApplyPreset(const UValueScopePreset* Preset);

	/** The settings a built-in preset applies. One definition, shared by the component, the Details picker and the toolbar. */
	UFUNCTION(BlueprintPure, Category = "Value Scope")
	static FValueScopeSettings GetBuiltInPreset(EValueScopeBuiltInPreset Preset);
};
