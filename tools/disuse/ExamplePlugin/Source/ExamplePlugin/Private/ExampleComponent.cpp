#include "ExampleComponent.h"
#include "ExamplePluginDef.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ConstructorHelpers.h"

UTempObject::UTempObject(const FObjectInitializer& Initializer) : Super(Initializer)
{

}

UExampleComponent::UExampleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

extern ENGINE_API float GAverageFPS;
void UExampleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickTimes++;

	UE_LOG(LogExamplePlugin, Log, TEXT("FPS(%f)"), GAverageFPS);

	auto NewPtr = GMalloc->Realloc(LastPtr, TickTimes, 16);
	if (NewPtr)
	{
		memset(NewPtr, 0, TickTimes);
		LastPtr = NewPtr;
	}

	TempObject = NewObject<UTempObject>();

	if (TickTimes % 16 == 0)
	{
		GMalloc->Malloc(TickTimes);
	}

	if (TickTimes == 128)
	{
		if (!GIsEditor)
		{
			GEngine->Exec(this->GetWorld(), TEXT("EXIT"));
		}
		else
		{
	        UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit);
		}
	}
}

