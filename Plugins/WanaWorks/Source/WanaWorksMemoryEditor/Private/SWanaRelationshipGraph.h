#pragma once

#include "WanaMemoryQuery.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_TwoParams(FOnWanaEdgeSelected, const FString& /*CharacterId*/, const FString& /*PlayerId*/);

class SWanaRelationshipGraph : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaRelationshipGraph) {}
        SLATE_EVENT(FOnWanaEdgeSelected, OnEdgeSelected)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetEdges(const TArray<FWanaMemoryEdge>& InEdges);
    void SetSelected(const FString& InCharacterId, const FString& InPlayerId);

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
    struct FNodeBox
    {
        FString Id;
        bool bCharacter = false;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Size = FVector2D(140.f, 32.f);
    };

    struct FEdgeLine
    {
        FString CharacterId;
        FString PlayerId;
        FVector2D Start = FVector2D::ZeroVector;
        FVector2D End = FVector2D::ZeroVector;
        FLinearColor Color = FLinearColor::White;
        float Thickness = 2.f;
        bool bSelected = false;
    };

    void BuildLayout(const FVector2D& Size, TArray<FNodeBox>& OutNodes, TArray<FEdgeLine>& OutLines) const;
    FLinearColor ColorForEdge(const FWanaMemoryEdge& Edge) const;

    TArray<FWanaMemoryEdge> Edges;
    FString SelectedCharacterId;
    FString SelectedPlayerId;
    FOnWanaEdgeSelected OnEdgeSelected;
    mutable TArray<FNodeBox> CachedNodes;
    mutable TArray<FEdgeLine> CachedLines;
};
