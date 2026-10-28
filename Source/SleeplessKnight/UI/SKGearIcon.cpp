#include "UI/SKGearIcon.h"
#include "Rendering/DrawElements.h"

void SGearIcon::Construct(const FArguments& InArgs)
{
	TeethCount = FMath::Clamp(InArgs._TeethCount, 4, 16);
}

int32 SGearIcon::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = AllottedGeometry.GetLocalSize();
	const FVector2f Center = Size * 0.5f;
	const float Outer = FMath::Min(Size.X, Size.Y) * 0.5f - 3.f;
	const float Root = Outer * 0.76f;
	const float Hole = Outer * 0.36f;

	// Outline of the toothed rim: root -> tip -> tip -> root for every tooth, angles in radians.
	const float ToothStep = 2.f * PI / TeethCount;
	TArray<FVector2f> Rim;
	for (int32 i = 0; i < TeethCount; ++i)
	{
		const float C = i * ToothStep;
		const float Angles[4] = { C - ToothStep * 0.28f, C - ToothStep * 0.16f, C + ToothStep * 0.16f, C + ToothStep * 0.28f };
		const float Radii[4] = { Root, Outer, Outer, Root };
		for (int32 k = 0; k < 4; ++k)
		{
			Rim.Add(Center + FVector2f(FMath::Cos(Angles[k]), FMath::Sin(Angles[k])) * Radii[k]);
		}
	}
	// Close the outline. Copy first: adding an element of the array to itself asserts when it reallocates.
	const FVector2f FirstPoint = Rim[0];
	Rim.Add(FirstPoint);

	TArray<FVector2f> Ring;
	constexpr int32 HoleSegments = 24;
	for (int32 i = 0; i <= HoleSegments; ++i)
	{
		const float A = 2.f * PI * i / HoleSegments;
		Ring.Add(Center + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Hole);
	}

	const FLinearColor Light = IsHovered() ? FLinearColor(1.f, 0.9f, 0.4f) : FLinearColor::White;
	const FLinearColor Dark(0.f, 0.f, 0.f, 0.85f);
	const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry();

	// Dark underlay then bright line, so the icon reads on any background.
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Paint, Rim, ESlateDrawEffect::None, Dark, true, 5.f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Paint, Ring, ESlateDrawEffect::None, Dark, true, 5.f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Paint, Rim, ESlateDrawEffect::None, Light, true, 2.5f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Paint, Ring, ESlateDrawEffect::None, Light, true, 2.5f);

	return LayerId + 1;
}

TSharedRef<SWidget> USKGearIcon::RebuildWidget()
{
	MyGear = SNew(SGearIcon).TeethCount(TeethCount);
	return MyGear.ToSharedRef();
}

void USKGearIcon::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyGear.Reset();
}
