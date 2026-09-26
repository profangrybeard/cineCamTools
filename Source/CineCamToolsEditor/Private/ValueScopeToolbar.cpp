#include "ValueScopeToolbar.h"
#include "Editor.h"
#include "ISettingsModule.h"
#include "LevelEditorViewport.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "ValueScopeEditorHook.h"
#include "ValueScopeEditorSettings.h"
#include "ValueScopePresetList.h"

#define LOCTEXT_NAMESPACE "ValueScopeToolbar"

namespace
{
	FDelegateHandle StartupHandle;

	const UValueScopeEditorSettings& Current()
	{
		return *GetDefault<UValueScopeEditorSettings>();
	}

	// Anything picked in the menu also turns the scope on, so the choice is visible straight away.
	void ChangeAndEnable(TFunction<void(FValueScopeSettings&)> Edit)
	{
		UValueScopeEditorSettings::Change([&Edit](UValueScopeEditorSettings& S)
		{
			Edit(S.Settings);
			S.bEnabled = true;
		});
	}

	ECheckBoxState CheckState(bool bChecked)
	{
		return bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	FText ButtonLabel()
	{
		if (!Current().bEnabled)
		{
			return LOCTEXT("Label", "Value Scope");
		}
		// Never "Off" while enabled: Mode Off only means the image is left alone (the Exposure preset),
		// and "Value Scope: Off" read as the whole scope being off.
		const FValueScopeSettings& S = Current().Settings;
		FText What;
		if (S.Mode != EValueScopeMode::Off)
		{
			What = StaticEnum<EValueScopeMode>()->GetDisplayNameTextByValue(static_cast<int64>(S.Mode));
		}
		else if (S.bClipZebras || S.bThirdsGuide || S.bHistogram || S.bClipPercentages || S.bWaveform)
		{
			What = LOCTEXT("LabelOverlays", "Overlays");
		}
		else
		{
			What = LOCTEXT("LabelOnly", "On");
		}
		return FText::Format(LOCTEXT("LabelOn", "Value Scope: {0}"), What);
	}

	void AddPresetEntries(FToolMenuSection& Section, const TArray<FValueScopePresetChoice>& Choices)
	{
		for (const FValueScopePresetChoice& Choice : Choices)
		{
			const FValueScopeSettings Settings = Choice.Settings;
			Section.AddMenuEntry(NAME_None, Choice.Label, Choice.ToolTip, FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Settings]()
				{
					ChangeAndEnable([&Settings](FValueScopeSettings& S) { S = Settings; });
				})));
		}
	}

	void FillPresets(UToolMenu* Menu)
	{
		FToolMenuSection& BuiltIn = Menu->AddSection("BuiltIn", ValueScopePresets::BuiltInHeading());
		AddPresetEntries(BuiltIn, ValueScopePresets::GetBuiltIn());

		FToolMenuSection& Project = Menu->AddSection("Project", ValueScopePresets::ProjectHeading());
		const TArray<FValueScopePresetChoice> Choices = ValueScopePresets::GetProject();
		if (Choices.IsEmpty())
		{
			Project.AddMenuEntry("None", ValueScopePresets::NoProjectPresetsLabel(), ValueScopePresets::NoProjectPresetsToolTip(),
				FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
		}
		AddPresetEntries(Project, Choices);
	}

	void AddModeEntry(FToolMenuSection& Section, EValueScopeMode Mode)
	{
		const UEnum* Enum = StaticEnum<EValueScopeMode>();
		const int64 Value = static_cast<int64>(Mode);
		Section.AddMenuEntry(FName(Enum->GetNameStringByValue(Value)),
			Enum->GetDisplayNameTextByValue(Value), Enum->GetToolTipTextByIndex(Enum->GetIndexByValue(Value)), FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([Mode]() { ChangeAndEnable([Mode](FValueScopeSettings& S) { S.Mode = Mode; }); }),
				FCanExecuteAction(),
				FGetActionCheckState::CreateLambda([Mode]() { return CheckState(Current().Settings.Mode == Mode); })),
			EUserInterfaceActionType::RadioButton);
	}

	void AddOverlayEntry(FToolMenuSection& Section, FName Name, const FText& Label, const FText& ToolTip, bool FValueScopeSettings::* Member)
	{
		Section.AddMenuEntry(Name, Label, ToolTip, FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([Member]()
				{
					ChangeAndEnable([Member](FValueScopeSettings& S) { S.*Member = !(S.*Member); });
				}),
				FCanExecuteAction(),
				FGetActionCheckState::CreateLambda([Member]() { return CheckState(Current().Settings.*Member); })),
			EUserInterfaceActionType::ToggleButton);
	}

	void FillMenu(UToolMenu* Menu)
	{
		FToolMenuSection& Top = Menu->AddSection("ValueScope", LOCTEXT("Heading", "Value Scope"));
		Top.AddMenuEntry("Enabled", LOCTEXT("Enabled", "Enabled"),
			LOCTEXT("EnabledTip", "Show Value Scope in the level editor viewports. A camera's own Value Scope wins while you look through it."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([]() { UValueScopeEditorSettings::Change([](UValueScopeEditorSettings& S) { S.bEnabled = !S.bEnabled; }); }),
				FCanExecuteAction(),
				FGetActionCheckState::CreateLambda([]() { return CheckState(Current().bEnabled); })),
			EUserInterfaceActionType::ToggleButton);
		Top.AddSubMenu("Presets", LOCTEXT("Presets", "Presets"),
			LOCTEXT("PresetsTip", "Set everything at once. The same presets as a camera's Value Scope."),
			FNewToolMenuDelegate::CreateStatic(&FillPresets));

		FToolMenuSection& ModeSection = Menu->AddSection("Mode", LOCTEXT("Mode", "Mode"));
		AddModeEntry(ModeSection, EValueScopeMode::Off);
		AddModeEntry(ModeSection, EValueScopeMode::Notan);
		AddModeEntry(ModeSection, EValueScopeMode::FalseColor);

		FToolMenuSection& Overlays = Menu->AddSection("Overlays", LOCTEXT("Overlays", "Overlays"));
		AddOverlayEntry(Overlays, "ClipZebras", LOCTEXT("Zebras", "Clip Zebras"),
			LOCTEXT("ZebrasTip", "Stripes over crushed blacks (blue) and blown whites (red)."), &FValueScopeSettings::bClipZebras);
		AddOverlayEntry(Overlays, "Thirds", LOCTEXT("Thirds", "Thirds Guide"),
			LOCTEXT("ThirdsTip", "Rule of thirds lines over the frame."), &FValueScopeSettings::bThirdsGuide);
		AddOverlayEntry(Overlays, "Histogram", LOCTEXT("Histogram", "Histogram"),
			LOCTEXT("HistogramTip", "Luma histogram, top right, like Photoshop's Luminosity histogram."), &FValueScopeSettings::bHistogram);
		AddOverlayEntry(Overlays, "ClipPercentages", LOCTEXT("ClipPercent", "Clip Percentages"),
			LOCTEXT("ClipPercentTip", "Share of the frame crushed and blown, under the histogram."), &FValueScopeSettings::bClipPercentages);
		AddOverlayEntry(Overlays, "Waveform", LOCTEXT("Waveform", "Waveform"),
			LOCTEXT("WaveformTip", "Luma waveform, top left. Shows where in the frame each value sits."), &FValueScopeSettings::bWaveform);

		FToolMenuSection& More = Menu->AddSection("More");
		More.AddMenuEntry("MoreSettings", LOCTEXT("MoreSettings", "More Settings..."),
			LOCTEXT("MoreSettingsTip", "Notan thresholds and clip levels, in Editor Preferences > Plugins > Value Scope."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				const UValueScopeEditorSettings& S = Current();
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
					.ShowViewer(S.GetContainerName(), S.GetCategoryName(), S.GetSectionName());
			})));
	}

	// True if State belongs to a level editor viewport. Material editors, thumbnails,
	// asset previews and PIE all have view states of their own, so they never match.
	bool IsLevelViewportState(const FSceneViewStateInterface* State)
	{
		if (!GEditor)
		{
			return false;
		}
		for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
		{
			if (!Client)
			{
				continue;
			}
			if (Client->ViewState.GetReference() == State)
			{
				return true;
			}
			for (FSceneViewStateReference& Stereo : Client->StereoViewStates)
			{
				if (Stereo.GetReference() == State)
				{
					return true;
				}
			}
		}
		return false;
	}
}

void ValueScopeToolbar::Register(void* Owner)
{
	ValueScope::SetEditorViewportResolver([](const FSceneViewStateInterface* State, FValueScopeSettings& OutSettings)
	{
		const UValueScopeEditorSettings& S = Current();
		if (!S.bEnabled || !IsLevelViewportState(State))
		{
			return false;
		}
		OutSettings = S.Settings;
		return true;
	});

	StartupHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([Owner]()
	{
		FToolMenuOwnerScoped ScopedOwner(Owner);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ViewportToolbar");
		FToolMenuSection& Right = Menu->FindOrAddSection("Right");

		// Same shape as the engine's Surface snapping button: click toggles, arrow opens the menu.
		FToolMenuEntry Entry = FToolMenuEntry::InitSubMenu(
			"ValueScope",
			LOCTEXT("Label", "Value Scope"),
			LOCTEXT("Tip", "Value Scope in the level editor viewports: notan, false color, zebras, histogram, waveform. Click to turn on or off."),
			FNewToolMenuDelegate::CreateStatic(&FillMenu),
			FUIAction(
				FExecuteAction::CreateLambda([]() { UValueScopeEditorSettings::Change([](UValueScopeEditorSettings& S) { S.bEnabled = !S.bEnabled; }); }),
				FCanExecuteAction(),
				FGetActionCheckState::CreateLambda([]() { return CheckState(Current().bEnabled); })),
			EUserInterfaceActionType::ToggleButton,
			/*bInOpenSubMenuOnClick*/ false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
		Entry.ToolBarData.LabelOverride = TAttribute<FText>::CreateStatic(&ButtonLabel);
		Right.AddEntry(Entry);
	}));
}

void ValueScopeToolbar::Unregister(void* Owner)
{
	ValueScope::SetEditorViewportResolver({});
	if (UObjectInitialized())
	{
		UToolMenus::UnRegisterStartupCallback(StartupHandle);
		UToolMenus::UnregisterOwner(Owner);
	}
}

#undef LOCTEXT_NAMESPACE
