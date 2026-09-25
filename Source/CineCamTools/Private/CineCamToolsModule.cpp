#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "SceneViewExtension.h"
#include "ValueScopeViewExtension.h"

DEFINE_LOG_CATEGORY_STATIC(LogCineCamTools, Log, All);

class FCineCamToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (GEngine)
		{
			CreateExtensions();
		}
		else
		{
			PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FCineCamToolsModule::CreateExtensions);
		}
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
		ValueScope.Reset();
	}

private:
	void CreateExtensions()
	{
		ValueScope = FSceneViewExtensions::NewExtension<FValueScopeViewExtension>();
		// Plumbing marker. Search the Output Log for this line to confirm the module loaded.
		UE_LOG(LogCineCamTools, Log, TEXT("CineCamTools: Value Scope view extension registered."));
	}

	TSharedPtr<FValueScopeViewExtension, ESPMode::ThreadSafe> ValueScope;
	FDelegateHandle PostEngineInitHandle;
};

IMPLEMENT_MODULE(FCineCamToolsModule, CineCamTools)
