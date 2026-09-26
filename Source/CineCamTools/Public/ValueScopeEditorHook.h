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
}
