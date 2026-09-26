#include "ValueScopePresetList.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ValueScopePreset.h"

#define LOCTEXT_NAMESPACE "ValueScopePresets"

TArray<FValueScopePresetChoice> ValueScopePresets::GetBuiltIn()
{
	TArray<FValueScopePresetChoice> Out;
	const UEnum* Enum = StaticEnum<EValueScopeBuiltInPreset>();
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index) // The last entry is the generated _MAX.
	{
		const EValueScopeBuiltInPreset Preset = static_cast<EValueScopeBuiltInPreset>(Enum->GetValueByIndex(Index));
		Out.Add({ Enum->GetDisplayNameTextByIndex(Index), Enum->GetToolTipTextByIndex(Index), UValueScopeComponent::GetBuiltInPreset(Preset) });
	}
	return Out;
}

TArray<FValueScopePresetChoice> ValueScopePresets::GetProject()
{
	TArray<FAssetData> Assets;
	IAssetRegistry::GetChecked().GetAssetsByClass(UValueScopePreset::StaticClass()->GetClassPathName(), Assets, true);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });

	TArray<FValueScopePresetChoice> Out;
	for (const FAssetData& Asset : Assets)
	{
		if (const UValueScopePreset* Preset = Cast<UValueScopePreset>(Asset.GetAsset()))
		{
			const FText Tip = Preset->Description.IsEmpty() ? FText::FromName(Asset.PackageName) : Preset->Description;
			Out.Add({ FText::FromName(Asset.AssetName), Tip, Preset->Settings });
		}
	}
	return Out;
}

FText ValueScopePresets::BuiltInHeading() { return LOCTEXT("BuiltIn", "Built-in"); }
FText ValueScopePresets::ProjectHeading() { return LOCTEXT("Project", "Project presets"); }
FText ValueScopePresets::NoProjectPresetsLabel() { return LOCTEXT("None", "None yet"); }
FText ValueScopePresets::NoProjectPresetsToolTip()
{
	return LOCTEXT("NoneTip", "Use Save as Preset on a camera's Value Scope, or Content Browser > Miscellaneous > Data Asset > Value Scope Preset.");
}

#undef LOCTEXT_NAMESPACE
