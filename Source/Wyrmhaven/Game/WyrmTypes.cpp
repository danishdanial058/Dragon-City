#include "Game/WyrmTypes.h"

#include "Math/UnrealMathUtility.h"

namespace WyrmText
{
FString Duration(double Seconds)
{
	const int64 Total = FMath::Max<int64>(0, static_cast<int64>(FMath::CeilToDouble(Seconds)));
	const int64 Hours = Total / 3600;
	const int64 Minutes = (Total % 3600) / 60;
	const int64 Secs = Total % 60;
	if (Hours > 0)
	{
		return FString::Printf(TEXT("%lldh %02lldm"), Hours, Minutes);
	}
	if (Minutes > 0)
	{
		return FString::Printf(TEXT("%lldm %02llds"), Minutes, Secs);
	}
	return FString::Printf(TEXT("%llds"), Secs);
}

FString Elements(Wyrm::ElementMask Mask)
{
	FString Result;
	for (int32 Index = 0; Index < Wyrm::ElementCount; ++Index)
	{
		const Wyrm::Element E = static_cast<Wyrm::Element>(Index);
		if (Wyrm::HasElement(Mask, E))
		{
			if (!Result.IsEmpty())
			{
				Result += TEXT(" / ");
			}
			Result += ANSI_TO_TCHAR(Wyrm::ElementName(E));
		}
	}
	return Result;
}

FString Number(int64 Value)
{
	return FText::AsNumber(Value).ToString();
}
} // namespace WyrmText
