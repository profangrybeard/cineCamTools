#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

// Level editor commands. Rebindable in Editor Preferences > Keyboard Shortcuts > Value Scope.
class FValueScopeCommands : public TCommands<FValueScopeCommands>
{
public:
	FValueScopeCommands();

	virtual void RegisterCommands() override;

	/** Pins a spot meter point under the cursor in the level viewport it's over. Alt+M. */
	TSharedPtr<FUICommandInfo> PinSpot;

	/** Removes every spot meter pin. No default key. */
	TSharedPtr<FUICommandInfo> ClearPins;
};
