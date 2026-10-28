#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SKAttributeSet.generated.h"

#define SK_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** Health and ultimate gauge. Single player only, so no replication. */
UCLASS()
class SLEEPLESSKNIGHT_API USKAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health = 100.f;
	SK_ATTRIBUTE_ACCESSORS(USKAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth = 100.f;
	SK_ATTRIBUTE_ACCESSORS(USKAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData UltGauge = 0.f;
	SK_ATTRIBUTE_ACCESSORS(USKAttributeSet, UltGauge)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxUltGauge = 100.f;
	SK_ATTRIBUTE_ACCESSORS(USKAttributeSet, MaxUltGauge)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
