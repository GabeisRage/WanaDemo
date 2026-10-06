#include "SWanaRelationshipGraph.h"

#include "WanaMemoryEditorStyle.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"

void SWanaRelationshipGraph::Construct(const FArguments& InArgs)
{
    OnEdgeSelected = InArgs._OnEdgeSelected;
}

void SWanaRelationshipGraph::SetEdges(const TArray<FWanaMemoryEdge>& InEdges)
{
    Edges = InEdges;
    Invalidate(EInvalidateWidgetReason::Paint);
}

void SWanaRelationshipGraph::SetSelected(const FString& InCharacterId, const FString& InPlayerId)
{
    SelectedCharacterId = InCharacterId;
    SelectedPlayerId = InPlayerId;
    Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SWanaRelationshipGraph::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    (void)LayoutScaleMultiplier;
    return FVector2D(520.f, 320.f);
}

FLinearColor SWanaRelationshipGraph::ColorForEdge(const FWanaMemoryEdge& Edge) const
{
    const float Trust = FMath::Max(Edge.Trust, 0.f);
    const float Affinity = FMath::Max(Edge.Affinity, 0.f);
    const float Fear = FMath::Max(Edge.Fear, 0.f);
    const float Respect = FMath::Max(Edge.Respect, 0.f);
    const float Sum = FMath::Max(Trust + Affinity + Fear + Respect, 0.001f);
    const FLinearColor RespectColor = FWanaMemoryEditorStyle::TextPrimary();
    return (FWanaMemoryEditorStyle::Cyan() * Trust
        + FWanaMemoryEditorStyle::Gold() * Affinity
        + FWanaMemoryEditorStyle::Violet() * Fear
        + RespectColor * Respect) / Sum;
}

void SWanaRelationshipGraph::BuildLayout(const FVector2D& Size, TArray<FNodeBox>& OutNodes, TArray<FEdgeLine>& OutLines) const
{
    OutNodes.Reset();
    OutLines.Reset();

    TArray<FString> Characters;
    TArray<FString> Players;
    for (const FWanaMemoryEdge& Edge : Edges)
    {
        Characters.AddUnique(Edge.CharacterId);
        Players.AddUnique(Edge.PlayerId);
    }

    const float NodeWidth = 148.f;
    const float NodeHeight = 32.f;
    const float Gap = 8.f;
    const int32 Rows = FMath::Max(Characters.Num(), Players.Num());
    const float ContentHeight = Rows > 0 ? Rows * NodeHeight + (Rows - 1) * Gap : 0.f;
    const float Top = FMath::Max(16.f, (Size.Y - ContentHeight) * 0.5f);

    auto Place = [&OutNodes](const TArray<FString>& Ids, bool bCharacter, float X, float StartY, float Height, float Spacing)
    {
        for (int32 Index = 0; Index < Ids.Num(); ++Index)
        {
            FNodeBox Node;
            Node.Id = Ids[Index];
            Node.bCharacter = bCharacter;
            Node.Position = FVector2D(X, StartY + Index * (Height + Spacing));
            Node.Size = FVector2D(148.f, Height);
            OutNodes.Add(Node);
        }
    };
    Place(Characters, true, 16.f, Top, NodeHeight, Gap);
    Place(Players, false, FMath::Max(180.f, Size.X - NodeWidth - 16.f), Top, NodeHeight, Gap);

    auto FindNode = [&OutNodes](const FString& Id, bool bCharacter) -> const FNodeBox*
    {
        for (const FNodeBox& Node : OutNodes)
        {
            if (Node.bCharacter == bCharacter && Node.Id == Id)
            {
                return &Node;
            }
        }
        return nullptr;
    };

    for (const FWanaMemoryEdge& Edge : Edges)
    {
        const FNodeBox* Character = FindNode(Edge.CharacterId, true);
        const FNodeBox* Player = FindNode(Edge.PlayerId, false);
        if (!Character || !Player)
        {
            continue;
        }
        FEdgeLine Line;
        Line.CharacterId = Edge.CharacterId;
        Line.PlayerId = Edge.PlayerId;
        Line.Start = FVector2D(Character->Position.X + Character->Size.X, Character->Position.Y + Character->Size.Y * 0.5f);
        Line.End = FVector2D(Player->Position.X, Player->Position.Y + Player->Size.Y * 0.5f);
        Line.bSelected = Edge.CharacterId == SelectedCharacterId && Edge.PlayerId == SelectedPlayerId;
        Line.Color = ColorForEdge(Edge);
        Line.Color.A = Line.bSelected ? 1.f : 0.45f;
        const float Weight = FMath::Clamp((Edge.Trust + Edge.Affinity + Edge.Fear + Edge.Respect) / 4.f, 0.f, 1.f);
        Line.Thickness = Line.bSelected ? 2.5f + Weight * 4.f : 1.25f + Weight * 3.f;
        OutLines.Add(Line);
    }
}

int32 SWanaRelationshipGraph::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    (void)Args;
    (void)MyCullingRect;
    (void)InWidgetStyle;
    (void)bParentEnabled;

    const FVector2D Size = AllottedGeometry.GetLocalSize();
    BuildLayout(Size, CachedNodes, CachedLines);

    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    int32 Layer = LayerId;
    static const FSlateRoundedBoxBrush CharacterNode(
        FWanaMemoryEditorStyle::Panel(), 8.f, FWanaMemoryEditorStyle::Violet(), 1.f);
    static const FSlateRoundedBoxBrush PlayerNode(
        FWanaMemoryEditorStyle::Panel(), 8.f, FWanaMemoryEditorStyle::Cyan(), 1.f);
    static const FSlateRoundedBoxBrush SelectedNode(
        FWanaMemoryEditorStyle::Inset(), 8.f, FWanaMemoryEditorStyle::Gold(), 1.5f);

    if (Edges.Num() == 0)
    {
        FSlateDrawElement::MakeText(
            OutDrawElements,
            Layer,
            AllottedGeometry.ToPaintGeometry(FVector2f(Size.X - 32.f, 24.f), FSlateLayoutTransform(FVector2f(16.f, 16.f))),
            TEXT("No relationship rows yet."),
            Font,
            ESlateDrawEffect::None,
            FWanaMemoryEditorStyle::TextMuted());
        return Layer;
    }

    for (const FEdgeLine& Line : CachedLines)
    {
        TArray<FVector2f> Points;
        Points.Add(FVector2f(Line.Start));
        Points.Add(FVector2f(Line.End));
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            Layer,
            AllottedGeometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Line.Color,
            true,
            Line.Thickness);
    }
    ++Layer;

    for (const FNodeBox& Node : CachedNodes)
    {
        const bool bSelected = Node.bCharacter
            ? Node.Id == SelectedCharacterId
            : Node.Id == SelectedPlayerId;
        const FSlateBrush* NodeBrush = bSelected
            ? static_cast<const FSlateBrush*>(&SelectedNode)
            : (Node.bCharacter ? static_cast<const FSlateBrush*>(&CharacterNode) : static_cast<const FSlateBrush*>(&PlayerNode));
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            Layer,
            AllottedGeometry.ToPaintGeometry(FVector2f(Node.Size), FSlateLayoutTransform(FVector2f(Node.Position))),
            NodeBrush,
            ESlateDrawEffect::None,
            FLinearColor::White);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            Layer + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(Node.Size.X - 16.f, 16.f), FSlateLayoutTransform(FVector2f(Node.Position.X + 8.f, Node.Position.Y + 8.f))),
            Node.Id,
            Font,
            ESlateDrawEffect::None,
            FWanaMemoryEditorStyle::TextPrimary());
    }
    return Layer + 1;
}

FReply SWanaRelationshipGraph::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    BuildLayout(MyGeometry.GetLocalSize(), CachedNodes, CachedLines);

    const FEdgeLine* Best = nullptr;
    float BestDistance = 10.f;
    for (const FEdgeLine& Line : CachedLines)
    {
        const FVector2D Segment = Line.End - Line.Start;
        const float LengthSquared = Segment.SizeSquared();
        float Distance = 0.f;
        if (LengthSquared < 0.001f)
        {
            Distance = FVector2D::Distance(Local, Line.Start);
        }
        else
        {
            const float T = FMath::Clamp(FVector2D::DotProduct(Local - Line.Start, Segment) / LengthSquared, 0.f, 1.f);
            Distance = FVector2D::Distance(Local, Line.Start + Segment * T);
        }
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = &Line;
        }
    }
    if (Best && OnEdgeSelected.IsBound())
    {
        OnEdgeSelected.Execute(Best->CharacterId, Best->PlayerId);
        return FReply::Handled();
    }

    for (const FNodeBox& Node : CachedNodes)
    {
        const FBox2D Box(Node.Position, Node.Position + Node.Size);
        if (!Box.IsInside(Local))
        {
            continue;
        }
        for (const FEdgeLine& Line : CachedLines)
        {
            const bool bMatch = Node.bCharacter ? Line.CharacterId == Node.Id : Line.PlayerId == Node.Id;
            if (bMatch && OnEdgeSelected.IsBound())
            {
                OnEdgeSelected.Execute(Line.CharacterId, Line.PlayerId);
                return FReply::Handled();
            }
        }
    }
    return FReply::Unhandled();
}
