// Wyrmhaven - the whole game interface, built in C++ so the project runs without any editor assets.
//
//  * Top bar: gold, food, gems, island level.
//  * Left menu: Collect all, Build, Dragons, Breed, Hatchery, Shop, Arena, Dragonpedia, Menu.
//  * Right panel: details and actions for whatever is selected.
//  * Toast: short feedback messages.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WyrmButton.h"

#include "WyrmHUDWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWyrmGameSubsystem;

UENUM()
enum class EWyrmPanel : uint8
{
	None,
	Structure,
	Dragon,
	Build,
	Dragons,
	Breed,
	Hatchery,
	Shop,
	Arena,
	Book,
	Menu
};

UCLASS()
class WYRMHAVEN_API UWyrmHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowStructure(const FGuid& StructureId);
	void ShowDragon(const FGuid& DragonId);
	void ShowPanel(EWyrmPanel NewPanel);
	void ClosePanel() { ShowPanel(EWyrmPanel::None); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleStateChanged();

	UFUNCTION()
	void HandleMessage(const FText& Message, bool bIsError);

	void HandleButton(UWyrmButton* Button);

	// Layout.
	void BuildLayout();
	void RefreshTopBar();
	void RefreshPanel();
	bool IsAnyButtonPressed() const;
	bool PanelHasTimers() const;

	// Panels.
	void BuildStructurePanel(UWyrmGameSubsystem& Game);
	void BuildDragonPanel(UWyrmGameSubsystem& Game);
	void BuildBuildPanel(UWyrmGameSubsystem& Game);
	void BuildDragonsPanel(UWyrmGameSubsystem& Game);
	void BuildBreedPanel(UWyrmGameSubsystem& Game);
	void BuildHatcheryPanel(UWyrmGameSubsystem& Game);
	void BuildShopPanel(UWyrmGameSubsystem& Game);
	void BuildArenaPanel(UWyrmGameSubsystem& Game);
	void BuildBookPanel(UWyrmGameSubsystem& Game);
	void BuildMenuPanel(UWyrmGameSubsystem& Game);

	// Widget helpers.
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color);
	UWyrmButton* MakeButton(const FString& Label, EWyrmAction Action, const FString& Payload, int32 Index, bool bEnabled,
		const FLinearColor& Color);
	void BeginPanel(const FString& Title);
	UTextBlock* AddText(const FString& Text, int32 Size = 16, const FLinearColor& Color = FLinearColor::White);
	UWyrmButton* AddButton(const FString& Label, EWyrmAction Action, const FString& Payload = FString(), int32 Index = 0,
		bool bEnabled = true);
	UHorizontalBox* AddRow();
	UTextBlock* AddRowText(UHorizontalBox* Row, const FString& Text, int32 Size = 16,
		const FLinearColor& Color = FLinearColor::White);
	UWyrmButton* AddRowButton(UHorizontalBox* Row, const FString& Label, EWyrmAction Action, const FString& Payload = FString(),
		int32 Index = 0, bool bEnabled = true);
	void AddSpacer();

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY()
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY()
	TObjectPtr<UTextBlock> FoodText;

	UPROPERTY()
	TObjectPtr<UTextBlock> GemsText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY()
	TObjectPtr<UVerticalBox> PanelBox;

	UPROPERTY()
	TObjectPtr<UBorder> ToastBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> ToastText;

	/** Buttons in the right panel, used to avoid rebuilding while one is held down. */
	UPROPERTY()
	TArray<TObjectPtr<UWyrmButton>> PanelButtons;

	EWyrmPanel Panel = EWyrmPanel::None;
	FGuid SelectedId;
	FGuid BreedParentA;
	FGuid BreedParentB;
	TArray<FString> BattleLog;
	bool bConfirmReset = false;
	bool bPanelDirty = true;
	bool bTopBarDirty = true;
	float RefreshTimer = 0.f;
	float ToastTimer = 0.f;
};
