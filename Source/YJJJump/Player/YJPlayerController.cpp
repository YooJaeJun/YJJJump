#include "Player/YJPlayerController.h"

AYJPlayerController::AYJPlayerController()
{
	bShowMouseCursor = true;
}

void AYJPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}
