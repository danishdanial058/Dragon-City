#include "UI/WyrmHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/WyrmGameSubsystem.h"
#include "Game/WyrmSaveGame.h"
#include "Rules/WyrmBreeding.h"
#include "Rules/WyrmProgression.h"
#include "UI/WyrmButton.h"

#include <algorithm>

namespace
{

const FLinearColor ColPanel(0.02f, 0.03f, 0.06f, 0.85f);
const FLinearColor ColButton(0.16f, 0.32f, 0.55f, 1.f);
const FLinearColor ColGood(0.15f, 0.55f, 0.25f, 1.f);
const FLinearColor ColGem(0.55f, 0.20f, 0.65f, 1.f);
const FLinearColor ColMuted(0.65f, 0.68f, 0.72f, 1.f);
const FLinearColor ColGold(1.f, 0.82f, 0.25f, 1.f);
const FLinearColor ColBad(1.f, 0.45f, 0.40f, 1.f);

FLinearColor WyrmRarityColor(Wyrm::Rarity R)
{
	switch (R)
	{
	case Wyrm::Rarity::Rare:
		return FLinearColor(0.35f, 0.65f, 1.f);
	case Wyrm::Rarity::Epic:
		return FLinearColor(0.80f, 0.45f, 1.f);
	case Wyrm::Rarity::Legendary:
		return FLinearColor(1.f, 0.78f, 0.20f);
	default:
		return FLinearColor(0.90f, 0.90f, 0.90f);
	}
}

FString WyrmGuid(const FGuid& Id)
{
	return Id.ToString();
}

FGuid WyrmParseGuid(const FString& Text)
{
	FGuid Id;
	FGuid::Parse(Text, Id);
	return Id;
}

FString WyrmSpeciesLine(const Wyrm::SpeciesDef& S)
{
	return FString::Printf(TEXT("%s  -  %s, %s"), *WyrmText::ToF(S.Name), *WyrmText::Elements(S.Elements),
		ANSI_TO_TCHAR(Wyrm::RarityName(S.Rar)));
}

FString WyrmHealthBar(int32 Health, int32 MaxHealth)
{
	const int32 Segments = 20;
	const int32 Filled = MaxHealth > 0 ? FMath::Clamp(FMath::CeilToInt(20.f * Health / MaxHealth), 0, Segments) : 0;
	return FString::ChrN(Filled, TEXT('|')) + FString::ChrN(Segments - Filled, TEXT('.'));
}
} // namespace

// ---- Layout --------------------------------------------------------------------------------------

TSharedRef<SWidget> UWyrmHUDWidget::RebuildWidget()
{
	// A Blueprint subclass with its own designer layout keeps it; otherwise build ours.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

UTextBlock* UWyrmHUDWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = Size;
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetAutoWrapText(true);
	return Block;
}

UWyrmButton* UWyrmHUDWidget::MakeButton(const FString& Label, EWyrmAction Action, const FString& Payload, int32 Index,
	bool bEnabled, const FLinearColor& Color)
{
	UWyrmButton* Button = WidgetTree->ConstructWidget<UWyrmButton>(UWyrmButton::StaticClass());
	Button->Configure(Action, Payload, Index);
	Button->OnWyrmClicked.BindUObject(this, &UWyrmHUDWidget::HandleButton);
	Button->SetBackgroundColor(Color);
	Button->SetIsEnabled(bEnabled);
	UTextBlock* Text = MakeText(Label, 15, FLinearColor::White);
	Text->SetAutoWrapText(false);
	Button->AddChild(Text);
	return Button;
}

void UWyrmHUDWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	// Top bar.
	{
		UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Bar->SetBrushColor(ColPanel);
		Bar->SetPadding(FMargin(16.f, 8.f));
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Bar->SetContent(Row);
		GoldText = MakeText(TEXT("Gold"), 20, ColGold);
		FoodText = MakeText(TEXT("Food"), 20, FLinearColor(0.55f, 0.95f, 0.45f));
		GemsText = MakeText(TEXT("Gems"), 20, FLinearColor(0.85f, 0.55f, 1.f));
		LevelText = MakeText(TEXT("Level"), 20, FLinearColor::White);
		for (UTextBlock* Text : {GoldText.Get(), FoodText.Get(), GemsText.Get(), LevelText.Get()})
		{
			Text->SetAutoWrapText(false);
			UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text);
			TextSlot->SetPadding(FMargin(0.f, 0.f, 36.f, 0.f));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Bar);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		CanvasSlot->SetOffsets(FMargin(16.f, 12.f, 16.f, 52.f));
	}

	// Left menu.
	{
		UBorder* Menu = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Menu->SetBrushColor(ColPanel);
		Menu->SetPadding(FMargin(10.f));
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Menu->SetContent(Column);
		struct FEntry
		{
			const TCHAR* Label;
			EWyrmAction Action;
			EWyrmPanel Target;
		};
		const FEntry Entries[] = {
			{TEXT("Collect all gold"), EWyrmAction::CollectAll, EWyrmPanel::None},
			{TEXT("Build"), EWyrmAction::Panel, EWyrmPanel::Build},
			{TEXT("Dragons"), EWyrmAction::Panel, EWyrmPanel::Dragons},
			{TEXT("Breed"), EWyrmAction::Panel, EWyrmPanel::Breed},
			{TEXT("Hatchery"), EWyrmAction::Panel, EWyrmPanel::Hatchery},
			{TEXT("Shop"), EWyrmAction::Panel, EWyrmPanel::Shop},
			{TEXT("Arena"), EWyrmAction::Panel, EWyrmPanel::Arena},
			{TEXT("Dragonpedia"), EWyrmAction::Panel, EWyrmPanel::Book},
			{TEXT("Menu"), EWyrmAction::Panel, EWyrmPanel::Menu},
		};
		for (const FEntry& Entry : Entries)
		{
			UWyrmButton* Button = MakeButton(Entry.Label, Entry.Action, FString(), static_cast<int32>(Entry.Target), true,
				Entry.Action == EWyrmAction::CollectAll ? ColGood : ColButton);
			UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(Button);
			ButtonSlot->SetPadding(FMargin(0.f, 3.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Menu);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(16.f, 20.f));
		CanvasSlot->SetAutoSize(true);
	}

	// Right panel.
	{
		PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		PanelBorder->SetBrushColor(ColPanel);
		PanelBorder->SetPadding(FMargin(14.f));
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		PanelBorder->SetContent(Scroll);
		PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Scroll->AddChild(PanelBox);
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(PanelBorder);
		// Anchored to the right edge, stretched vertically: offsets are Left, Top, Width, Bottom margin.
		CanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(-476.f, 76.f, 460.f, 16.f));
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Toast.
	{
		ToastBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		ToastBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		ToastBorder->SetPadding(FMargin(18.f, 10.f));
		ToastText = MakeText(FString(), 18, FLinearColor::White);
		ToastText->SetAutoWrapText(false);
		ToastBorder->SetContent(ToastText);
		ToastBorder->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(ToastBorder);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.f));
		CanvasSlot->SetPosition(FVector2D(0.f, -24.f));
		CanvasSlot->SetAutoSize(true);
	}
}

void UWyrmHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this))
	{
		Game->OnStateChanged.AddUniqueDynamic(this, &UWyrmHUDWidget::HandleStateChanged);
		Game->OnMessage.AddUniqueDynamic(this, &UWyrmHUDWidget::HandleMessage);
		if (!Game->IsDataLoaded())
		{
			HandleMessage(FText::FromString(TEXT("Game data failed to load - see the Output Log.")), true);
		}
	}
	bTopBarDirty = true;
	bPanelDirty = true;
}

void UWyrmHUDWidget::NativeDestruct()
{
	if (UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this))
	{
		Game->OnStateChanged.RemoveDynamic(this, &UWyrmHUDWidget::HandleStateChanged);
		Game->OnMessage.RemoveDynamic(this, &UWyrmHUDWidget::HandleMessage);
	}
	Super::NativeDestruct();
}

void UWyrmHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ToastTimer > 0.f)
	{
		ToastTimer -= InDeltaTime;
		if (ToastTimer <= 0.f && ToastBorder)
		{
			ToastBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Once a second, update counters and timers.
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 1.f;
		bTopBarDirty = true;
		bPanelDirty = bPanelDirty || PanelHasTimers();
	}

	if (bTopBarDirty)
	{
		bTopBarDirty = false;
		RefreshTopBar();
	}
	// Rebuilding is deferred to here so a button is never destroyed inside its own click.
	if (bPanelDirty && !IsAnyButtonPressed())
	{
		bPanelDirty = false;
		RefreshPanel();
	}
}

void UWyrmHUDWidget::HandleStateChanged()
{
	bTopBarDirty = true;
	bPanelDirty = true;
}

void UWyrmHUDWidget::HandleMessage(const FText& Message, bool bIsError)
{
	if (!ToastText || !ToastBorder)
	{
		return;
	}
	ToastText->SetText(Message);
	ToastText->SetColorAndOpacity(FSlateColor(bIsError ? ColBad : FLinearColor::White));
	ToastBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	ToastTimer = 3.f;
}

void UWyrmHUDWidget::RefreshTopBar()
{
	const UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this);
	const UWyrmSaveGame* State = Game ? Game->GetState() : nullptr;
	if (!State || !GoldText)
	{
		return;
	}
	GoldText->SetText(FText::FromString(TEXT("Gold ") + WyrmText::Number(State->Gold)));
	FoodText->SetText(FText::FromString(TEXT("Food ") + WyrmText::Number(State->Food)));
	GemsText->SetText(FText::FromString(TEXT("Gems ") + WyrmText::Number(State->Gems)));
	LevelText->SetText(FText::FromString(FString::Printf(TEXT("Island Lv %d  (%s / %s XP)"), State->PlayerLevel,
		*WyrmText::Number(State->PlayerXp), *WyrmText::Number(Wyrm::PlayerXpToNextLevel(State->PlayerLevel)))));
}

bool UWyrmHUDWidget::IsAnyButtonPressed() const
{
	for (const TObjectPtr<UWyrmButton>& Button : PanelButtons)
	{
		if (Button && Button->IsPressed())
		{
			return true;
		}
	}
	return false;
}

bool UWyrmHUDWidget::PanelHasTimers() const
{
	return Panel == EWyrmPanel::Structure || Panel == EWyrmPanel::Breed || Panel == EWyrmPanel::Hatchery;
}

void UWyrmHUDWidget::ShowStructure(const FGuid& StructureId)
{
	UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this);
	const FWyrmStructure* Structure = Game ? Game->FindStructure(StructureId) : nullptr;
	if (!Structure)
	{
		return;
	}
	// Tapping a habitat or a ripe farm collects straight away - fewer taps for the most common actions.
	if (Structure->Type == EWyrmStructureType::Habitat && Game->GetHabitatGold(StructureId) > 0)
	{
		Game->CollectHabitat(StructureId);
	}
	else if (Structure->Type == EWyrmStructureType::Farm && !Structure->CropId.IsEmpty() &&
		UWyrmGameSubsystem::SecondsUntil(Structure->ReadyAtUtc) <= 0.0)
	{
		Game->HarvestFarm(StructureId);
	}
	if (Structure->Type == EWyrmStructureType::BreedingCave)
	{
		ShowPanel(EWyrmPanel::Breed);
		return;
	}
	if (Structure->Type == EWyrmStructureType::Hatchery)
	{
		ShowPanel(EWyrmPanel::Hatchery);
		return;
	}
	SelectedId = StructureId;
	ShowPanel(EWyrmPanel::Structure);
}

void UWyrmHUDWidget::ShowDragon(const FGuid& DragonId)
{
	SelectedId = DragonId;
	ShowPanel(EWyrmPanel::Dragon);
}

void UWyrmHUDWidget::ShowPanel(EWyrmPanel NewPanel)
{
	if (NewPanel != EWyrmPanel::Arena)
	{
		BattleLog.Reset();
	}
	bConfirmReset = false;
	Panel = NewPanel;
	bPanelDirty = true;
}

// ---- Panel helpers -------------------------------------------------------------------------------

void UWyrmHUDWidget::BeginPanel(const FString& Title)
{
	UHorizontalBox* Header = AddRow();
	UTextBlock* TitleText = AddRowText(Header, Title, 22, ColGold);
	TitleText->SetAutoWrapText(false);
	AddRowButton(Header, TEXT("X"), EWyrmAction::Close);
	AddSpacer();
}

UTextBlock* UWyrmHUDWidget::AddText(const FString& Text, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Block = MakeText(Text, Size, Color);
	UVerticalBoxSlot* BoxSlot = PanelBox->AddChildToVerticalBox(Block);
	BoxSlot->SetPadding(FMargin(0.f, 2.f));
	return Block;
}

UWyrmButton* UWyrmHUDWidget::AddButton(const FString& Label, EWyrmAction Action, const FString& Payload, int32 Index, bool bEnabled)
{
	const FLinearColor Color = Label.Contains(TEXT("gems")) ? ColGem : ColButton;
	UWyrmButton* Button = MakeButton(Label, Action, Payload, Index, bEnabled, Color);
	PanelButtons.Add(Button);
	UVerticalBoxSlot* BoxSlot = PanelBox->AddChildToVerticalBox(Button);
	BoxSlot->SetPadding(FMargin(0.f, 3.f));
	return Button;
}

UHorizontalBox* UWyrmHUDWidget::AddRow()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BoxSlot = PanelBox->AddChildToVerticalBox(Row);
	BoxSlot->SetPadding(FMargin(0.f, 3.f));
	return Row;
}

UTextBlock* UWyrmHUDWidget::AddRowText(UHorizontalBox* Row, const FString& Text, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Block = MakeText(Text, Size, Color);
	UHorizontalBoxSlot* BoxSlot = Row->AddChildToHorizontalBox(Block);
	BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BoxSlot->SetVerticalAlignment(VAlign_Center);
	BoxSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	return Block;
}

UWyrmButton* UWyrmHUDWidget::AddRowButton(UHorizontalBox* Row, const FString& Label, EWyrmAction Action, const FString& Payload,
	int32 Index, bool bEnabled)
{
	const FLinearColor Color = Label.Contains(TEXT("gems")) ? ColGem : ColButton;
	UWyrmButton* Button = MakeButton(Label, Action, Payload, Index, bEnabled, Color);
	PanelButtons.Add(Button);
	UHorizontalBoxSlot* BoxSlot = Row->AddChildToHorizontalBox(Button);
	BoxSlot->SetVerticalAlignment(VAlign_Center);
	BoxSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
	return Button;
}

void UWyrmHUDWidget::AddSpacer()
{
	AddText(FString(), 6);
}

void UWyrmHUDWidget::RefreshPanel()
{
	if (!PanelBox || !PanelBorder)
	{
		return;
	}
	PanelBox->ClearChildren();
	PanelButtons.Reset();

	UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this);
	if (Panel == EWyrmPanel::None || !Game || !Game->GetState())
	{
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PanelBorder->SetVisibility(ESlateVisibility::Visible);

	switch (Panel)
	{
	case EWyrmPanel::Structure:
		BuildStructurePanel(*Game);
		break;
	case EWyrmPanel::Dragon:
		BuildDragonPanel(*Game);
		break;
	case EWyrmPanel::Build:
		BuildBuildPanel(*Game);
		break;
	case EWyrmPanel::Dragons:
		BuildDragonsPanel(*Game);
		break;
	case EWyrmPanel::Breed:
		BuildBreedPanel(*Game);
		break;
	case EWyrmPanel::Hatchery:
		BuildHatcheryPanel(*Game);
		break;
	case EWyrmPanel::Shop:
		BuildShopPanel(*Game);
		break;
	case EWyrmPanel::Arena:
		BuildArenaPanel(*Game);
		break;
	case EWyrmPanel::Book:
		BuildBookPanel(*Game);
		break;
	case EWyrmPanel::Menu:
		BuildMenuPanel(*Game);
		break;
	default:
		break;
	}
}

// ---- Panels --------------------------------------------------------------------------------------

void UWyrmHUDWidget::BuildStructurePanel(UWyrmGameSubsystem& Game)
{
	const FWyrmStructure* Structure = Game.FindStructure(SelectedId);
	if (!Structure)
	{
		Panel = EWyrmPanel::None;
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const Wyrm::Catalog& Data = Game.GetCatalog();

	if (Structure->Type == EWyrmStructureType::Habitat)
	{
		const Wyrm::HabitatDef* Def = Data.FindHabitat(WyrmText::ToStd(Structure->DefId));
		if (!Def)
		{
			return;
		}
		BeginPanel(WyrmText::ToF(Def->Name));
		const int64 Gold = Game.GetHabitatGold(SelectedId);
		AddText(FString::Printf(TEXT("Gold: %s / %s"), *WyrmText::Number(Gold), *WyrmText::Number(Def->GoldCap)), 18, ColGold);
		AddText(FString::Printf(TEXT("Earning %d gold per minute"), Game.GetHabitatGoldPerMinute(SelectedId)), 14, ColMuted);
		AddButton(FString::Printf(TEXT("Collect %s gold"), *WyrmText::Number(Gold)), EWyrmAction::Collect, WyrmGuid(SelectedId), 0, Gold > 0);
		AddSpacer();
		const TArray<const FWyrmDragon*> Residents = Game.GetDragonsIn(SelectedId);
		AddText(FString::Printf(TEXT("Dragons (%d / %d)"), Residents.Num(), Def->Capacity), 18);
		for (const FWyrmDragon* Dragon : Residents)
		{
			UHorizontalBox* Row = AddRow();
			AddRowText(Row, FString::Printf(TEXT("%s  Lv %d"), *Game.GetDisplayName(*Dragon), Dragon->Level));
			AddRowButton(Row, TEXT("Open"), EWyrmAction::ShowDragon, WyrmGuid(Dragon->Id));
		}
		if (Residents.Num() == 0)
		{
			AddText(TEXT("Empty. Hatch or move a dragon of this element here."), 14, ColMuted);
		}
		return;
	}

	if (Structure->Type == EWyrmStructureType::Farm)
	{
		BeginPanel(TEXT("Farm"));
		const Wyrm::CropDef* Crop = Data.FindCrop(WyrmText::ToStd(Structure->CropId));
		if (!Crop)
		{
			AddText(TEXT("Plant a crop to grow food for your dragons."), 14, ColMuted);
			for (const Wyrm::CropDef& Option : Data.Crops)
			{
				UHorizontalBox* Row = AddRow();
				AddRowText(Row, FString::Printf(TEXT("%s\n%s food in %s"), *WyrmText::ToF(Option.Name),
					*WyrmText::Number(Option.Food), *WyrmText::Duration(Option.GrowSeconds)), 14);
				AddRowButton(Row, FString::Printf(TEXT("%s gold"), *WyrmText::Number(Option.Cost)), EWyrmAction::Plant,
					WyrmText::ToF(Option.Id), 0, Game.GetState()->Gold >= Option.Cost);
			}
			return;
		}
		const double Remaining = UWyrmGameSubsystem::SecondsUntil(Structure->ReadyAtUtc);
		AddText(FString::Printf(TEXT("Growing %s (+%s food)"), *WyrmText::ToF(Crop->Name), *WyrmText::Number(Crop->Food)), 18);
		if (Remaining > 0.0)
		{
			AddText(TEXT("Ready in ") + WyrmText::Duration(Remaining), 16, ColMuted);
			const int64 Gems = Game.GemCostFor(Structure->ReadyAtUtc);
			AddButton(FString::Printf(TEXT("Finish now (%lld gems)"), static_cast<long long>(Gems)), EWyrmAction::SpeedFarm, WyrmGuid(SelectedId));
		}
		else
		{
			AddButton(TEXT("Harvest"), EWyrmAction::Harvest, WyrmGuid(SelectedId));
		}
	}
}

void UWyrmHUDWidget::BuildDragonPanel(UWyrmGameSubsystem& Game)
{
	const FWyrmDragon* Dragon = Game.FindDragon(SelectedId);
	const Wyrm::SpeciesDef* Species = Dragon ? Game.FindSpeciesOf(*Dragon) : nullptr;
	if (!Dragon || !Species)
	{
		ShowPanel(EWyrmPanel::Dragons);
		return;
	}
	const Wyrm::Catalog& Data = Game.GetCatalog();
	const UWyrmSaveGame* State = Game.GetState();

	BeginPanel(Game.GetDisplayName(*Dragon));
	AddText(WyrmSpeciesLine(*Species), 15, WyrmRarityColor(Species->Rar));
	AddText(FString::Printf(TEXT("Level %d / %d"), Dragon->Level, Wyrm::MaxDragonLevel), 18);
	AddText(FString::Printf(TEXT("Health %d   Attack %d   Gold %d/min"), Wyrm::ScaledHealth(*Species, Dragon->Level),
		Wyrm::ScaledAttack(*Species, Dragon->Level), Wyrm::ScaledGoldPerMinute(*Species, Dragon->Level)), 14, ColMuted);

	if (Dragon->Level < Wyrm::MaxDragonLevel)
	{
		const int64 Food = Wyrm::FoodToNextLevel(Dragon->Level);
		AddButton(FString::Printf(TEXT("Feed: %s food"), *WyrmText::Number(Food)), EWyrmAction::Feed, WyrmGuid(SelectedId), 0, State->Food >= Food);
	}
	if (Dragon->Level < Wyrm::BreedMinLevel)
	{
		AddText(FString::Printf(TEXT("Can breed from level %d."), Wyrm::BreedMinLevel), 14, ColMuted);
	}

	AddSpacer();
	AddText(TEXT("Skills"), 18);
	const int32 Unlocked = Wyrm::UnlockedSkillCount(Dragon->Level, static_cast<int32>(Species->Skills.size()));
	for (int32 Index = 0; Index < static_cast<int32>(Species->Skills.size()); ++Index)
	{
		const Wyrm::SkillDef* Skill = Data.FindSkill(Species->Skills[static_cast<size_t>(Index)]);
		if (!Skill)
		{
			continue;
		}
		const int32 UnlockLevel = Index < 2 ? 1 : (Index == 2 ? 10 : 20);
		const FString Line = FString::Printf(TEXT("%s  (%s, power %d, %d%% accuracy)%s"), *WyrmText::ToF(Skill->Name),
			ANSI_TO_TCHAR(Wyrm::ElementName(Skill->Elem)), Skill->Power, Skill->Accuracy,
			Index < Unlocked ? TEXT("") : *FString::Printf(TEXT(" - unlocks at Lv %d"), UnlockLevel));
		AddText(Line, 14, Index < Unlocked ? FLinearColor::White : ColMuted);
	}

	// Move to another compatible habitat.
	AddSpacer();
	AddText(TEXT("Move to"), 18);
	bool bAnyHome = false;
	for (const FWyrmStructure& S : State->Structures)
	{
		const Wyrm::HabitatDef* Def = S.Type == EWyrmStructureType::Habitat ? Data.FindHabitat(WyrmText::ToStd(S.DefId)) : nullptr;
		if (!Def || S.Id == Dragon->HabitatId || !Wyrm::CanLiveIn(*Species, *Def))
		{
			continue;
		}
		const int32 Used = Game.GetDragonsIn(S.Id).Num();
		AddButton(FString::Printf(TEXT("%s (%d/%d)"), *WyrmText::ToF(Def->Name), Used, Def->Capacity), EWyrmAction::Move,
			WyrmGuid(SelectedId) + TEXT("|") + WyrmGuid(S.Id), 0, Used < Def->Capacity);
		bAnyHome = true;
	}
	if (!bAnyHome)
	{
		AddText(TEXT("No other habitat fits this dragon."), 14, ColMuted);
	}

	AddSpacer();
	AddButton(FString::Printf(TEXT("Sell for %s gold"), *WyrmText::Number(Wyrm::SellValue(*Species, Dragon->Level))), EWyrmAction::Sell,
		WyrmGuid(SelectedId));
}

void UWyrmHUDWidget::BuildBuildPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	BeginPanel(TEXT("Build"));
	AddText(FString::Printf(TEXT("Buildings: %d / %d (raise island level for more)"), State->Structures.Num(), Game.GetMaxStructures()),
		14, ColMuted);

	UHorizontalBox* FarmRow = AddRow();
	AddRowText(FarmRow, TEXT("Farm\nGrows food to level up dragons"), 14);
	AddRowButton(FarmRow, FString::Printf(TEXT("%s gold"), *WyrmText::Number(Game.GetSetup().FarmCost)), EWyrmAction::BuildFarm, FString(), 0,
		State->Gold >= Game.GetSetup().FarmCost);

	for (const Wyrm::HabitatDef& Def : Game.GetCatalog().Habitats)
	{
		UHorizontalBox* Row = AddRow();
		AddRowText(Row, FString::Printf(TEXT("%s\nHolds %d dragons, stores %s gold"), *WyrmText::ToF(Def.Name), Def.Capacity,
			*WyrmText::Number(Def.GoldCap)), 14);
		AddRowButton(Row, FString::Printf(TEXT("%s gold"), *WyrmText::Number(Def.Cost)), EWyrmAction::Build, WyrmText::ToF(Def.Id), 0,
			State->Gold >= Def.Cost);
	}
}

void UWyrmHUDWidget::BuildDragonsPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	BeginPanel(FString::Printf(TEXT("Dragons (%d)"), State->Dragons.Num()));
	TArray<const FWyrmDragon*> Sorted;
	for (const FWyrmDragon& Dragon : State->Dragons)
	{
		Sorted.Add(&Dragon);
	}
	Sorted.StableSort([](const FWyrmDragon& L, const FWyrmDragon& R) { return L.Level > R.Level; });
	for (const FWyrmDragon* Dragon : Sorted)
	{
		const Wyrm::SpeciesDef* Species = Game.FindSpeciesOf(*Dragon);
		if (!Species)
		{
			continue;
		}
		UHorizontalBox* Row = AddRow();
		AddRowText(Row, FString::Printf(TEXT("%s  Lv %d\n%s"), *Game.GetDisplayName(*Dragon), Dragon->Level,
			*WyrmText::Elements(Species->Elements)), 14, WyrmRarityColor(Species->Rar));
		const int64 Food = Wyrm::FoodToNextLevel(Dragon->Level);
		AddRowButton(Row, FString::Printf(TEXT("Feed %s"), *WyrmText::Number(Food)), EWyrmAction::Feed, WyrmGuid(Dragon->Id), 0,
			Dragon->Level < Wyrm::MaxDragonLevel && State->Food >= Food);
		AddRowButton(Row, TEXT("Open"), EWyrmAction::ShowDragon, WyrmGuid(Dragon->Id));
	}
}

void UWyrmHUDWidget::BuildBreedPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	const Wyrm::Catalog& Data = Game.GetCatalog();
	BeginPanel(TEXT("Breeding Cave"));

	if (State->Breeding.bActive)
	{
		const FWyrmDragon* A = Game.FindDragon(State->Breeding.ParentA);
		const FWyrmDragon* B = Game.FindDragon(State->Breeding.ParentB);
		AddText(FString::Printf(TEXT("%s + %s"), A ? *Game.GetDisplayName(*A) : TEXT("?"), B ? *Game.GetDisplayName(*B) : TEXT("?")), 18);
		const double Remaining = UWyrmGameSubsystem::SecondsUntil(State->Breeding.ReadyAtUtc);
		if (Remaining > 0.0)
		{
			AddText(TEXT("Egg ready in ") + WyrmText::Duration(Remaining), 16, ColMuted);
			AddButton(FString::Printf(TEXT("Finish now (%lld gems)"), static_cast<long long>(Game.GemCostFor(State->Breeding.ReadyAtUtc))),
				EWyrmAction::SpeedBreed);
		}
		else
		{
			AddButton(TEXT("Collect egg"), EWyrmAction::CollectEgg, FString(), 0, State->Eggs.Num() < Wyrm::MaxEggsInHatchery);
			if (State->Eggs.Num() >= Wyrm::MaxEggsInHatchery)
			{
				AddText(TEXT("The hatchery is full - hatch an egg first."), 14, ColBad);
			}
		}
		return;
	}

	AddText(FString::Printf(TEXT("Pick two dragons (level %d+). Their elements decide what can hatch."), Wyrm::BreedMinLevel), 14,
		ColMuted);
	if (State->BreedPity > 0)
	{
		AddText(FString::Printf(TEXT("Luck bonus: +%d%% Epic & Legendary odds"), State->BreedPity * 25), 14, ColGold);
	}

	// Drop selections that are no longer valid (sold, etc.).
	if (!Game.FindDragon(BreedParentA))
	{
		BreedParentA.Invalidate();
	}
	if (!Game.FindDragon(BreedParentB))
	{
		BreedParentB.Invalidate();
	}

	if (BreedParentA.IsValid() && BreedParentB.IsValid())
	{
		FString Error;
		const TArray<FWyrmBreedOddsEntry> Odds = Game.GetBreedOdds(BreedParentA, BreedParentB, Error);
		AddSpacer();
		if (Odds.Num() == 0)
		{
			AddText(Error, 15, ColBad);
		}
		else
		{
			AddText(TEXT("Possible offspring"), 18);
			for (const FWyrmBreedOddsEntry& Entry : Odds)
			{
				const Wyrm::SpeciesDef* Species = Data.FindSpecies(WyrmText::ToStd(Entry.SpeciesId));
				const bool bKnown = State->Discovered.Contains(Entry.SpeciesId);
				AddText(FString::Printf(TEXT("%5.1f%%   %s (%s)%s"), Entry.Probability * 100.f, *Entry.Name, *Entry.Rarity,
					bKnown ? TEXT("") : TEXT("  NEW!")), 15, Species ? WyrmRarityColor(Species->Rar) : FLinearColor::White);
			}
			AddButton(TEXT("Breed!"), EWyrmAction::Breed);
		}
	}

	AddSpacer();
	AddText(TEXT("Your dragons"), 18);
	for (const FWyrmDragon& Dragon : State->Dragons)
	{
		const Wyrm::SpeciesDef* Species = Game.FindSpeciesOf(Dragon);
		if (!Species)
		{
			continue;
		}
		const bool bReady = Dragon.Level >= Wyrm::BreedMinLevel;
		const bool bIsA = Dragon.Id == BreedParentA;
		const bool bIsB = Dragon.Id == BreedParentB;
		UHorizontalBox* Row = AddRow();
		AddRowText(Row, FString::Printf(TEXT("%s%s  Lv %d\n%s"), bIsA ? TEXT("[1] ") : (bIsB ? TEXT("[2] ") : TEXT("")),
			*Game.GetDisplayName(Dragon), Dragon.Level, *WyrmText::Elements(Species->Elements)), 14,
			bReady ? WyrmRarityColor(Species->Rar) : ColMuted);
		AddRowButton(Row, (bIsA || bIsB) ? TEXT("Remove") : TEXT("Pick"), EWyrmAction::PickParent, WyrmGuid(Dragon.Id), 0, bReady);
	}
}

void UWyrmHUDWidget::BuildHatcheryPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	BeginPanel(FString::Printf(TEXT("Hatchery (%d / %d)"), State->Eggs.Num(), Wyrm::MaxEggsInHatchery));
	if (State->Eggs.Num() == 0)
	{
		AddText(TEXT("No eggs. Breed dragons or buy eggs in the shop."), 14, ColMuted);
	}
	for (const FWyrmEgg& Egg : State->Eggs)
	{
		const Wyrm::SpeciesDef* Species = Game.GetCatalog().FindSpecies(WyrmText::ToStd(Egg.SpeciesId));
		if (!Species)
		{
			continue;
		}
		const double Remaining = UWyrmGameSubsystem::SecondsUntil(Egg.HatchAtUtc);
		UHorizontalBox* Row = AddRow();
		AddRowText(Row, FString::Printf(TEXT("%s egg\n%s"), *WyrmText::ToF(Species->Name),
			Remaining > 0.0 ? *WyrmText::Duration(Remaining) : TEXT("Ready!")), 15, WyrmRarityColor(Species->Rar));
		if (Remaining > 0.0)
		{
			AddRowButton(Row, FString::Printf(TEXT("%lld gems"), static_cast<long long>(Game.GemCostFor(Egg.HatchAtUtc))), EWyrmAction::SpeedEgg,
				WyrmGuid(Egg.Id));
		}
		else
		{
			AddRowButton(Row, TEXT("Hatch"), EWyrmAction::Hatch, WyrmGuid(Egg.Id));
		}
	}
}

void UWyrmHUDWidget::BuildShopPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	BeginPanel(TEXT("Egg Shop"));
	AddText(TEXT("Buy common eggs here. Rarer dragons can only be bred."), 14, ColMuted);
	for (const Wyrm::SpeciesDef& Species : Game.GetCatalog().Species)
	{
		if (Species.ShopGold <= 0)
		{
			continue;
		}
		UHorizontalBox* Row = AddRow();
		AddRowText(Row, FString::Printf(TEXT("%s\n%s, hatches in %s"), *WyrmText::ToF(Species.Name),
			*WyrmText::Elements(Species.Elements), *WyrmText::Duration(Species.HatchSeconds)), 14);
		AddRowButton(Row, FString::Printf(TEXT("%s gold"), *WyrmText::Number(Species.ShopGold)), EWyrmAction::Buy, WyrmText::ToF(Species.Id), 0,
			State->Gold >= Species.ShopGold && State->Eggs.Num() < Wyrm::MaxEggsInHatchery);
	}
}

void UWyrmHUDWidget::BuildArenaPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	BeginPanel(FString::Printf(TEXT("Arena - Tier %d"), State->ArenaTier));

	const Wyrm::Battle* Battle = Game.GetBattle();
	if (!Battle)
	{
		AddText(TEXT("Your strongest three dragons fight in order. Element matchups matter: water beats fire, "
			"fire beats nature and ice, and so on."), 14, ColMuted);
		AddText(TEXT("Your team"), 18);
		for (const FWyrmDragon* Dragon : Game.GetArenaTeam())
		{
			AddText(FString::Printf(TEXT("%s  Lv %d"), *Game.GetDisplayName(*Dragon), Dragon->Level), 15);
		}
		AddButton(TEXT("Fight!"), EWyrmAction::Fight);
		return;
	}

	for (int32 Side = 1; Side >= 0; --Side)
	{
		const Wyrm::Combatant* Active = Battle->Active(Side);
		if (!Active)
		{
			continue;
		}
		int32 Remaining = 0;
		for (const Wyrm::Combatant& C : Battle->Team(Side))
		{
			Remaining += C.IsFainted() ? 0 : 1;
		}
		AddText(FString::Printf(TEXT("%s%s  Lv %d  (%s)"), Side == 0 ? TEXT("You: ") : TEXT("Foe: "), *WyrmText::ToF(Active->Name),
			Active->Level, *WyrmText::Elements(Active->Elements)), 16, Side == 0 ? FLinearColor(0.6f, 0.9f, 1.f) : ColBad);
		AddText(FString::Printf(TEXT("%s %d/%d   (%d left)"), *WyrmHealthBar(Active->Health, Active->MaxHealth), FMath::Max(0, Active->Health),
			Active->MaxHealth, Remaining), 14, ColMuted);
	}

	AddSpacer();
	if (!Battle->IsOver())
	{
		const Wyrm::Combatant* Me = Battle->Active(0);
		const Wyrm::Combatant* Foe = Battle->Active(1);
		if (Me && Foe)
		{
			for (int32 Index = 0; Index < static_cast<int32>(Me->Skills.size()); ++Index)
			{
				const Wyrm::SkillDef& Skill = Me->Skills[static_cast<size_t>(Index)];
				const float Eff = Wyrm::Effectiveness(Skill.Elem, Foe->Elements);
				const TCHAR* Hint = Eff > 1.01f ? TEXT("  - strong!") : (Eff < 0.99f ? TEXT("  - weak") : TEXT(""));
				AddButton(FString::Printf(TEXT("%s (%s, %d)%s"), *WyrmText::ToF(Skill.Name), ANSI_TO_TCHAR(Wyrm::ElementName(Skill.Elem)),
					Skill.Power, Hint), EWyrmAction::Skill, FString(), Index);
			}
		}
	}
	else
	{
		AddText(Battle->Winner() == 0 ? TEXT("Victory!") : TEXT("Defeated"), 22, Battle->Winner() == 0 ? ColGold : ColBad);
		AddButton(TEXT("Leave arena"), EWyrmAction::LeaveArena);
	}

	AddSpacer();
	const int32 First = FMath::Max(0, BattleLog.Num() - 8);
	for (int32 Index = BattleLog.Num() - 1; Index >= First; --Index)
	{
		AddText(BattleLog[Index], 13, Index == BattleLog.Num() - 1 ? FLinearColor::White : ColMuted);
	}
}

void UWyrmHUDWidget::BuildBookPanel(UWyrmGameSubsystem& Game)
{
	const UWyrmSaveGame* State = Game.GetState();
	const Wyrm::Catalog& Data = Game.GetCatalog();
	BeginPanel(FString::Printf(TEXT("Dragonpedia (%d / %d)"), State->Discovered.Num(), static_cast<int32>(Data.Species.size())));
	for (const Wyrm::SpeciesDef& Species : Data.Species)
	{
		const bool bKnown = State->Discovered.Contains(WyrmText::ToF(Species.Id));
		AddText(WyrmSpeciesLine(Species), 15, bKnown ? WyrmRarityColor(Species.Rar) : ColMuted);
		if (Species.HasRecipe())
		{
			const Wyrm::SpeciesDef* A = Data.FindSpecies(Species.RecipeA);
			const Wyrm::SpeciesDef* B = Data.FindSpecies(Species.RecipeB);
			AddText(FString::Printf(TEXT("    Breed %s with %s"), A ? *WyrmText::ToF(A->Name) : TEXT("?"), B ? *WyrmText::ToF(B->Name) : TEXT("?")),
				13, ColMuted);
		}
	}
}

void UWyrmHUDWidget::BuildMenuPanel(UWyrmGameSubsystem& Game)
{
	BeginPanel(TEXT("Menu"));
	AddText(TEXT("The game saves automatically every 20 seconds."), 14, ColMuted);
	AddButton(TEXT("Save now"), EWyrmAction::Save);
	AddSpacer();
	AddButton(bConfirmReset ? TEXT("Tap again to erase everything") : TEXT("Start a new island"), EWyrmAction::Reset);
	if (!Game.IsDataLoaded())
	{
		AddSpacer();
		AddText(Game.GetDataError(), 13, ColBad);
	}
}

// ---- Actions -------------------------------------------------------------------------------------

void UWyrmHUDWidget::HandleButton(UWyrmButton* Button)
{
	UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this);
	if (!Button || !Game || !Game->GetState())
	{
		return;
	}
	const EWyrmAction Action = Button->Action;
	const FString& Payload = Button->Payload;

	if (Action == EWyrmAction::Close)
	{
		ClosePanel();
	}
	else if (Action == EWyrmAction::Panel)
	{
		const EWyrmPanel Target = static_cast<EWyrmPanel>(Button->Index);
		ShowPanel(Panel == Target ? EWyrmPanel::None : Target); // menu buttons toggle
	}
	else if (Action == EWyrmAction::CollectAll)
	{
		if (Game->CollectAllGold() <= 0)
		{
			HandleMessage(FText::FromString(TEXT("Nothing to collect yet.")), false);
		}
	}
	else if (Action == EWyrmAction::Collect)
	{
		Game->CollectHabitat(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::Plant)
	{
		Game->PlantCrop(SelectedId, Payload);
	}
	else if (Action == EWyrmAction::Harvest)
	{
		Game->HarvestFarm(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::SpeedFarm)
	{
		Game->SpeedUpFarm(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::ShowDragon)
	{
		ShowDragon(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::Feed)
	{
		Game->FeedDragon(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::Sell)
	{
		if (Game->SellDragon(WyrmParseGuid(Payload)))
		{
			ShowPanel(EWyrmPanel::Dragons);
		}
	}
	else if (Action == EWyrmAction::Move)
	{
		FString DragonText;
		FString HabitatText;
		if (Payload.Split(TEXT("|"), &DragonText, &HabitatText))
		{
			Game->MoveDragon(WyrmParseGuid(DragonText), WyrmParseGuid(HabitatText));
		}
	}
	else if (Action == EWyrmAction::Build)
	{
		Game->BuildHabitat(Payload);
	}
	else if (Action == EWyrmAction::BuildFarm)
	{
		Game->BuildFarm();
	}
	else if (Action == EWyrmAction::PickParent)
	{
		const FGuid Id = WyrmParseGuid(Payload);
		if (Id == BreedParentA)
		{
			BreedParentA.Invalidate();
		}
		else if (Id == BreedParentB)
		{
			BreedParentB.Invalidate();
		}
		else if (!BreedParentA.IsValid())
		{
			BreedParentA = Id;
		}
		else
		{
			BreedParentB = Id;
		}
		bPanelDirty = true;
	}
	else if (Action == EWyrmAction::Breed)
	{
		if (Game->StartBreeding(BreedParentA, BreedParentB))
		{
			BreedParentA.Invalidate();
			BreedParentB.Invalidate();
		}
	}
	else if (Action == EWyrmAction::SpeedBreed)
	{
		Game->SpeedUpBreeding();
	}
	else if (Action == EWyrmAction::CollectEgg)
	{
		Game->CollectBreedingEgg();
	}
	else if (Action == EWyrmAction::Hatch)
	{
		Game->HatchEgg(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::SpeedEgg)
	{
		Game->SpeedUpEgg(WyrmParseGuid(Payload));
	}
	else if (Action == EWyrmAction::Buy)
	{
		Game->BuyEgg(Payload);
	}
	else if (Action == EWyrmAction::Fight)
	{
		BattleLog.Reset();
		Game->StartArenaBattle();
	}
	else if (Action == EWyrmAction::Skill)
	{
		BattleLog.Append(Game->BattleUseSkill(Button->Index));
	}
	else if (Action == EWyrmAction::LeaveArena)
	{
		BattleLog.Reset();
		Game->EndBattle();
	}
	else if (Action == EWyrmAction::Save)
	{
		Game->SaveNow();
		HandleMessage(FText::FromString(TEXT("Saved.")), false);
	}
	else if (Action == EWyrmAction::Reset)
	{
		if (bConfirmReset)
		{
			bConfirmReset = false;
			BreedParentA.Invalidate();
			BreedParentB.Invalidate();
			Game->ResetProgress();
			ClosePanel();
		}
		else
		{
			bConfirmReset = true;
			bPanelDirty = true;
		}
	}
}
