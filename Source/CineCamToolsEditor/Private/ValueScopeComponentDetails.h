#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class SWidget;
class UValueScopeComponent;

/**
 * Adds a Preset row to the top of the Value Scope category: Apply Preset lists the built-in
 * presets, then every Value Scope Preset asset in the project, and copies the chosen one into
 * Settings (one undo step). Save as Preset writes the current settings to a new asset.
 */
class FValueScopeComponentDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TSharedRef<SWidget> MakePresetMenu() const;
	FReply OnSaveAsPreset() const;

	TSharedPtr<IPropertyHandle> SettingsHandle;
	TArray<TWeakObjectPtr<UObject>> Objects;
};
