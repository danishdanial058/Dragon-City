#include "UI/WyrmButton.h"

void UWyrmButton::Configure(EWyrmAction InAction, const FString& InPayload, int32 InIndex)
{
	Action = InAction;
	Payload = InPayload;
	Index = InIndex;
	OnClicked.AddUniqueDynamic(this, &UWyrmButton::HandleClicked);
}

void UWyrmButton::HandleClicked()
{
	OnWyrmClicked.ExecuteIfBound(this);
}
