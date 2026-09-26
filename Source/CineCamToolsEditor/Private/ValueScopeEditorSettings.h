#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ValueScopeComponent.h"
#include "ValueScopeEditorSettings.generated.h"

/**
 * The level viewport toolbar's Value Scope. One setting for every level editor viewport,
 * saved per user and per project (Saved/Config). Looking through a camera that has its own
 * Value Scope component shows the component instead, the same as in PIE. Console
 * r.ValueScope.* settings win over both.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Value Scope"))
class UValueScopeEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Show Value Scope in the level editor viewports. Same as clicking the Value Scope button on the viewport toolbar. */
	UPROPERTY(config, EditAnywhere, Category = "Viewport Toolbar")
	bool bEnabled = false;

	UPROPERTY(config, EditAnywhere, Category = "Viewport Toolbar", meta = (ShowOnlyInnerProperties))
	FValueScopeSettings Settings;

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Change the settings from code: runs Edit, saves, and repaints the viewports. */
	static void Change(TFunctionRef<void(UValueScopeEditorSettings&)> Edit);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
