// Fill out your copyright notice in the Description page of Project Settings.


#include "ATestbedActor.h"

// Sets default values
AATestbedActor::AATestbedActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATestbedActor::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("TestSpeed = %f"), TestSpeed);
	
	// Apply mapping context
	// Create APlayerController ptr to class
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
}

// Called every frame
void AATestbedActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
