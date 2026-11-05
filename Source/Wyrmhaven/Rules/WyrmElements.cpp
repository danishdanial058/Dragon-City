#include "WyrmElements.h"

namespace Wyrm
{
namespace
{
const char* const GWyrmElementNames[ElementCount] = {
	"Fire", "Water", "Earth", "Air", "Nature", "Lightning", "Ice", "Metal", "Shadow", "Light"};

// Row = attacker. Two strong matchups per element.
const Element GWyrmStrongChart[ElementCount][2] = {
	/* Fire      */ {Element::Nature, Element::Ice},
	/* Water     */ {Element::Fire, Element::Metal},
	/* Earth     */ {Element::Lightning, Element::Fire},
	/* Air       */ {Element::Earth, Element::Shadow},
	/* Nature    */ {Element::Water, Element::Earth},
	/* Lightning */ {Element::Water, Element::Air},
	/* Ice       */ {Element::Nature, Element::Air},
	/* Metal     */ {Element::Ice, Element::Light},
	/* Shadow    */ {Element::Light, Element::Lightning},
	/* Light     */ {Element::Shadow, Element::Metal},
};
} // namespace

int CountElements(ElementMask Mask)
{
	int Count = 0;
	for (int Index = 0; Index < ElementCount; ++Index)
	{
		if (HasElement(Mask, static_cast<Element>(Index)))
		{
			++Count;
		}
	}
	return Count;
}

Element PrimaryElement(ElementMask Mask)
{
	for (int Index = 0; Index < ElementCount; ++Index)
	{
		if (HasElement(Mask, static_cast<Element>(Index)))
		{
			return static_cast<Element>(Index);
		}
	}
	return Element::Fire;
}

Element SecondaryElement(ElementMask Mask)
{
	const Element First = PrimaryElement(Mask);
	for (int Index = static_cast<int>(First) + 1; Index < ElementCount; ++Index)
	{
		if (HasElement(Mask, static_cast<Element>(Index)))
		{
			return static_cast<Element>(Index);
		}
	}
	return First;
}

const char* ElementName(Element E)
{
	const int Index = static_cast<int>(E);
	return (Index >= 0 && Index < ElementCount) ? GWyrmElementNames[Index] : "Unknown";
}

bool ParseElement(const std::string& Name, Element& Out)
{
	for (int Index = 0; Index < ElementCount; ++Index)
	{
		if (Name == GWyrmElementNames[Index])
		{
			Out = static_cast<Element>(Index);
			return true;
		}
	}
	return false;
}

bool IsStrongAgainst(Element Attacker, Element Defender)
{
	const int Index = static_cast<int>(Attacker);
	if (Index < 0 || Index >= ElementCount)
	{
		return false;
	}
	return GWyrmStrongChart[Index][0] == Defender || GWyrmStrongChart[Index][1] == Defender;
}

float Effectiveness(Element Attack, Element Defender)
{
	if (IsStrongAgainst(Attack, Defender))
	{
		return 2.0f;
	}
	if (IsStrongAgainst(Defender, Attack))
	{
		return 0.5f;
	}
	return 1.0f;
}

float Effectiveness(Element Attack, ElementMask Defender)
{
	float Result = 1.0f;
	for (int Index = 0; Index < ElementCount; ++Index)
	{
		const Element E = static_cast<Element>(Index);
		if (HasElement(Defender, E))
		{
			Result *= Effectiveness(Attack, E);
		}
	}
	return Result;
}
} // namespace Wyrm
