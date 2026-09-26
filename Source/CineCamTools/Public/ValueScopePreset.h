#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ValueScopeComponent.h"
#include "ValueScopePreset.generated.h"

/**
 * A saved set of Value Scope settings. Make one with Save as Preset on a camera's Value Scope,
 * or Content Browser > Miscellaneous > Data Asset > Value Scope Preset. It shows in every
 * Value Scope's preset picker. Applying it copies the settings, so later edits to the preset
 * don't change cameras that already used it.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Value Scope Preset"))
class CINECAMTOOLS_API UValueScopePreset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** What this preset is for. Shown as the tooltip in the preset picker. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Value Scope", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Value Scope", meta = (ShowOnlyInnerProperties))
	FValueScopeSettings Settings;
};
