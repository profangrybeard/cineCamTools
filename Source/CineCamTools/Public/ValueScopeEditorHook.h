#pragma once

#include "CoreMinimal.h"
#include "ValueScopeComponent.h"

class FSceneViewStateInterface;

namespace ValueScope
{
	/**
	 * Lets the editor module add its viewport toolbar setting without the runtime module knowing
	 * about the editor. Called on the game thread for each view whose view target has no Value Scope
	 * component. Return true and fill OutSettings if the toolbar applies to that view.
	 */
	using FEditorViewportResolver = TFunction<bool(const FSceneViewStateInterface* State, FValueScopeSettings& OutSettings)>;

	/** Pass an empty function to clear it. Packaged games never set one. */
	CINECAMTOOLS_API void SetEditorViewportResolver(FEditorViewportResolver Resolver);

	/**
	 * Where the mouse is, for the spot meter. Called on the game thread per view. Return true and
	 * fill OutViewportPixel (pixels from the viewport's top left) if the cursor is over the level
	 * editor viewport that owns this view state. Otherwise the meter reads the frame center.
	 */
	using FEditorCursorProvider = TFunction<bool(const FSceneViewStateInterface* State, FVector2D& OutViewportPixel)>;

	/** Pass an empty function to clear it. Packaged games never set one. */
	CINECAMTOOLS_API void SetEditorCursorProvider(FEditorCursorProvider Provider);

	enum class EAddSpotPinResult : uint8
	{
		Added,
		Full,     // Already 4 pins on this view.
		MeterOff, // The spot meter isn't on for this view.
		NoCursor, // The cursor hasn't been over this view's image yet.
	};

	/**
	 * Pins a spot meter point (A to D) where the cursor was last over this view's image.
	 * Pins belong to the view, keep reading live, and are not saved. Game thread.
	 */
	CINECAMTOOLS_API EAddSpotPinResult AddSpotPin(const FSceneViewStateInterface* State);

	/** Removes every view's pins. Game thread. */
	CINECAMTOOLS_API void ClearSpotPins();
}
