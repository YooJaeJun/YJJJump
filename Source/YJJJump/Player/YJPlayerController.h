#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "YJPlayerController.generated.h"

// 마우스 커서를 보이게 하고 클릭/드래그를 게임 입력으로 받는다
UCLASS()
class YJJJUMP_API AYJPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AYJPlayerController();

protected:
	virtual void BeginPlay() override;
};
