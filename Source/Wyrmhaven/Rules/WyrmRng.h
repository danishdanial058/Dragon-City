// Wyrmhaven - deterministic random numbers (SplitMix64).
// Pure C++: no engine dependencies, so the rules can be unit tested outside Unreal.
#pragma once

#include <cstdint>

namespace Wyrm
{
class Rng
{
public:
	explicit Rng(uint64_t Seed = 0x2545F4914F6CDD1DULL) : StateValue(Seed) {}

	uint64_t NextU64()
	{
		uint64_t Z = (StateValue += 0x9E3779B97F4A7C15ULL);
		Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBULL;
		return Z ^ (Z >> 31);
	}

	// Uniform in [0, 1).
	double NextDouble() { return static_cast<double>(NextU64() >> 11) * (1.0 / 9007199254740992.0); }

	// Uniform integer in [Low, High] (inclusive).
	int RangeInt(int Low, int High)
	{
		if (High <= Low)
		{
			return Low;
		}
		const uint64_t Span = static_cast<uint64_t>(static_cast<int64_t>(High) - static_cast<int64_t>(Low)) + 1ULL;
		return static_cast<int>(static_cast<int64_t>(Low) + static_cast<int64_t>(NextU64() % Span));
	}

	uint64_t GetState() const { return StateValue; }
	void SetState(uint64_t NewState) { StateValue = NewState; }

private:
	uint64_t StateValue;
};
} // namespace Wyrm
