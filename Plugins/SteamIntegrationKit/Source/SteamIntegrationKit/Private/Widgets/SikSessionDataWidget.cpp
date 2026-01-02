// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#include "Widgets/SikSessionDataWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "System/SikLogger.h"
#include "Widgets/SikHudWidget.h"

bool USikSessionDataWidget::Initialize()
{
	if (!Super::Initialize())
		return false;

	if (JoinSessionButton)
		JoinSessionButton->OnClicked.AddDynamic(this, &ThisClass::OnJoinSessionButtonClicked);
	
	return true;
}

void USikSessionDataWidget::OnJoinSessionButtonClicked()
{
	if (!SikHudWidget.IsValid())
	{
		LOG_ERROR(TEXT("SikHUD is INVALID"));
		return;
	}
	
	SikHudWidget->JoinTheGivenSession(SessionSearchResult);
}

void USikSessionDataWidget::SetSessionInfo(const FOnlineSessionSearchResult& InSessionSearchResultRef, 
	const FSikCustomSessionSettings& SessionSettings)
{
	SessionSearchResult = InSessionSearchResultRef;
	
	MapName->SetText(FText::FromString(SessionSettings.MapName));
	Players->SetText(FText::FromString(SessionSettings.Players));
	GameMode->SetText(FText::FromString(SessionSettings.GameMode));
}

void USikSessionDataWidget::SetSikHudWidget(USikHudWidget* InSikHudWidget)
{
	SikHudWidget = InSikHudWidget;
}
