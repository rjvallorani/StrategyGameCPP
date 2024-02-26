// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIData.h"
#include "SimpleButtonWidget.h"
#include "FormationButtonWidget.generated.h"


/**
 * 
 */
UCLASS()
class RTSGAME_API UFormationButtonWidget : public USimpleButtonWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category= "Settings")
	TEnumAsByte<EFormation> Formation;
};



