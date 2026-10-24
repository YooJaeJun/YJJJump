#include "Player/YJJPlayerController.h"

AYJJPlayerController::AYJJPlayerController()
{
	bShowMouseCursor = true;
}

void AYJJPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}
