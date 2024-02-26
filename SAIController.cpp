// Fill out your copyright notice in the Description page of Project Settings.


#include "SAIController.h"

#include "RTSGameCharacter.h"

ASAIController::ASAIController(const FObjectInitializer& ObjectInitializer)
{
}

void ASAIController::CommandMove(const FCommandData& CommandData)
{
	CurrentCommandData = CommandData;
	MoveToLocation(CommandData.Location);
}

void ASAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Set a reference to the owner and set a reference of this controller on the owner
	OwningCharacter = Cast<ARTSGameCharacter>(InPawn);
	if (OwningCharacter)
	{
		OwningCharacter->SetAIController(this);
	}
}

void ASAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	OnReachedDestination.Broadcast(CurrentCommandData);
}
