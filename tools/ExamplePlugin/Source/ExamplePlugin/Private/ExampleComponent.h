#pragma once

#include "Components/ActorComponent.h"
#include "ExampleComponent.generated.h"

UCLASS()
class UTempObject : public UObject
{
	GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    int Value1;

	UPROPERTY()
	int Value2;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EXAMPLEPLUGIN_API UExampleComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	int TickTimes = 0;
	void* LastPtr = nullptr;
    UPROPERTY()
    UObject* TempObject;
};