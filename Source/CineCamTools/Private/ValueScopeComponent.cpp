#include "ValueScopeComponent.h"
#include "ValueScopePreset.h"

void UValueScopeComponent::ApplyBuiltInPreset(EValueScopeBuiltInPreset Preset)
{
	Settings = GetBuiltInPreset(Preset);
}

void UValueScopeComponent::ApplyPreset(const UValueScopePreset* Preset)
{
	if (Preset)
	{
		Settings = Preset->Settings;
	}
}

FValueScopeSettings UValueScopeComponent::GetBuiltInPreset(EValueScopeBuiltInPreset Preset)
{
	// Start from the struct defaults (Notan at 0.25 / 0.75, clip levels 0.02 / 0.98, every overlay off).
	FValueScopeSettings Out;

	switch (Preset)
	{
	case EValueScopeBuiltInPreset::Notan:
		Out.Mode = EValueScopeMode::Notan;
		break;

	case EValueScopeBuiltInPreset::ValueCheck:
		Out.Mode = EValueScopeMode::FalseColor;
		Out.bHistogram = true;
		break;

	case EValueScopeBuiltInPreset::Exposure:
		Out.Mode = EValueScopeMode::Off;
		Out.bClipZebras = true;
		Out.bHistogram = true;
		Out.bClipPercentages = true;
		Out.bWaveform = true;
		break;

	case EValueScopeBuiltInPreset::Composition:
		Out.Mode = EValueScopeMode::Notan;
		Out.bThirdsGuide = true;
		break;
	}

	return Out;
}
