#include "Rope/YJRopeSubsystem.h"
#include "Rope/YJRope.h"

int32 UYJRopeSubsystem::RegisterRope(AYJRope* Rope)
{
	check(Rope);

	FYJRopeSegment Segment = Rope->MakeSegment();
	Segment.Id = NextRopeId++;

	Ropes.Add(Rope);
	Segments.Add(Segment);
	return Segment.Id;
}

void UYJRopeSubsystem::UnregisterRope(AYJRope* Rope)
{
	const int32 Index = Ropes.IndexOfByKey(Rope);
	if (Index != INDEX_NONE)
	{
		Ropes.RemoveAt(Index);
		Segments.RemoveAt(Index);
	}
}

AYJRope* UYJRopeSubsystem::FindRopeActor(int32 RopeId) const
{
	const int32 Index = Segments.IndexOfByPredicate([RopeId](const FYJRopeSegment& Segment) { return Segment.Id == RopeId; });
	return Index != INDEX_NONE ? Ropes[Index].Get() : nullptr;
}

float UYJRopeSubsystem::GetLowestHeight() const
{
	float Lowest = TNumericLimits<float>::Max();
	for (const FYJRopeSegment& Segment : Segments)
	{
		Lowest = FMath::Min(Lowest, Segment.Height);
	}
	return Lowest;
}

const FYJRopeSegment* UYJRopeSubsystem::FindRopeBelow(float X, float Z) const
{
	const FYJRopeSegment* Best = nullptr;
	for (const FYJRopeSegment& Segment : Segments)
	{
		if (Segment.Height <= Z && X >= Segment.MinX && X <= Segment.MaxX
			&& (!Best || Segment.Height > Best->Height))
		{
			Best = &Segment;
		}
	}
	return Best;
}
