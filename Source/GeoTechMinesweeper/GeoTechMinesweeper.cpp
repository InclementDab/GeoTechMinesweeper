// Copyright Epic Games, Inc. All Rights Reserved.

#include "GeoTechMinesweeper.h"

#if WITH_EDITOR
#include "SListViewSelectorDropdownMenu.h"
#endif
#include "Dialog/SMessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/UMGCoreStyle.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SNumericEntryBox.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FGeoTechMinesweeperModule, GeoTechMinesweeper, "GeoTechMinesweeper");

void FGeoTechMinesweeperModule::StartupModule()
{
	IModuleInterface::StartupModule();

#if WITH_EDITOR
	UToolMenu* ToolBar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.ModesToolBar");
	check(ToolBar);
	FToolMenuSection& Section = ToolBar->AddSection("Debug");	
	FToolUIAction Action;
	Action.ExecuteAction.BindLambda([this](const FToolMenuContext&) {
		int Result = StartMinesweeper();
		check(!Result);
	});

	Action.CanExecuteAction.BindLambda([this](const FToolMenuContext&) {
		return !Minesweeper.IsValid();
	});
	
	const FToolMenuEntry MinesweeperEntry = FToolMenuEntry::InitToolBarButton("MinesweeperButton", FToolUIActionChoice(Action), INVTEXT("Minesweeper"), INVTEXT("Begin procrastination."), FSlateIcon(FName("EditorStyle"), "Kismet.VariableList.ArrayTypeIcon"));
	Section.AddEntry(MinesweeperEntry);
#else
	StartMinesweeper();
#endif
}

void FGeoTechMinesweeperModule::ShutdownModule()
{
	IModuleInterface::ShutdownModule();
}

int FGeoTechMinesweeperModule::StartMinesweeper()
{
	FSlateApplication& App = FSlateApplication::Get();
	
	if (!App.CanAddModalWindow()) {
		return -1;
	}

	static FName NAME_Easy = "Easy";
	static FName NAME_Medium = "Medium";
	static FName NAME_Hard = "Hard";
	static FName NAME_Impossible = "Impossible";
	static FName NAME_Custom = "Custom...";
	static TArray<FName> Difficulties = { NAME_Easy, NAME_Medium, NAME_Hard, NAME_Impossible, NAME_Custom };
	static FName CurrentDifficulty = "Medium";
	
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(INVTEXT("Minesweeper"))
		.ClientSize(FVector2D(800.f, 650.f))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SizingRule(ESizingRule::Autosized)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.SizeRule(SSplitter::FractionOfParent)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(7.0f)
				[
					// Game Header blocks
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.Padding(FMargin(0.0f, 2.0f, 3.0f, 2.0f))
						.AutoWidth()
						.HAlign(HAlign_Left)
					[
						SNew(SComboBox<FName>)
						.OptionsSource(&Difficulties)
						.OnSelectionChanged_Lambda([this](FName Value, ESelectInfo::Type InSelectInfo) {
							CurrentDifficulty = Value;
							if (CurrentDifficulty == NAME_Easy) {
								GameWidth = 8;
								GameHeight = 8;
								GameMineCount = 10;
							}
							if (CurrentDifficulty == NAME_Medium) {
								GameWidth = 16;
								GameHeight = 16;
								GameMineCount = 40;
							}
							if (CurrentDifficulty == NAME_Hard) {
								GameWidth = 32;
								GameHeight = 16;
								GameMineCount = 99;
							}

							if (CurrentDifficulty == NAME_Impossible) {
								GameWidth = 32;
								GameHeight = 32;
								GameMineCount = 170;
							}
						})

						.OnGenerateWidget_Lambda([this](FName Value) -> TSharedRef<SWidget> {
							return SNew(STextBlock).Text(FText::FromString(Value.ToString()));
						})
						.InitiallySelectedItem(CurrentDifficulty)
						.IsEnabled_Lambda([this] {
							return !Minesweeper.IsValid();
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this] () {
								return FText::FromString(CurrentDifficulty.ToString());
							})
						]
					]
					
					+ SHorizontalBox::Slot()
					.Padding(FMargin(0.0f, 2.0f, 3.0f, 2.0f))
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						// Width
						SNew(SNumericEntryBox<int>)
						.AllowSpin(false)
						.AllowWheel(true)
						.Label()
						[
							SNumericEntryBox<int>::BuildLabel(INVTEXT("Width"), FLinearColor::White, FLinearColor::Transparent)
						]
						.Value_Lambda([this] {
							return GameWidth;
						})
						.OnValueChanged_Lambda([this](int NewValue) {
							GameWidth = FMath::Clamp(NewValue, 1, 256);
						})
						.IsEnabled_Lambda([this] {
							return !Minesweeper.IsValid() && CurrentDifficulty == NAME_Custom;
						})
					]

					+ SHorizontalBox::Slot()
					.Padding(FMargin(0.0f, 2.0f, 3.0f, 2.0f))
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						// Height
						SNew(SNumericEntryBox<int>)
						.AllowSpin(false)
						.AllowWheel(true)
						.Label()
						[
							SNumericEntryBox<int>::BuildLabel(INVTEXT("Height"), FLinearColor::White, FLinearColor::Transparent)
						]
						.Value_Lambda([this] {
							return GameHeight;
						})
						.OnValueChanged_Lambda([this](int NewValue) {
							GameHeight = FMath::Clamp(NewValue, 1, 256);
						})
						.IsEnabled_Lambda([this] {
							return !Minesweeper.IsValid() && CurrentDifficulty == NAME_Custom;
						})
					]
					
					+ SHorizontalBox::Slot()
                    .Padding(FMargin(0.0f, 2.0f, 3.0f, 2.0f))
                    .AutoWidth()
					.HAlign(HAlign_Left)
                    [
                    	// Mine count
                    	SNew(SNumericEntryBox<int>)
                    	.AllowSpin(false)
                    	.AllowWheel(true)
                    	.Label()
                    	[
                    		SNumericEntryBox<int>::BuildLabel(INVTEXT("Mine Count"), FLinearColor::White, FLinearColor::Transparent)
                    	]
                    	.Value_Lambda([this] {
                    		return GameMineCount;
                    	})
                    	.OnValueChanged_Lambda([this](int NewValue) {
                    		GameMineCount = FMath::Clamp(NewValue, 1, GameHeight * GameWidth);
                    	})
	                    .IsEnabled_Lambda([this] {
	                    	return !Minesweeper.IsValid() && CurrentDifficulty == NAME_Custom;
	                    })
                    ]

					+ SHorizontalBox::Slot()
					.Padding(FMargin(0.0f, 2.0f, 3.0f, 2.0f))
					.AutoWidth()
					.HAlign(HAlign_Right)
					[
						// Start
						SNew(SButton)
						.Text_Lambda([this] {
							if (Minesweeper.IsValid()) {
								return INVTEXT("End");
							}
							
							return INVTEXT("Begin");
						})
						.OnClicked_Lambda([this] {
							if (Minesweeper.IsValid()) {
								Minesweeper.Reset();
								return FReply::Handled();
							}
							
							Minesweeper = MakeShareable<FMinesweeperGame>(new FMinesweeperGame(GameWidth, GameHeight, GameMineCount));
							Minesweeper->SetPlayArea(GameArea);
							return FReply::Handled();
						})
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(GameArea, SBorder)
					.Padding(14)
					.HAlign(HAlign_Center)
				]
			]
		];


	const auto Parent = App.FindBestParentWindowForDialogs(nullptr);
	App.AddModalWindow(Window, Parent, false);
	
	// After modal closed, cleanup
	Minesweeper.Reset();
	
	return 0;
}

FReply SMineButton::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();
	const EButtonClickMethod::Type InputClickMethod = GetClickMethodFromInputType(MouseEvent);
	const bool bMustBePressed = InputClickMethod == EButtonClickMethod::DownAndUp || InputClickMethod == EButtonClickMethod::PreciseClick;

	if (( ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || MouseEvent.IsTouchEvent())))	{
		Release();

		if (IsEnabled())
		{
			if (InputClickMethod != EButtonClickMethod::MouseDown)
			{
				bool bEventOverButton = IsHovered();

				if (!bEventOverButton && MouseEvent.IsTouchEvent())
				{
					bEventOverButton = MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
				}

				if (bEventOverButton)
				{
					Reply = ExecuteOnRightClick();
				}
			}
		}
		
		//If the user of the button didn't handle this click, then the button's
		//default behavior handles it.
		if ( Reply.IsEventHandled() == false )
		{
			Reply = FReply::Handled();
		}
	}

	if (Reply.IsEventHandled()) {
		return Reply;
	}

	return SButton::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SMineButton::ExecuteOnRightClick()
{
	if (OnRightClicked.IsBound())
    {
    	return OnRightClicked.Execute();
    }

	return FReply::Handled();
}

FMinesweeperGame::FMinesweeperGame(const int InWidth, const int InHeight, const int InMineCount)
{
	Width = InWidth;
	Height = InHeight;

	// I suppose you could play on hard mode and make it the total area but thats just kinda weird 
	MineCount = FMath::Clamp(InMineCount, 1, Width * Height);
	
	Board = new FMinesweeperTile[Width * Height];
	PlayAreaGridWidgets = new SGridPanel::FSlot*[Width * Height];

	// Generate play area
	for (int i = 0; i < Width; i++) {
		for (int j = 0; j < Height; j++) {
			// Initialize tile info
			auto& Tile = GetTile(i, j);
			Tile.Game = this;
			Tile.Position = { i, j };
		}
	}
	
	int MineCountCopy = MineCount;
	while (MineCountCopy-- > 0) {
		const int X = FMath::RandRange(0, Width - 1);
		const int Y = FMath::RandRange(0, Height - 1);

		auto& Tile = GetTile(X, Y);
		if (Tile.IsMine) {
			MineCountCopy++;
			continue;
		}

		// Set minefield randomly
		Tile.IsMine = true;
	}
	
	GameState = Playing;
}

FMinesweeperGame::~FMinesweeperGame()
{
	if (PlayBorder) {
		PlayBorder->ClearContent();
	}
		
	delete Board;
	delete PlayAreaGridWidgets;
}

bool FMinesweeperGame::SetPlayArea(TSharedPtr<SBorder> Panel)
{
	if (!Panel) {
		return false;
	}
	
	constexpr double ButtonSizePx = 32.0;
	constexpr double ButtonImageRelativeSize = 0.78;
	const FVector2d ButtonSize(ButtonSizePx, ButtonSizePx);
	
	const TSharedPtr<SVerticalBox> VerticalBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SAssignNew(PlayAreaWidget, SGridPanel)
		]
		+ SVerticalBox::Slot()
	    .AutoHeight()
	    [
	        SNew(SHorizontalBox)
	        + SHorizontalBox::Slot()
	        [
        		SNew(STextBlock)
        		.Text_Lambda([this] {
        			const FString MinesRemainingText = FString::Printf(L"Mines Remaining: %d", FMath::Max<int>(0, MineCount - FlagsPlaced));
					return FText::FromString(MinesRemainingText);
        		})
	        ]
	    ];
	
	for (int i = 0; i < Width; i++) {
		for (int j = 0; j < Height; j++) {
			FMinesweeperTile* Tile = GetTilePtr(i, j);
			
			auto Slot = PlayAreaWidget->AddSlot(i, j);
			Slot
			[
				SNew(SBox)
				.MinDesiredHeight(ButtonSizePx)
				.MinDesiredWidth(ButtonSizePx)
				.MaxDesiredHeight(ButtonSizePx)
				.MinDesiredHeight(ButtonSizePx)
				.MaxAspectRatio(1.0f)
				[
					SNew(SBorder)
					.ColorAndOpacity(FColor::White)
					.BorderBackgroundColor(FSlateColor(FColorList::Grey))
					.Padding(0)
					[
						SAssignNew(Tile->Button, SMineButton)
						.IsEnabled_Raw(Tile, &FMinesweeperTile::IsEnabled)
						.OnClicked_Raw(Tile, &FMinesweeperTile::OnClicked)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.ButtonColorAndOpacity(FSlateColor(FColorList::Blue))
						.ContentPadding(0)
						[							
							SNew(SCanvas)
							+ SCanvas::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Position(ButtonSize / (2.0 + 1 - ButtonImageRelativeSize))
							.Size(ButtonSize * ButtonImageRelativeSize)
							[
								SNew(SImage)
								.Image(FSlateIcon(FName("CoreStyle"), "GenericWhiteBox").GetIcon())
								.ColorAndOpacity_Raw(Tile, &FMinesweeperTile::GetBackgroundColor)
							]
							
							+ SCanvas::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Position(ButtonSize / (2.0 + 1 - ButtonImageRelativeSize))
							.Size(ButtonSize * ButtonImageRelativeSize)
							[
								SAssignNew(Tile->Image, SImage)
								.ColorAndOpacity_Raw(Tile, &FMinesweeperTile::GetColor)
								.Image_Raw(Tile, &FMinesweeperTile::GetImage)
							]

							+ SCanvas::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Position(ButtonSize / (2.0 + 1 - ButtonImageRelativeSize))
							.Size(ButtonSize * ButtonImageRelativeSize)
							[
								SAssignNew(Tile->Text, STextBlock)
								.Text_Raw(Tile, &FMinesweeperTile::GetText)
								.ColorAndOpacity_Raw(Tile, &FMinesweeperTile::GetColor)
								.TextStyle(&FSlateStyleRegistry::FindSlateStyle("StateTreeEditorStyle")->GetWidgetStyle<FTextBlockStyle>("StateTree.State.Title"))
							]
						]
					]
				]
			];

			// Not a huge fan of this, but unless I want to write an entire button class for a demo its the only way I know how to get SButton bindings for right click
			// (without copying & pasting the *entire* class)
			Tile->Button->OnRightClicked.BindRaw(Tile, &FMinesweeperTile::OnRightClicked);

			// Flattened index
			const int Index = i + j * Width;
			
			// Assign the grid slot to the play area array
			PlayAreaGridWidgets[Index] = Slot.GetSlot();
		}
	}

	Panel->SetContent(VerticalBox.ToSharedRef());
	PlayBorder = Panel;
	return true;
}

void FMinesweeperGame::SetState(EMinesweeperGameState State)
{
	GameState = State;

	static int TotalPlayCount = 0;
	TotalPlayCount++;
	
	FString Message;
	switch (GameState) {
	case FinishWin: {
		Message = "You've Won!! Congratulations.";
		break;
	}
	case FinishLose: {
		Message = "Game Over! Try again, if you dare!";
		break;
	}

	default: {
		break;
	}
	}
	
	TArray Buttons = {
		SMessageDialog::FButton(INVTEXT("OK"))
	};

	if (GameState == FinishLose) {
		// FSimpleDelegate RestartDelegate;
		// RestartDelegate.BindLambda([this] {
		// 	
		// });
		//
		// Buttons.Add(SMessageDialog::FButton(INVTEXT("Restart"), RestartDelegate));
	}

	if (TotalPlayCount > 5) {
		Message += L"\nWow, you're having some fun. You should check out my Github!";
		FSimpleDelegate GithubDelegate;
		GithubDelegate.BindLambda([] {
			FString Error;
			FPlatformProcess::LaunchURL(L"https://github.com/InclementDab/", nullptr, &Error);
		});
		
		Buttons.Add(SMessageDialog::FButton(INVTEXT("Github"), GithubDelegate));
	}

	if (IsGameComplete()) {
		if (GameState == FinishLose) {
			for (int i = 0; i < Width; i++) {
				for (int j = 0; j < Height; j++) {
					auto& Tile = GetTile(i, j);
					if (Tile.IsMine) {
						// Not calling the function on purpose. infinite loop
						Tile.IsExposed = true;
					}
				}
			}
		}

		FSlateApplication& App = FSlateApplication::Get();
		if (App.CanAddModalWindow()) {
			TSharedRef<SMessageDialog> Dialog = SNew(SMessageDialog)
				.Title(INVTEXT("Thanks for playing!"))
				.Buttons(Buttons)
				.Message(FText::FromString(Message));

			// This is forcing the application to run a full frame before showing the modal dialog.
			// and it is the only thing i've used an LLM for on this project
			App.Tick();
			App.PumpMessages();
			App.Tick();
			
			Dialog->ShowModal();
		}
	}
}

bool FMinesweeperGame::IsGameComplete() const
{
	return GameState == FinishLose || GameState == FinishWin;
}

FMinesweeperTile& FMinesweeperGame::GetTile(const int X, const int Y) const
{
	check(X < Width && Y < Height && X >= 0 && Y >= 0);
	
	return Board[X + Y * Width];
}

FMinesweeperTile* FMinesweeperGame::GetTilePtr(const int X, const int Y) const
{
	check(X < Width && Y < Height && X >= 0 && Y >= 0);
	
	return &Board[X + Y * Width];
}

int FMinesweeperGame::GetMineCountInArea(const FIntPoint Position, const int DistanceFromCenter) const
{
	int AreaMineCount = 0;
	for (int i = Position[0] - DistanceFromCenter; i <= DistanceFromCenter + Position[0]; i++) {
		for (int j = Position[1] - DistanceFromCenter; j <= DistanceFromCenter + Position[1]; j++) {
			// Not to care about the center tile
			if (i == Position[0] && j == Position[1]) {
				continue;
			}

			// cannot be mines outside of map
			if (i < 0 || i >= Width) {
				continue;	
			}

			if (j < 0 || j >= Height) {
				continue;
			}
	
			AreaMineCount += GetTile(i, j).IsMine;
		}
	}

	return AreaMineCount;
}

void FMinesweeperGame::UnlockSurroundingTilesIfEmpty(const FIntPoint Position, const int DistanceFromCenter, TArray<FIntPoint>& CheckedPositions)
{
	CheckedPositions.Add(Position);

	if (GetMineCountInArea(Position, DistanceFromCenter)) {
		return;
	}

	// slow. but functional. wont check things twice but will require a O(n) array look up at each iteration
	// making this O(nlogn) i think
	for (int i = Position[0] - DistanceFromCenter; i <= DistanceFromCenter + Position[0]; i++) {
		for (int j = Position[1] - DistanceFromCenter; j <= DistanceFromCenter + Position[1]; j++) {
			if (i == Position[0] && j == Position[1]) {
				continue;
			}
			
			FIntPoint TilePosition = { i, j };
			// cannot be mines outside of map
			if (TilePosition[0] < 0 || TilePosition[0] >= Width) {
				continue;	
			}

			if (TilePosition[1] < 0 || TilePosition[1] >= Height) {
				continue;
			}
			
			if (CheckedPositions.Contains(TilePosition)) {
				continue;
			}
			
			const int Count = GetMineCountInArea(TilePosition, DistanceFromCenter);
			if (!Count) {
				UnlockSurroundingTilesIfEmpty(TilePosition, DistanceFromCenter, CheckedPositions);
			}			
			
            auto& Tile = GetTile(TilePosition[0], TilePosition[1]);
            Tile.Expose();
		}
	}
}

void FMinesweeperGame::OnTileExposed(FMinesweeperTile* Tile)
{
	SpacesExposed++;
		
	// handle distance to mine logic, if we arent a mine
	if (!Tile->IsMine) {
		Tile->MinesInArea = GetMineCountInArea(Tile->Position, 1);
	}
	
	// Womp womp
	if (Tile->IsMine) {
		SetState(FinishLose);
		return;
	}

	// You won!!
	if ((Width * Height - MineCount) <= SpacesExposed) {
		SetState(FinishWin);
	}
}
