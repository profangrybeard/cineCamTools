#include "ValueScopeEditorSettings.h"
#include "Editor.h"

void UValueScopeEditorSettings::Change(TFunctionRef<void(UValueScopeEditorSettings&)> Edit)
{
	UValueScopeEditorSettings* Settings = GetMutableDefault<UValueScopeEditorSettings>();
	Edit(*Settings);
	Settings->SaveConfig();
	if (GEditor)
	{
		// Non-Realtime viewports only repaint on input, so ask for it.
		GEditor->RedrawLevelEditingViewports();
	}
}

#if WITH_EDITOR
void UValueScopeEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SaveConfig();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
}
#endif
