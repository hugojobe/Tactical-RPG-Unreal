// By hugojobe.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ChessAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS(Blueprintable)
class TRPG_EXAM_API UChessAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UChessAttributeSet();
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UChessAttributeSet, Health);
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UChessAttributeSet, Damage);
	
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHealthValue() const { return Health.GetCurrentValue(); }
	
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetDamageValue() const { return Damage.GetCurrentValue(); }
};
