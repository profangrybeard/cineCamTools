#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ValueScopeComponent.h"
#include "ValueScopeComponentDetails.h"

class FCineCamToolsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout(UValueScopeComponent::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FValueScopeComponentDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyEditor->UnregisterCustomClassLayout(UValueScopeComponent::StaticClass()->GetFName());
			PropertyEditor->NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FCineCamToolsEditorModule, CineCamToolsEditor)
