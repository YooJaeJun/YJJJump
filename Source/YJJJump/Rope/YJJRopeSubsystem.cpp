#include "Rope/YJJRopeSubsystem.h"
#include "Rope/YJJRope.h"

int32 UYJJRopeSubsystem::RegisterRope(AYJJRope* Rope)
{
	check(Rope);

	FYJJRopeSegment Segment = Rope->MakeSegment();
	Segment.Id = NextRopeId++;

	Ropes.Add(Rope);
	Segments.Add(Segment);
	return Segment.Id;
}

void UYJJRopeSubsystem::UnregisterRope(AYJJRope* Rope)
{
	const int32 Index = Ropes.IndexOfByKey(Rope);
	if (Index != INDEX_NONE)
	{
		Ropes.RemoveAt(Index);
		Segments.RemoveAt(Index);
	}
}

AYJJRope* UYJJRopeSubsystem::FindRopeActor(int32 RopeId) const
{
	const int32 Index = Segments.IndexOfByPredicate([RopeId](const FYJJRopeSegment& Segment) { return Segment.Id == RopeId; });
	return Index != INDEX_NONE ? Ropes[Index].Get() : nullptr;
}

float UYJJRopeSubsystem::GetLowestHeight() const
{
	float Lowest = TNumericLimits<float>::Max();
	for (const FYJJRopeSegment& Segment : Segments)
	{
		Lowest = FMath::Min(Lowest, Segment.Height);
	}
	return Lowest;
}

const FYJJRopeSegment* UYJJRopeSubsystem::FindRopeBelow(float X, float Z) const
{
	const FYJJRopeSegment* Best = nullptr;
	for (const FYJJRopeSegment& Segment : Segments)
	{
		if (Segment.Height <= Z && X >= Segment.MinX && X <= Segment.MaxX
			&& (!Best || Segment.Height > Best->Height))
		{
			Best = &Segment;
		}
	}
	return Best;
}
