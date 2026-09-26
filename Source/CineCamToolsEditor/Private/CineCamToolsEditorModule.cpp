#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ValueScopeComponent.h"
#include "ValueScopeComponentDetails.h"
#include "ValueScopeToolbar.h"

class FCineCamToolsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout(UValueScopeComponent::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FValueScopeComponentDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();

		ValueScopeToolbar::Register(this);
	}

	virtual void ShutdownModule() override
	{
		ValueScopeToolbar::Unregister(this);

		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyEditor->UnregisterCustomClassLayout(UValueScopeComponent::StaticClass()->GetFName());
			PropertyEditor->NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FCineCamToolsEditorModule, CineCamToolsEditor)
