// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "SimpleButtonWidget.generated.h"

class UButton;
class UImage;
class USimpleButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonClicked, USimpleButtonWidget*, Button, int, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonHovered, USimpleButtonWidget*, Button, int, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonUnHovered, USimpleButtonWidget*, Button, int, Index);

UCLASS()
class RTSGAME_API USimpleButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* Button;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ButtonImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* ButtonText;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FButtonClicked OnButtonClicked;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FButtonHovered OnButtonHovered;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FButtonUnHovered OnButtonUnHovered;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	int32 ButtonIndex;

protected:
	UFUNCTION()
	void OnSimpleButtonClickedEvent();

	UFUNCTION()
	void OnSimpleButtonHoveredEvent();

	UFUNCTION()
	void OnSimpleButtonUnHoveredEvent();
};