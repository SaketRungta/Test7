// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "SikSessionDataWidget.generated.h"

struct FSikCustomSessionSettings;
class UTextBlock;
class UButton;
class USikHudWidget;

/**
 * Class to show the session data in the scroll box
 * Stores and displays all the session related info in the scroll box
 ******************************************************************************************/
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Widgets))
class STEAMINTEGRATIONKIT_API USikSessionDataWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** Initializes all the widgets */
	virtual bool Initialize() override;
	
#pragma region Components
	
private:
	/** Text to show the map name */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MapName;

	/** Text to show the players count */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Players;

	/** Text to show the game mode */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GameMode;

	/** Button to let user join this session */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinSessionButton;

#pragma endregion Components
	
#pragma region Bindings
	
private:
	/** Join button clicked callback, calls the main menu widget to join this session */
	UFUNCTION()
	void OnJoinSessionButtonClicked();
	
#pragma endregion Bindings
	
#pragma region CachedData
	
private:
	/** Ref to the main menu widget set via setter, for when user joins this session we can call the main menu widget to join this session */
	TWeakObjectPtr<USikHudWidget> SikHudWidget;
	
	/** Stores the session search result for this widget */
	FOnlineSessionSearchResult SessionSearchResult;

#pragma endregion CachedData
	
#pragma region Setters
	
public:
	/** Called from USikHudWidget::AddSessionSearchResultsToScrollBox upon adding this widget to the scroll box to fill it with necessary information */
	void SetSessionInfo(const FOnlineSessionSearchResult& InSessionSearchResultRef, 
		const FSikCustomSessionSettings& SessionSettings);

	/** Called from USikHudWidget::AddSessionSearchResultsToScrollBox upon adding this widget to the scroll box to set the ref to main menu widget */
	void SetSikHudWidget(USikHudWidget* InSikHUDWidget);
	
#pragma endregion Setters
	
};
