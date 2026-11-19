#include "Rope/YJJRope.h"
#include "Rope/YJJRopeSubsystem.h"
#include "Data/YJJData.h"
#include "Visual/YJJSpriteUtils.h"
#include "YJJAssetPaths.h"
#include "Engine/World.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

AYJJRope::AYJJRope()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	LeftStrand = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("LeftStrand"));
	RightStrand = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("RightStrand"));
	LeftPeg = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("LeftPeg"));
	RightPeg = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("RightPeg"));
	for (UPaperSpriteComponent* Part : { LeftStrand.Get(), RightStrand.Get(), LeftPeg.Get(), RightPeg.Get() })
	{
		Part->SetupAttachment(Root);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	StrandSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::RopeSprite));
	PegSprite = TSoftObjectPtr<UPaperSprite>(FSoftObjectPath(YJJAssetPaths::RopePegSprite));
}

void AYJJRope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySprites();
	ClearContact();
}

void AYJJRope::BeginPlay()
{
	Super::BeginPlay();

	if (UYJJRopeSubsystem* Ropes = GetWorld()->GetSubsystem<UYJJRopeSubsystem>())
	{
		RopeId = Ropes->RegisterRope(this);
	}
	ApplySprites();
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

void AYJJRope::ApplySprites()
{
	UPaperSprite* Strand = StrandSprite.LoadSynchronous();
	LeftStrand->SetSprite(Strand);
	RightStrand->SetSprite(Strand);
	StrandSpriteSize = YJJSpriteUtils::GetSpriteSize(Strand);

	// 말뚝은 로프보다 앞에 그린다
	const FYJJStageTuning& Stage = FYJJData::Get().Stage;
	UPaperSprite* Peg = PegSprite.LoadSynchronous();
	const float HalfWidth = Width * 0.5f;
	YJJSpriteUtils::SetSpriteWithWidth(LeftPeg, Peg, Stage.PegWidth);
	YJJSpriteUtils::SetSpriteWithWidth(RightPeg, Peg, Stage.PegWidth);
	LeftPeg->SetRelativeLocation(FVector(-HalfWidth, Stage.PegDepth, 0.f));
	RightPeg->SetRelativeLocation(FVector(HalfWidth, Stage.PegDepth, 0.f));
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

void AYJJRope::PlaceStrand(UPaperSpriteComponent* Strand, const FVector& From, const FVector& To) const
{
	if (StrandSpriteSize.X <= KINDA_SMALL_NUMBER || StrandSpriteSize.Y <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 스프라이트(XZ 평면)를 두 점 사이에 늘려 붙인다. 피치 회전이 XZ 평면 회전
	const FVector Delta = To - From;
	const double Length = FMath::Max(Delta.Size(), 1.0);
	const FVector Scale(Length / StrandSpriteSize.X, 1.0, FYJJData::Get().Stage.RopeThickness / StrandSpriteSize.Y);

	Strand->SetWorldTransform(FTransform(Delta.Rotation(), (From + To) * 0.5, Scale));
}
