#pragma once

#include "CoreMinimal.h"
#include "ValueScopeComponent.h"

// One list of presets for every menu that offers them (component Details, viewport toolbar),
// so they can't drift apart.
struct FValueScopePresetChoice
{
	FText Label;
	FText ToolTip;
	FValueScopeSettings Settings;
};

namespace ValueScopePresets
{
	/** Notan, Value Check, Exposure, Composition, from UValueScopeComponent::GetBuiltInPreset. */
	TArray<FValueScopePresetChoice> GetBuiltIn();

	/** Every Value Scope Preset asset in the project, sorted by name. Loads them; they're tiny. */
	TArray<FValueScopePresetChoice> GetProject();

	FText BuiltInHeading();
	FText ProjectHeading();
	FText NoProjectPresetsLabel();
	FText NoProjectPresetsToolTip();
}
