// Wyrmhaven - a UMG button that remembers which action it triggers and for what.
#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"

#include "WyrmButton.generated.h"

class UWyrmButton;

/** Everything a HUD button can do. */
enum class EWyrmAction : uint8
{
	Close,
	Panel,
	Collect,
	CollectAll,
	Plant,
	Harvest,
	SpeedFarm,
	ShowDragon,
	Feed,
	Sell,
	Move,
	Build,
	BuildFarm,
	PickParent,
	Breed,
	SpeedBreed,
	CollectEgg,
	Hatch,
	SpeedEgg,
	Buy,
	Fight,
	Skill,
	LeaveArena,
	Save,
	Reset
};

DECLARE_DELEGATE_OneParam(FWyrmButtonClicked, UWyrmButton*);

UCLASS()
class WYRMHAVEN_API UWyrmButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(EWyrmAction InAction, const FString& InPayload, int32 InIndex);

	EWyrmAction Action = EWyrmAction::Close;
	FString Payload;
	int32 Index = 0;

	FWyrmButtonClicked OnWyrmClicked;

private:
	UFUNCTION()
	void HandleClicked();
};
