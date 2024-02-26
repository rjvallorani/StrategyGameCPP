// Fill out your copyright notice in the Description page of Project Settings.



#include "SimpleButtonWidget.h"

#include "Components/Button.h"

void USimpleButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button->OnClicked.AddDynamic(this, &USimpleButtonWidget::OnSimpleButtonClickedEvent);
	Button->OnHovered.AddDynamic(this, &USimpleButtonWidget::OnSimpleButtonHoveredEvent);
	Button->OnUnhovered.AddDynamic(this, &USimpleButtonWidget::OnSimpleButtonUnHoveredEvent);
	
}

void USimpleButtonWidget::OnSimpleButtonClickedEvent()
{
	OnButtonClicked.Broadcast(this, ButtonIndex);
}

void USimpleButtonWidget::OnSimpleButtonHoveredEvent()
{
	OnButtonHovered.Broadcast(this, ButtonIndex);
}

void USimpleButtonWidget::OnSimpleButtonUnHoveredEvent()
{
	OnButtonUnHovered.Broadcast(this, ButtonIndex);
}
