// Fill out your copyright notice in the Description page of Project Settings.


#include "SPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "PlacementPreview.h"
#include "RTSGameCharacter.h"
#include "Selectable.h"
#include "Net/UnrealNetwork.h"

ASPlayerController::ASPlayerController(const FObjectInitializer& ObjectInitializer)
{
	bPlacementModeEnabled = false;
}

void ASPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASPlayerController, Selected, COND_OwnerOnly);
}

void ASPlayerController::Handle_Selection(AActor* ActorToSelect)
{
	if (ISelectable* Selectable = Cast<ISelectable>(ActorToSelect))
	{
		if (ActorToSelect && ActorSelected(ActorToSelect))
		{
			Server_DeSelect(ActorToSelect);
		}
		else
		{
			Server_Select(ActorToSelect);
		}
	}
	else
	{
		Server_ClearSelected();
	}
}

void ASPlayerController::Handle_Selection(TArray<AActor*> ActorsToSelect)
{
	Server_Select_Group(ActorsToSelect);
}

FVector ASPlayerController::GetMousePositionOnTerrain() const
{
	FVector WorldLocation, WorldDirection;
	DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	FHitResult OutHit;
	if (GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + (WorldDirection * 100000.0f), ECC_GameTraceChannel1))
	{
		if (OutHit.bBlockingHit)
		{
			return OutHit.Location;
		}
	}

	return FVector::ZeroVector;
}
FVector ASPlayerController::GetMousePositionOnSurface() const
{
	FVector WorldLocation, WorldDirection;
	DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	FHitResult OutHit;
	if (GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + (WorldDirection * 100000.0f), ECC_Visibility))
	{
		if (OutHit.bBlockingHit)
		{
			return OutHit.Location;
		}
	}

	return FVector::ZeroVector;
}

void ASPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPlacementModeEnabled && PlacementPreviewActor)
	{
		UpdatePlacement();
	}
}

void ASPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Sets cursor to remain on screen after selecting
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

bool ASPlayerController::ActorSelected(AActor* ActorToCheck) const
{
	if (ActorToCheck && Selected.Contains(ActorToCheck))
	{
		return true;
	}

	return false;
}

void ASPlayerController::Server_Select_Group_Implementation(const TArray<AActor*>& ActorsToSelect)
{
	Server_ClearSelected();

	TArray<AActor*> ValidActors;
	for (int i = 0; i < ActorsToSelect.Num(); ++i)
	{
		if (ActorsToSelect[i])
		{
			if (ISelectable* Selectable = Cast<ISelectable>(ActorsToSelect[i]))
			{
				ValidActors.Add(ActorsToSelect[i]);
				Selectable->Select();
			}
		}
	}
	for (int j = 0; j < ValidActors.Num(); ++j)
	{
		Selected.Append(ValidActors);
		OnRep_Selected();
	}
	ValidActors.Empty();
}

void ASPlayerController::Server_Select_Implementation(AActor* ActorToSelect)
{
	Server_ClearSelected();

	if (ActorToSelect)
	{
		if (ISelectable* Selectable = Cast<ISelectable>(ActorToSelect))
		{
			Selectable->Select();
			Selected.Add(ActorToSelect);
			OnRep_Selected();
		}
	}
}

void ASPlayerController::Server_DeSelect_Implementation(AActor* ActorToDeSelect)
{
	if (ActorToDeSelect)
	{
		if (ISelectable* Selectable = Cast<ISelectable>(ActorToDeSelect))
		{
			Selectable->DeSelect();
			Selected.Remove(ActorToDeSelect);
			OnRep_Selected();
		}
	}
}

void ASPlayerController::Server_ClearSelected_Implementation()
{
	// Loop through selected to make sure nothing is left highlighted
	for (int i = 0; i < Selected.Num(); ++i)
	{
		if (Selected[i])
		{
			if (ISelectable* Selectable = Cast<ISelectable>(Selected[i]))
			{
				Selectable->DeSelect();
			}
		}
	}

	Selected.Empty();
	OnRep_Selected();
}

void ASPlayerController::OnRep_Selected()
{
	OnSelectedUpdated.Broadcast();
}

/** Enhanced Input **/
void ASPlayerController::AddInputMapping(const UInputMappingContext* InputMapping, const int32 MappingPriority) const
{
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		ensure(InputMapping);

		if (!InputSubsystem->HasMappingContext(InputMapping))
		{
			InputSubsystem->AddMappingContext(InputMapping, MappingPriority);
		}
	}
}

void ASPlayerController::RemoveInputMapping(const UInputMappingContext* InputMapping) const
{
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		ensure(InputMapping);
		InputSubsystem->RemoveMappingContext(InputMapping);
	}
}

void ASPlayerController::SetInputDefault(const bool Enabled) const
{
	ensureMsgf(PlayerActionsAsset, TEXT("PlayerActionAsset is NULL!"));

	if (const UPlayerInputActions* PlayerActions = Cast<UPlayerInputActions>(PlayerActionsAsset))
	{
		ensure(PlayerActions->MappingContextDefault);

		if (Enabled)
		{
			AddInputMapping(PlayerActions->MappingContextDefault, PlayerActions->MapPriorityDefault);
		}
		else
		{
			RemoveInputMapping(PlayerActions->MappingContextDefault);
		}
	}
}

void ASPlayerController::SetInputPlacement(const bool Enabled) const
{
	if (const UPlayerInputActions* PlayerActions = Cast<UPlayerInputActions>(PlayerActionsAsset))
	{
		ensure(PlayerActions->MappingContextPlacement);

		if (Enabled)
		{
			AddInputMapping(PlayerActions->MappingContextPlacement, PlayerActions->MapPriorityPlacement);
			SetInputDefault(!Enabled);
		}
		else
		{
			RemoveInputMapping(PlayerActions->MappingContextPlacement);
			SetInputDefault();
		}
	}
}

void ASPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->ClearAllMappings();
		SetInputDefault();
	}
}

void ASPlayerController::SetPlacementPreview()
{
	if (PreviewActorType && !bPlacementModeEnabled)
	{
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(GetMousePositionOnSurface());
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PlacementPreviewActor = GetWorld()->SpawnActor<APlacementPreview>(PreviewActorType, SpawnTransform, SpawnParams);

		if (PlacementPreviewActor)
		{
			// Enable placement input
			SetInputPlacement();

			// Set placement mode
			bPlacementModeEnabled = true;
		}
	}
}

void ASPlayerController::Place()
{
	if (!IsPlacementModeEnabled() || !PlacementPreviewActor)
	{
		return;
	}

	// Disable placement mods
	bPlacementModeEnabled = false;

	// Disable placement Input
	SetInputPlacement(false);

	// Sent to server for aurthoritative spawning
	Server_Place(PlacementPreviewActor);
}

void ASPlayerController::PlaceCancel()
{
	if (IsPlacementModeEnabled() || !PlacementPreviewActor)
	{
		return;
	}

	// Disable placement mode
	bPlacementModeEnabled = false;

	// Disable placement Input
	SetInputPlacement(false);

	EndPlacement();
}

void ASPlayerController::UpdatePlacement() const
{
	if (!PlacementPreviewActor)
	{
		return;
	}

	// Update placeable actor position with mouse position (on tick)
	PlacementPreviewActor->SetActorLocation(GetMousePositionOnSurface());
}


void ASPlayerController::Server_Place_Implementation(AActor* PlacementPreviewToSpawn)
{
	if (const APlacementPreview* Preview = Cast<APlacementPreview>(PlacementPreviewToSpawn))
	{
		FTransform SpawnTransform;
		FVector Location = Preview->GetActorLocation();
		SpawnTransform.SetLocation(FVector(Location.X, Location.Y, Location.Z + 100.0f));
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (ARTSGameCharacter* NewUnit = GetWorld()->SpawnActor<ARTSGameCharacter>(Preview->PlaceableClass, SpawnTransform, SpawnParams))
		{
			NewUnit->SetOwner(this);
		}
	}

	EndPlacement();
}

void ASPlayerController::EndPlacement_Implementation()
{
	PlacementPreviewActor->Destroy();
}