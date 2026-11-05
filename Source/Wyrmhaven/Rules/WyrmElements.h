// Wyrmhaven - the ten elements and the type chart.
#pragma once

#include <cstdint>
#include <string>

namespace Wyrm
{
enum class Element : uint8_t
{
	Fire,
	Water,
	Earth,
	Air,
	Nature,
	Lightning,
	Ice,
	Metal,
	Shadow,
	Light,
	Count
};

// A dragon has one or two elements, stored as a bit mask.
using ElementMask = uint16_t;

constexpr int ElementCount = static_cast<int>(Element::Count);

constexpr ElementMask MaskOf(Element E)
{
	return static_cast<ElementMask>(1u << static_cast<unsigned>(E));
}

constexpr bool HasElement(ElementMask Mask, Element E)
{
	return (Mask & MaskOf(E)) != 0;
}

constexpr bool IsSubsetOf(ElementMask Sub, ElementMask Super)
{
	return (Sub & ~Super) == 0;
}

int CountElements(ElementMask Mask);

// Lowest element in the mask (used for colours and the "home" habitat). Fire if empty.
Element PrimaryElement(ElementMask Mask);

// Second element in the mask, or the primary one for pure dragons.
Element SecondaryElement(ElementMask Mask);

const char* ElementName(Element E);
bool ParseElement(const std::string& Name, Element& Out);

// Each element is strong against exactly two others and weak against exactly two.
bool IsStrongAgainst(Element Attacker, Element Defender);

// 2.0 = super effective, 0.5 = resisted, 1.0 = neutral.
float Effectiveness(Element Attack, Element Defender);

// Multiplied across every element of the defender (so 0.25 .. 4.0).
float Effectiveness(Element Attack, ElementMask Defender);
} // namespace Wyrm
