// Fill out your copyright notice in the Description page of Project Settings.


#include "SPlayerPawn.h"

#include "EngineUtils.h"
#include "IMovieSceneTracksModule.h"
#include "InteractiveToolManager.h"
#include "SelectionBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ASPlayerPawn::ASPlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 2000.0f;
	SpringArmComponent->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	MoveSpeed = 20.0f;
	EdgeScrollSpeed = 3.0f;
	RotateSpeed = 2.0f;
	RotatePitchMin = 10.0f;
	RotatePitchMax = 80.0f;
	ZoomSpeed = 300.0f;
	MinZoom = 500.0f;
	MaxZoom = 4000.0f;
	Smoothing = 2.0f;
}

// Called when the game starts or when spawned
void ASPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	// Set the initial desired zoom and location
	TargetLocation = GetActorLocation();
	TargetZoom = 3000.0f;

	// Set an initial rotation for the camera
	const FRotator Rotation = SpringArmComponent->GetRelativeRotation();
	TargetRotation = FRotator(Rotation.Pitch - 50, Rotation.Yaw, 0.0f);

	// Assign player controller reference
	SPlayer = Cast<ASPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	CreateSelectionBox();
	
}
// Called every frame
void ASPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraBounds();
	EdgeScroll();

	// Move the pawn in the desired direction
	const FVector InterpolatedLocation = UKismetMathLibrary::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime,Smoothing);
	SetActorLocation(InterpolatedLocation);

	// Zoom the camera in desired direction
	const float InterpolatedZoom = UKismetMathLibrary::FInterpTo(SpringArmComponent->TargetArmLength, TargetZoom, DeltaTime, Smoothing);
	SpringArmComponent->TargetArmLength = InterpolatedZoom;

	// Rotate the camera in the desired direction
	const FRotator InterpolatedRotation = UKismetMathLibrary::RInterpTo(SpringArmComponent->GetRelativeRotation(), TargetRotation, DeltaTime, Smoothing);
	SpringArmComponent->SetRelativeRotation(InterpolatedRotation);
}

// Called to bind functionality to input
void ASPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	const ASPlayerController* PlayerController = Cast<ASPlayerController>(GetController());

	//Set up action bindings
	if (IsValid(Input) && IsValid(PlayerController))
	{
		if (const UPlayerInputActions* PlayerActions = Cast<UPlayerInputActions>(PlayerController->GetInputActionAsset()))
		{
			/** Default **/
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->Move, this, &ASPlayerPawn::Move);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->Look, this, &ASPlayerPawn::Look);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->Zoom, this, &ASPlayerPawn::Zoom);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->Rotate, this, &ASPlayerPawn::Rotate);
			EPlayerInputActions::BindInput_StartTriggerComplete(Input, PlayerActions->Select, this, &ASPlayerPawn::Select, &ASPlayerPawn::SelectHold, &ASPlayerPawn::SelectEnd);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->TestPlacement, this, &ASPlayerPawn::TestPlacement);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->SelectDoubleTap, this, &ASPlayerPawn::SelectDoubleTap);
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->Command, this, &ASPlayerPawn::CommandStart, &ASPlayerPawn::Command);
			
			/** Placement **/
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->Place, this, &ASPlayerPawn::Place);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->PlaceCancel, this, &ASPlayerPawn::PlaceCancel);

			// Shift
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->Shift, this, &ASPlayerPawn::Shift, &ASPlayerPawn::Shift);
			EPlayerInputActions::BindInput_TriggerOnly(Input, PlayerActions->ShiftSelect, this, &ASPlayerPawn::ShiftSelect);
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->ShiftCommand, this, &ASPlayerPawn::CommandStart, &ASPlayerPawn::ShiftCommand);

			//Alt
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->Alt, this, &ASPlayerPawn::Alt, &ASPlayerPawn::Alt);
			EPlayerInputActions::BindInput_StartTriggerComplete(Input, PlayerActions->AltSelect, this, &ASPlayerPawn::AltSelect, &ASPlayerPawn::SelectHold, &ASPlayerPawn::AltSelectEnd);
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->AltCommand, this, &ASPlayerPawn::CommandStart, &ASPlayerPawn::AltCommand);

			// Ctrl
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->Ctrl, this, &ASPlayerPawn::Ctrl, &ASPlayerPawn::Ctrl);
			EPlayerInputActions::BindInput_StartTriggerComplete(Input, PlayerActions->CtrlSelect, this, &ASPlayerPawn::CtrlSelect, &ASPlayerPawn::SelectHold, &ASPlayerPawn::CtrlSelectEnd);
			EPlayerInputActions::BindInput_Simple(Input, PlayerActions->CtrlCommand, this, &ASPlayerPawn::CommandStart, &ASPlayerPawn::CtrlCommand);

		}
		
	}

}

void ASPlayerPawn::GetTerrainPosition(FVector& TerrainPosition) const
{
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	FVector TraceOrigin = TerrainPosition;
	TraceOrigin.Z += 10000.0f;
	FVector TraceEnd = TerrainPosition;
	TraceEnd.Z -= 10000.0f;

	if (UWorld* WorldContext = GetWorld())
	{
		if (WorldContext->LineTraceSingleByChannel(Hit, TraceOrigin, TraceEnd, ECC_Visibility, CollisionParams))
		{
			TerrainPosition = Hit.ImpactPoint;
		}
	}
}


// Allow player to move camera by moving mouse to edge of screen
void ASPlayerPawn::EdgeScroll()
{
	if(UWorld* WorldContext = GetWorld())
	{
		FVector2D MousePosition= UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldContext);
		const FVector2d ViewportSize = UWidgetLayoutLibrary::GetViewportSize(WorldContext);
		MousePosition = MousePosition * UWidgetLayoutLibrary::GetViewportScale(WorldContext);
		MousePosition.X = MousePosition.X / ViewportSize.X;
		MousePosition.Y = MousePosition.Y / ViewportSize.Y;

		//Right/Left
		if (MousePosition.X > 0.98f && MousePosition.X < 1.0f)
		{
			TargetLocation += SpringArmComponent->GetTargetRotation().RotateVector(FVector(0.f, 1.0f, 0.f)) * EdgeScrollSpeed;
		}
		else if (MousePosition.X < 0.02f && MousePosition.X >0.f)
		{
			TargetLocation += SpringArmComponent->GetTargetRotation().RotateVector(FVector(0.f, -1.0f, 0.f)) * EdgeScrollSpeed;
		}

		//Forward/Backward
		if (MousePosition.Y > 0.98f && MousePosition.Y < 1.0f)
		{
			TargetLocation += SpringArmComponent->GetTargetRotation().RotateVector(FVector(-1.0f, 0.f, 0.f)) * EdgeScrollSpeed;
		}
		else if (MousePosition.Y < 0.02f && MousePosition.Y >0.f)
		{
			TargetLocation += SpringArmComponent->GetTargetRotation().RotateVector(FVector(1.0f, 0.f, 0.f)) * EdgeScrollSpeed;
		}

		GetTerrainPosition(TargetLocation);
	}
}



//Clamps the camera pitch to prevent camera going upside down
void ASPlayerPawn::CameraBounds()
{
	float NewPitch = TargetRotation.Pitch;
	if (TargetRotation.Pitch < (RotatePitchMax * -1))
	{
		NewPitch = (RotatePitchMax * -1);
	}
	else if (TargetRotation.Pitch > (RotatePitchMin * -1))
	{
		NewPitch = (RotatePitchMin * -1);
	}

	// Set the new pitch and clamp any roll
	TargetRotation = FRotator(NewPitch, TargetRotation.Yaw, 0.0f);
}

AActor* ASPlayerPawn::GetSelectedObject()
{
	// Check if we hit an actor we could potentially select
	if (UWorld* World = GetWorld())
	{
		FVector WorldLocation, WorldDirection;
		SPlayer->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		FVector End = WorldDirection * 1000000.0f + WorldLocation;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, WorldLocation, End, ECC_Visibility, Params))
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				return HitActor;
			}
		}
	}
	return nullptr;
}

void ASPlayerPawn::CreateSelectionBox()
{
	if (!SelectionBoxClass)
	{
		return;
	}

	if (UWorld* WorldContext = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = this;
		SpawnParams.Owner = this;
		SelectionBox = WorldContext->SpawnActor<ASelectionBox>(SelectionBoxClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (SelectionBox)
		{
			SelectionBox->SetOwner(this);
		}
	}
}

void ASPlayerPawn::Move(const FInputActionValue& Value)
{
	if (!SpringArmComponent)
	{
		return;
	}

	if (ensure(Value.GetValueType() == EInputActionValueType::Axis2D))
	{
		TargetLocation += SpringArmComponent->GetTargetRotation().RotateVector(Value.Get<FVector>()) * MoveSpeed;
		GetTerrainPosition(TargetLocation);
	}
	
}

void ASPlayerPawn::Look(const FInputActionValue& Value)
{
	if (ensure(Value.GetValueType() == EInputActionValueType::Axis1D))
	{
		const float NewPitch = Value.Get<float>() * RotateSpeed * 0.5;
		TargetRotation = FRotator(TargetRotation.Pitch + NewPitch, TargetRotation.Yaw, 0.f);
	}
}

void ASPlayerPawn::Rotate(const FInputActionValue& Value)
{
	if (ensure(Value.GetValueType() == EInputActionValueType::Axis1D))
	{
		const float NewRotation = Value.Get<float>() * RotateSpeed;
		TargetRotation = FRotator(TargetRotation.Pitch, TargetRotation.Yaw + NewRotation, 0.f);
	}
}

void ASPlayerPawn::Select(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Clear any current selection
	SPlayer->Handle_Selection(nullptr);

	//Reset box select
	BoxSelect = false;

	//Store the initial hit location
	SelectHitLocation = SPlayer->GetMousePositionOnTerrain();
}

void ASPlayerPawn::SelectHold(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	if ((SelectHitLocation - SPlayer->GetMousePositionOnTerrain()).Length() > 100.0f)
	{
		if (!BoxSelect && SelectionBox)
		{
			SelectionBox->Start(SelectHitLocation, TargetRotation);
			BoxSelect = true;
		}
	}
	
}

void ASPlayerPawn::SelectEnd(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Check if there is an active box selection
	if (BoxSelect && SelectionBox)
	{
		// Use box for selection
		SelectionBox->End();

		// Reset box selection
		BoxSelect = false;
	}
	else
	{
		// If no box selection, perform a single selection
		SPlayer->Handle_Selection(GetSelectedObject());
	}
}

void ASPlayerPawn::Zoom(const FInputActionValue& Value)
{
	if (ensure(Value.GetValueType() == EInputActionValueType::Axis1D))
	{
		TargetZoom = FMath::Clamp(TargetZoom + (Value.Get<float>() * ZoomSpeed), MinZoom, MaxZoom);
	}
}

void ASPlayerPawn::TestPlacement(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	SPlayer->SetPlacementPreview();
}

void ASPlayerPawn::SelectDoubleTap(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}
}

void ASPlayerPawn::CommandStart(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	CommandLocation = SPlayer->GetMousePositionOnTerrain();
}

void ASPlayerPawn::Command(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	SPlayer->CommandSelected(CreateCommandData(ECommandType::CommandMove));
}

void ASPlayerPawn::ShiftCommand(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	SPlayer->CommandSelected(CreateCommandData(ECommandType::CommandMoveFast));
}

void ASPlayerPawn::AltCommand(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	SPlayer->CommandSelected(CreateCommandData(ECommandType::CommandMoveSlow));
}

void ASPlayerPawn::CtrlCommand(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	SPlayer->CommandSelected(CreateCommandData(ECommandType::CommandMoveAttack));
}

void ASPlayerPawn::Shift(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shift %s"), Value.Get<bool>() ? TEXT("ON") : TEXT("OFF"));
	SPlayer->SetInputShift(Value.Get<bool>());
}

void ASPlayerPawn::Alt(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shift %s"), Value.Get<bool>() ? TEXT("ON") : TEXT("OFF"));
	SPlayer->SetInputAlt(Value.Get<bool>());
}

void ASPlayerPawn::Ctrl(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shift %s"), Value.Get<bool>() ? TEXT("ON") : TEXT("OFF"));
	SPlayer->SetInputCtrl(Value.Get<bool>());
}

void ASPlayerPawn::ShiftSelect(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Get the selection under cursor
	if (AActor* Selection = GetSelectedObject())
	{
		// Get selection class
		const TSubclassOf<AActor> SelectionClass = Selection->GetClass();

		TArray<AActor*> Actors;
		Actors.Add(Selection);

		for (TActorIterator<AActor> ActorItr(GetWorld(), SelectionClass); ActorItr; ++ActorItr)
		{
			AActor* Actor = *ActorItr;
			const float DistanceSquared = FVector::DistSquared(Actor->GetActorLocation(), SelectHitLocation);
			if (DistanceSquared <= FMath::Square(1000.f))
			{
				Actors.AddUnique(Actor);
			}
		}
		SPlayer->Handle_Selection(Actors);
	}
	else
	{
		SPlayer->Handle_Selection(nullptr);
	}
}

void ASPlayerPawn::AltSelect(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	
	// Reset box select
	BoxSelect = false;

	// Store the initial hit location
	SelectHitLocation = SPlayer->GetMousePositionOnTerrain();
}

void ASPlayerPawn::AltSelectEnd(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Check if there is an active box selection
	if (BoxSelect && SelectionBox)
	{
		// Use for box selection
		SelectionBox->End(false);

		// Reset the box selection
		BoxSelect = false;
	}
	else
	{
		// if no box selection, perform a single selection
		SPlayer->Handle_DeSelection(GetSelectedObject());
	}

}

void ASPlayerPawn::CtrlSelect(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Reset box select
	BoxSelect = false;

	// Store the initial hit location
	SelectHitLocation = SPlayer->GetMousePositionOnTerrain();
}

void ASPlayerPawn::CtrlSelectEnd(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	// Check if there is an active box selection
	if (BoxSelect && SelectionBox)
	{
		// Use for box selection
		SelectionBox->End(true, true);

		// Reset the box selection
		BoxSelect = false;
	}
	else
	{
		// If no box selection, perform a single selection
		if (AActor* Selection = GetSelectedObject())
		{
			SPlayer->Handle_Selection(Selection);
		}
	}
}

void ASPlayerPawn::Place(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	if (SPlayer->IsPlacementModeEnabled())
	{
		SPlayer->Place();
	}
}

void ASPlayerPawn::PlaceCancel(const FInputActionValue& Value)
{
	if (!SPlayer)
	{
		return;
	}

	if (SPlayer->IsPlacementModeEnabled())
	{
		SPlayer->PlaceCancel();
	}
}

FCommandData ASPlayerPawn::CreateCommandData(const ECommandType Type) const
{
	if (!SPlayer)
	{
		return FCommandData();
	}

	FRotator CommandRotation = FRotator::ZeroRotator;
	const FVector CommandEndLocation = SPlayer->GetMousePositionOnTerrain();
	// Check the mouse was dragged, otherwise leave rotation at zero
	if ((CommandEndLocation - CommandLocation).Length() > 100.f)
	{
		// Calculate the rotation angle
		const FVector Direction = CommandEndLocation - CommandLocation;
		const float RotationAngle = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));

		//Convert the rotation angle to a rotation
		CommandRotation = FRotator(0.f, RotationAngle, 0.f);
		
	}

	return FCommandData(CommandLocation, CommandRotation, Type);
}
