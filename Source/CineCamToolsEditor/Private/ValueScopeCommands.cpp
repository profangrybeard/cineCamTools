#include "ValueScopeCommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "ValueScopeCommands"

FValueScopeCommands::FValueScopeCommands()
	: TCommands<FValueScopeCommands>(TEXT("ValueScope"), LOCTEXT("Context", "Value Scope"), TEXT("LevelEditor"), FAppStyle::GetAppStyleSetName())
{
}

void FValueScopeCommands::RegisterCommands()
{
	// Alt+M: Ctrl+Alt+M didn't work on Tim's machine (2026-09-26). Alt+M is also Control Rig's in its
	// edit mode and DMX's in its own editor; neither applies in the normal level editor.
	UI_COMMAND(PinSpot, "Pin Spot Meter Point", "Pin a spot meter point (A to D) under the cursor. It keeps reading live, and B to D show how far they are from A.",
		EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::M));
	UI_COMMAND(ClearPins, "Clear Spot Meter Pins", "Remove every spot meter pin, in every viewport.",
		EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
