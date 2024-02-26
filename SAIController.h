// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIData.h"
#include "SAIController.generated.h"

class ARTSGameCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReachedDestinationDelegate, const FCommandData, CommandData);

/**
 * 
 */
UCLASS()
class RTSGAME_API ASAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASAIController(const FObjectInitializer& ObjectInitializer);

	UFUNCTION()
	void CommandMove(const FCommandData& CommandData);

	UPROPERTY()
	FReachedDestinationDelegate OnReachedDestination;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY()
	ARTSGameCharacter* OwningCharacter;

	UPROPERTY()
	FCommandData CurrentCommandData;
};
