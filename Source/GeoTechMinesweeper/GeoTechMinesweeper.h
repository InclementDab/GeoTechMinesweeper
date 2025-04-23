// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SMineButton: public SButton
{
public:
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FReply ExecuteOnRightClick();

	FOnClicked OnRightClicked;
};

enum EMinesweeperGameState
{
	None,
	Playing,
	FinishWin,
	FinishLose
};

class FMinesweeperGame
{
public:
	FMinesweeperGame() = default;
	FMinesweeperGame(const int InWidth, const int InHeight, const int InMineCount);
	virtual ~FMinesweeperGame();

	bool SetPlayArea(TSharedPtr<SBorder> Panel);
	void SetState(EMinesweeperGameState State);
	bool IsGameComplete() const;
	
	class FMinesweeperTile& GetTile(const int X, const int Y) const;
	FMinesweeperTile* GetTilePtr(const int X, const int Y) const;
	
	// Returns mine count in nxn space around position
	int GetMineCountInArea(const FIntPoint Position, const int DistanceFromCenter) const;

	void UnlockSurroundingTilesIfEmpty(const FIntPoint Position, const int DistanceFromCenter, TArray<FIntPoint>& CheckedPositions);

	void OnTileExposed(FMinesweeperTile* Tile);

	// User facing statistics
	int FlagsPlaced = 0;
	int SpacesExposed = 0;
	int Width = 0, Height = 0, MineCount = 0;
	
protected:
	FMinesweeperTile* Board = nullptr;

	// 8x8 for beginner, 10 mines
	// 16x16 for intermediate 40 mines
	// 32x16 for expert 99 mines
	// 32x32 for impossible 170 mines
	
	TSharedPtr<SBorder> PlayBorder;
	TSharedPtr<SGridPanel> PlayAreaWidget;
	SGridPanel::FSlot** PlayAreaGridWidgets = nullptr;
	
	EMinesweeperGameState GameState = None;
};

class FMinesweeperTile
{
public:
	FMinesweeperGame* Game = nullptr;
	FIntPoint Position = { -1, -1 };
	
	bool IsMine = false; // This tile is a mine
	
	bool IsExposed = false; // User has clicked this mine
	bool IsFlagged = false; // User has flagged this as a danger

	int MinesInArea = 0; // Validated when clicked
	
	TSharedPtr<SMineButton> Button;
	TSharedPtr<SImage> Image;
	TSharedPtr<STextBlock> Text;

	FReply OnRightClicked()
	{
		if (Game->IsGameComplete()) {
			return FReply::Handled();
		}
		
		if (IsExposed) {
			return FReply::Handled();
		}
		
		IsFlagged = !IsFlagged;
		Game->FlagsPlaced += IsFlagged ? 1 : -1;
		
		return FReply::Handled();
	}

	void Expose()
	{
		if (IsExposed) {
			return;
		}
		
		IsExposed = true;
		Game->OnTileExposed(this);
	}
	
	FReply OnClicked()
	{
		if (Game->IsGameComplete()) {
			return FReply::Handled();
		}
		
		Expose();

		if (!IsMine) {			
			// Could be two functions to clean up but im not trying *that* hard
			TArray<FIntPoint> CheckedPositions;
			Game->UnlockSurroundingTilesIfEmpty(Position, 1, CheckedPositions);
		}
		
		return FReply::Handled();
	}

	bool IsEnabled() const
	{
		return !IsExposed;
	}
	
	const FSlateBrush* GetImage() const
	{
		if (!IsExposed) {
			if (IsFlagged) {
				// flagged
				return FSlateIcon(FName("EditorStyle"), "ShowFlagsMenu.Navigation").GetIcon();
			}
			
			return nullptr;
		}
		
		// death
		if (IsMine) {
			return FSlateIcon(FName("EditorStyle"), "ShowFlagsMenu.Collision").GetIcon();
		}
		
		return nullptr;
	}
	
	// Returns text for icon
	FText GetText() const
	{
		if (IsMine) {
			return INVTEXT("");
		}
		
		if (IsExposed) {
			// Expose the number here
			if (MinesInArea) {
				return FText::FromString(FString::FormatAsNumber(MinesInArea));
			}
			
			return INVTEXT("");
		}
		
		return INVTEXT("");
	}

	FSlateColor GetColor() const
	{
		if (IsFlagged) {
			return FColorList::Red;
		}

		if (IsExposed && IsMine) {
			return FColor::Black;
		}

		if (IsExposed && !IsMine) {
			switch (MinesInArea) {
			case 1: return FColorList::NeonBlue;
			case 2: return FColorList::Green;
			case 3: return FColorList::Red;
			case 4: return FColorList::Violet;
			case 5: return FColorList::Brown;
			case 6: return FColorList::Orange;
			case 7: return FColorList::DarkPurple;
			case 8: return FColorList::Gold;
			default: return FColorList::White;
			}
		}

		return FColorList::White;
	}

	FSlateColor GetBackgroundColor() const
	{
		if (!IsExposed) {
			return FColorList::DimGrey;	
		}

		if (IsMine) {
			return FColor::Red;
		}
	
		return FColorList::DarkSlateGrey;
	}
};

class FGeoTechMinesweeperModule: public IModuleInterface
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	int StartMinesweeper();
	
	int GameWidth = 16, GameHeight = 16, GameMineCount = 40;

	TSharedPtr<SBorder> GameArea;
	TSharedPtr<FMinesweeperGame> Minesweeper;
};

