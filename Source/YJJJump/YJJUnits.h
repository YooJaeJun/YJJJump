#pragma once

#include "CoreMinimal.h"

// 단위 변환 (언리얼 기본 단위는 cm)
namespace YJJUnits
{
	inline constexpr float CmPerMeter = 100.f;

	// cm/s -> km/h
	inline constexpr float CmPerSecondToKmh = 0.036f;
}
