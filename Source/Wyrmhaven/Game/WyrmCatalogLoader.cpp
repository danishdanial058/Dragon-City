#include "Game/WyrmCatalogLoader.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Game/WyrmTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString WyrmJsonString(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FString& Default = FString())
{
	FString Value;
	return Obj->TryGetStringField(Field, Value) ? Value : Default;
}

int32 WyrmJsonInt(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, int32 Default)
{
	double Value = 0.0;
	return Obj->TryGetNumberField(Field, Value) ? static_cast<int32>(Value) : Default;
}

TArray<FString> WyrmJsonStringArray(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field)
{
	TArray<FString> Result;
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (Obj->TryGetArrayField(Field, Values) && Values)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString Text;
			if (Value.IsValid() && Value->TryGetString(Text))
			{
				Result.Add(Text);
			}
		}
	}
	return Result;
}

bool WyrmParseElementField(const FString& Name, Wyrm::Element& Out, const FString& Context, FString& OutError)
{
	if (!Wyrm::ParseElement(WyrmText::ToStd(Name), Out))
	{
		OutError = FString::Printf(TEXT("%s: unknown element '%s'"), *Context, *Name);
		return false;
	}
	return true;
}

const TArray<TSharedPtr<FJsonValue>>& WyrmJsonArray(const TSharedPtr<FJsonObject>& Root, const TCHAR* Field)
{
	static const TArray<TSharedPtr<FJsonValue>> Empty;
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	return (Root->TryGetArrayField(Field, Values) && Values) ? *Values : Empty;
}
} // namespace

namespace WyrmCatalogLoader
{
FString DefaultDataPath()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), TEXT("GameData.json"));
}

bool ParseCatalog(const FString& JsonText, Wyrm::Catalog& OutCatalog, FWyrmStartingSetup& OutSetup, FString& OutError)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = FString::Printf(TEXT("Invalid JSON: %s"), *Reader->GetErrorMessage());
		return false;
	}

	Wyrm::Catalog Catalog;

	for (const TSharedPtr<FJsonValue>& Value : WyrmJsonArray(Root, TEXT("skills")))
	{
		const TSharedPtr<FJsonObject> Obj = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		Wyrm::SkillDef Skill;
		Skill.Id = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("id")));
		Skill.Name = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("name"), WyrmJsonString(Obj, TEXT("id"))));
		if (!WyrmParseElementField(WyrmJsonString(Obj, TEXT("element")), Skill.Elem, TEXT("skill ") + WyrmText::ToF(Skill.Id), OutError))
		{
			return false;
		}
		Skill.Power = WyrmJsonInt(Obj, TEXT("power"), Skill.Power);
		Skill.Accuracy = WyrmJsonInt(Obj, TEXT("accuracy"), Skill.Accuracy);
		Catalog.Skills.push_back(Skill);
	}

	for (const TSharedPtr<FJsonValue>& Value : WyrmJsonArray(Root, TEXT("species")))
	{
		const TSharedPtr<FJsonObject> Obj = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		Wyrm::SpeciesDef S;
		S.Id = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("id")));
		S.Name = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("name"), WyrmJsonString(Obj, TEXT("id"))));
		const FString Context = TEXT("species ") + WyrmText::ToF(S.Id);
		for (const FString& ElementName : WyrmJsonStringArray(Obj, TEXT("elements")))
		{
			Wyrm::Element E = Wyrm::Element::Fire;
			if (!WyrmParseElementField(ElementName, E, Context, OutError))
			{
				return false;
			}
			S.Elements = static_cast<Wyrm::ElementMask>(S.Elements | Wyrm::MaskOf(E));
		}
		const FString RarityName = WyrmJsonString(Obj, TEXT("rarity"), TEXT("Common"));
		if (!Wyrm::ParseRarity(WyrmText::ToStd(RarityName), S.Rar))
		{
			OutError = FString::Printf(TEXT("%s: unknown rarity '%s'"), *Context, *RarityName);
			return false;
		}
		S.BaseHealth = WyrmJsonInt(Obj, TEXT("baseHealth"), S.BaseHealth);
		S.BaseAttack = WyrmJsonInt(Obj, TEXT("baseAttack"), S.BaseAttack);
		S.GoldPerMinute = WyrmJsonInt(Obj, TEXT("goldPerMinute"), S.GoldPerMinute);
		S.BreedSeconds = WyrmJsonInt(Obj, TEXT("breedSeconds"), S.BreedSeconds);
		S.HatchSeconds = WyrmJsonInt(Obj, TEXT("hatchSeconds"), S.HatchSeconds);
		S.ShopGold = WyrmJsonInt(Obj, TEXT("shopGold"), 0);
		bool bBreedable = true;
		Obj->TryGetBoolField(TEXT("breedable"), bBreedable);
		S.Breedable = bBreedable;
		const TArray<FString> Recipe = WyrmJsonStringArray(Obj, TEXT("recipe"));
		if (Recipe.Num() == 2)
		{
			S.RecipeA = WyrmText::ToStd(Recipe[0]);
			S.RecipeB = WyrmText::ToStd(Recipe[1]);
		}
		for (const FString& SkillId : WyrmJsonStringArray(Obj, TEXT("skills")))
		{
			S.Skills.push_back(WyrmText::ToStd(SkillId));
		}
		S.Mesh = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("mesh")));
		Catalog.Species.push_back(S);
	}

	for (const TSharedPtr<FJsonValue>& Value : WyrmJsonArray(Root, TEXT("habitats")))
	{
		const TSharedPtr<FJsonObject> Obj = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		Wyrm::HabitatDef H;
		H.Id = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("id")));
		H.Name = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("name"), WyrmJsonString(Obj, TEXT("id"))));
		if (!WyrmParseElementField(WyrmJsonString(Obj, TEXT("element")), H.Elem, TEXT("habitat ") + WyrmText::ToF(H.Id), OutError))
		{
			return false;
		}
		H.Capacity = WyrmJsonInt(Obj, TEXT("capacity"), H.Capacity);
		H.GoldCap = WyrmJsonInt(Obj, TEXT("goldCap"), H.GoldCap);
		H.Cost = WyrmJsonInt(Obj, TEXT("cost"), H.Cost);
		Catalog.Habitats.push_back(H);
	}

	for (const TSharedPtr<FJsonValue>& Value : WyrmJsonArray(Root, TEXT("crops")))
	{
		const TSharedPtr<FJsonObject> Obj = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		Wyrm::CropDef C;
		C.Id = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("id")));
		C.Name = WyrmText::ToStd(WyrmJsonString(Obj, TEXT("name"), WyrmJsonString(Obj, TEXT("id"))));
		C.Cost = WyrmJsonInt(Obj, TEXT("cost"), C.Cost);
		C.Food = WyrmJsonInt(Obj, TEXT("food"), C.Food);
		C.GrowSeconds = WyrmJsonInt(Obj, TEXT("growSeconds"), C.GrowSeconds);
		Catalog.Crops.push_back(C);
	}

	const std::vector<std::string> Problems = Catalog.Validate();
	if (!Problems.empty())
	{
		OutError = TEXT("Game data problems:");
		for (const std::string& Problem : Problems)
		{
			OutError += TEXT("\n  ") + WyrmText::ToF(Problem);
		}
		return false;
	}

	FWyrmStartingSetup Setup;
	const TSharedPtr<FJsonObject>* NewGame = nullptr;
	if (Root->TryGetObjectField(TEXT("newGame"), NewGame) && NewGame && NewGame->IsValid())
	{
		Setup.Gold = WyrmJsonInt(*NewGame, TEXT("gold"), static_cast<int32>(Setup.Gold));
		Setup.Food = WyrmJsonInt(*NewGame, TEXT("food"), static_cast<int32>(Setup.Food));
		Setup.Gems = WyrmJsonInt(*NewGame, TEXT("gems"), static_cast<int32>(Setup.Gems));
		Setup.StarterFarms = WyrmJsonInt(*NewGame, TEXT("starterFarms"), Setup.StarterFarms);
		Setup.StarterDragons = WyrmJsonStringArray(*NewGame, TEXT("starterDragons"));
		Setup.StarterHabitats = WyrmJsonStringArray(*NewGame, TEXT("starterHabitats"));
	}
	const TSharedPtr<FJsonObject>* Buildings = nullptr;
	if (Root->TryGetObjectField(TEXT("buildings"), Buildings) && Buildings && Buildings->IsValid())
	{
		Setup.FarmCost = WyrmJsonInt(*Buildings, TEXT("farmCost"), static_cast<int32>(Setup.FarmCost));
	}

	OutCatalog = MoveTemp(Catalog);
	OutSetup = MoveTemp(Setup);
	return true;
}

bool LoadCatalog(const FString& Path, Wyrm::Catalog& OutCatalog, FWyrmStartingSetup& OutSetup, FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutError = FString::Printf(TEXT("Could not read game data file '%s'"), *Path);
		return false;
	}
	return ParseCatalog(Text, OutCatalog, OutSetup, OutError);
}
} // namespace WyrmCatalogLoader
