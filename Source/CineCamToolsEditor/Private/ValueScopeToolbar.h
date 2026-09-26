#pragma once

#include "CoreMinimal.h"

/**
 * Value Scope button on the level viewport toolbar (LevelEditor.ViewportToolbar, Right section).
 * Click toggles it; the arrow opens Enabled, Presets, Mode, overlays and More Settings.
 * Also tells the runtime module which views are level editor viewports.
 */
namespace ValueScopeToolbar
{
	void Register(void* Owner);
	void Unregister(void* Owner);
}
