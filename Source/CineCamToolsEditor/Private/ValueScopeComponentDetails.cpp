#include "ValueScopeComponentDetails.h"
#include "AssetToolsModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Factories/DataAssetFactory.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTools.h"
#include "PropertyHandle.h"
#include "ValueScopeComponent.h"
#include "ValueScopePreset.h"
#include "ValueScopePresetList.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValueScopeComponentDetails"

namespace
{
	// Goes through the property handle, not the component, so it's one undo step, covers every
	// selected camera, reaches instances when editing a Blueprint's component, and refreshes the panel.
	void ApplySettings(const TSharedPtr<IPropertyHandle>& Handle, const FValueScopeSettings& Settings)
	{
		if (!Handle.IsValid() || !Handle->IsValidHandle())
		{
			return;
		}
		FString Text;
		FValueScopeSettings::StaticStruct()->ExportText(Text, &Settings, nullptr, nullptr, PPF_None, nullptr);
		Handle->SetValueFromFormattedString(Text);
	}
}

TSharedRef<IDetailCustomization> FValueScopeComponentDetails::MakeInstance()
{
	return MakeShared<FValueScopeComponentDetails>();
}

void FValueScopeComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	SettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UValueScopeComponent, Settings));
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Value Scope");
	// Whole row, not the value column: the value column is too narrow for two buttons in a docked Details panel.
	Category.AddCustomRow(LOCTEXT("PresetSearch", "Preset"))
	.WholeRowContent()
	.HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FValueScopeComponentDetails::MakePresetMenu)
			.ToolTipText(LOCTEXT("ApplyTip", "Copy a preset into the settings below. You can still edit and key them after."))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Apply", "Apply Preset"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.OnClicked(this, &FValueScopeComponentDetails::OnSaveAsPreset)
			.ToolTipText(LOCTEXT("SaveTip", "Save the current settings as a new Value Scope Preset asset. It shows in every camera's preset list."))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Save", "Save as Preset"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		]
	];
}

TSharedRef<SWidget> FValueScopeComponentDetails::MakePresetMenu() const
{
	FMenuBuilder Menu(/*bShouldCloseWindowAfterMenuSelection*/ true, nullptr);
	const TSharedPtr<IPropertyHandle> Handle = SettingsHandle;

	const auto AddChoice = [&Menu, &Handle](const FValueScopePresetChoice& Choice)
	{
		const FValueScopeSettings Settings = Choice.Settings;
		Menu.AddMenuEntry(Choice.Label, Choice.ToolTip, FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Handle, Settings]() { ApplySettings(Handle, Settings); })));
	};

	Menu.BeginSection("BuiltIn", ValueScopePresets::BuiltInHeading());
	for (const FValueScopePresetChoice& Choice : ValueScopePresets::GetBuiltIn())
	{
		AddChoice(Choice);
	}
	Menu.EndSection();

	Menu.BeginSection("Project", ValueScopePresets::ProjectHeading());
	const TArray<FValueScopePresetChoice> Project = ValueScopePresets::GetProject();
	if (Project.IsEmpty())
	{
		Menu.AddMenuEntry(ValueScopePresets::NoProjectPresetsLabel(), ValueScopePresets::NoProjectPresetsToolTip(),
			FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
	}
	for (const FValueScopePresetChoice& Choice : Project)
	{
		AddChoice(Choice);
	}
	Menu.EndSection();

	return Menu.MakeWidget();
}

FReply FValueScopeComponentDetails::OnSaveAsPreset() const
{
	const UValueScopeComponent* Source = nullptr;
	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if ((Source = Cast<UValueScopeComponent>(Object.Get())) != nullptr)
		{
			break; // With several cameras selected, the first one's settings are saved.
		}
	}
	if (!Source)
	{
		return FReply::Handled();
	}

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UValueScopePreset::StaticClass();

	// Skip the factory's class picker; we already know the class. The user still picks name and folder.
	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	UObject* Created = AssetTools.CreateAssetWithDialog(TEXT("NewValueScopePreset"), TEXT("/Game"),
		UValueScopePreset::StaticClass(), Factory, NAME_None, /*bCallConfigureProperties*/ false);

	if (UValueScopePreset* Preset = Cast<UValueScopePreset>(Created))
	{
		Preset->Modify();
		Preset->Settings = Source->Settings;
		Preset->MarkPackageDirty();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
