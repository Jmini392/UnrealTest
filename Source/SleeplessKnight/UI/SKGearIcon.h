#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SLeafWidget.h"
#include "SKGearIcon.generated.h"

/** Gear outline drawn with lines, so it needs no texture and no icon font. */
class SGearIcon : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGearIcon) {}
		SLATE_ARGUMENT(int32, TeethCount)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(40.f, 40.f); }

private:
	int32 TeethCount = 8;
};

/** UMG wrapper so the gear can sit inside a Button. */
UCLASS()
class SLEEPLESSKNIGHT_API USKGearIcon : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
	int32 TeethCount = 8;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SGearIcon> MyGear;
};
