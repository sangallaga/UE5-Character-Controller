// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATestbedActor.generated.h"

UCLASS()
class CHARACTERCONTROLLER_API AATestbedActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATestbedActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	float TestSpeed = 400.f;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
