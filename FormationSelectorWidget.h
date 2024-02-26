// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FormationButtonWidget.h"
#include "SPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "FormationSelectorWidget.generated.h"

/**
 * 
 */
UCLASS()
class RTSGAME_API UFormationSelectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UFormationButtonWidget* LineButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UFormationButtonWidget* ColumnButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UFormationButtonWidget* WedgeButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UFormationButtonWidget* BlobButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	USlider* SpacingSlider;

protected:
	UFUNCTION()
	void OnFormationButtonClicked(USimpleButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSpacingValueChanged(const float Value);

	UPROPERTY()
	ASPlayerController* SPlayer;
	
};
