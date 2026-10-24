#include "Rope/YJJRope.h"
#include "Rope/YJJRopeSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AYJJRope::AYJJRope()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// 프로토타입 비주얼: 엔진 기본 큐브(100cm)를 늘려서 줄 두 가닥으로 쓴다
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	LeftStrand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftStrand"));
	RightStrand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightStrand"));
	for (UStaticMeshComponent* Strand : { LeftStrand.Get(), RightStrand.Get() })
	{
		Strand->SetupAttachment(Root);
		Strand->SetMobility(EComponentMobility::Movable);
		Strand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Strand->SetCastShadow(false);
		if (CubeMesh.Succeeded())
		{
			Strand->SetStaticMesh(CubeMesh.Object);
		}
	}
}

void AYJJRope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ClearContact();
}

void AYJJRope::BeginPlay()
{
	Super::BeginPlay();

	if (UYJJRopeSubsystem* Ropes = GetWorld()->GetSubsystem<UYJJRopeSubsystem>())
	{
		RopeId = Ropes->RegisterRope(this);
	}
	ClearContact();
}

void AYJJRope::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UYJJRopeSubsystem* Ropes = GetWorld()->GetSubsystem<UYJJRopeSubsystem>())
	{
		Ropes->UnregisterRope(this);
	}
	Super::EndPlay(EndPlayReason);
}

FYJJRopeSegment AYJJRope::MakeSegment() const
{
	const FVector3f Center(GetActorLocation());

	FYJJRopeSegment Segment;
	Segment.Id = RopeId;
	Segment.MinX = Center.X - Width * 0.5f;
	Segment.MaxX = Center.X + Width * 0.5f;
	Segment.Height = Center.Z;
	return Segment;
}

void AYJJRope::SetContact(float ContactX, float Deflection)
{
	UpdateVisual(ContactX, FMath::Max(Deflection, 0.f));
}

void AYJJRope::ClearContact()
{
	UpdateVisual(GetActorLocation().X, 0.f);
}

void AYJJRope::UpdateVisual(float ContactX, float Deflection)
{
	const FVector Center = GetActorLocation();
	const FVector LeftAnchor(Center.X - Width * 0.5f, Center.Y, Center.Z);
	const FVector RightAnchor(Center.X + Width * 0.5f, Center.Y, Center.Z);
	const FVector Contact(FMath::Clamp<double>(ContactX, LeftAnchor.X, RightAnchor.X), Center.Y, Center.Z - Deflection);

	PlaceStrand(LeftStrand, LeftAnchor, Contact);
	PlaceStrand(RightStrand, Contact, RightAnchor);
}

void AYJJRope::PlaceStrand(UStaticMeshComponent* Strand, const FVector& From, const FVector& To) const
{
	const FVector Delta = To - From;
	const double Length = FMath::Max(Delta.Size(), 1.0);
	const float Scale = Thickness / 100.f;

	Strand->SetWorldTransform(FTransform(Delta.Rotation(), (From + To) * 0.5, FVector(Length / 100.0, Scale, Scale)));
}
