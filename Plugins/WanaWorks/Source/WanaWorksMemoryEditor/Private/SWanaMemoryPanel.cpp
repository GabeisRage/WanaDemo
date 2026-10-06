#include "SWanaMemoryPanel.h"

#include "SWanaRelationshipGraph.h"
#include "WanaMemoryEditorStyle.h"
#include "WanaMemorySubsystem.h"

#include "Containers/Ticker.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

SWanaMemoryPanel::~SWanaMemoryPanel()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }
    if (BoundSubsystem.IsValid() && DelegateHandle.IsValid())
    {
        BoundSubsystem->OnMemoryWrittenNative.Remove(DelegateHandle);
        DelegateHandle.Reset();
    }
}

void SWanaMemoryPanel::Construct(const FArguments& InArgs)
{
    (void)InArgs;

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FWanaMemoryEditorStyle::BackgroundBrush())
        .Padding(16.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("WanaWorks Memory")))
                    .Font(FWanaMemoryEditorStyle::TitleFont())
                    .ColorAndOpacity(FWanaMemoryEditorStyle::TextPrimary())
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Studio-owned timeline, relationships, and WIT / WAY / WAI / WAMI. The language model does not keep this.")))
                    .Font(FWanaMemoryEditorStyle::SmallFont())
                    .ColorAndOpacity(FWanaMemoryEditorStyle::TextMuted())
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SAssignNew(CharacterBox, SEditableTextBox)
                    .Style(&FWanaMemoryEditorStyle::TextBoxStyle())
                    .Font(FWanaMemoryEditorStyle::BodyFont())
                    .HintText(FText::FromString(TEXT("Character id")))
                    .OnTextCommitted(this, &SWanaMemoryPanel::HandleIdCommitted)
                ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SAssignNew(PlayerBox, SEditableTextBox)
                    .Style(&FWanaMemoryEditorStyle::TextBoxStyle())
                    .Font(FWanaMemoryEditorStyle::BodyFont())
                    .HintText(FText::FromString(TEXT("Player id")))
                    .OnTextCommitted(this, &SWanaMemoryPanel::HandleIdCommitted)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .ButtonStyle(&FWanaMemoryEditorStyle::ButtonStyle())
                    .OnClicked(this, &SWanaMemoryPanel::HandleRefreshClicked)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Refresh")))
                        .Font(FWanaMemoryEditorStyle::BodyFont())
                        .ColorAndOpacity(FWanaMemoryEditorStyle::TextPrimary())
                    ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
            [
                SAssignNew(StatusText, STextBlock)
                .Font(FWanaMemoryEditorStyle::SmallFont())
                .ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary())
                .Text(FText::FromString(TEXT("Opening the studio database.")))
            ]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SNew(SSplitter)
                .Orientation(Orient_Horizontal)
                + SSplitter::Slot().Value(0.38f)
                [
                    MakeCard(
                        TEXT("Timeline"),
                        FWanaMemoryEditorStyle::Cyan(),
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SAssignNew(TimelineBox, SVerticalBox)
                        ])
                ]
                + SSplitter::Slot().Value(0.62f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(0.62f).Padding(8.f, 0.f, 0.f, 8.f)
                    [
                        MakeCard(
                            TEXT("Relationships"),
                            FWanaMemoryEditorStyle::Violet(),
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().FillHeight(1.f)
                            [
                                SAssignNew(Graph, SWanaRelationshipGraph)
                                .OnEdgeSelected(this, &SWanaMemoryPanel::HandleEdgeSelected)
                            ]
                            + SVerticalBox::Slot().AutoHeight().MaxHeight(88.f)
                            [
                                SNew(SScrollBox)
                                + SScrollBox::Slot()
                                [
                                    SAssignNew(EdgeList, SVerticalBox)
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
                            [
                                SAssignNew(HistoryText, STextBlock)
                                .Font(FWanaMemoryEditorStyle::SmallFont())
                                .ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary())
                                .AutoWrapText(true)
                                .Text(FText::FromString(TEXT("Select an edge to see trust, affinity, fear, and respect history.")))
                            ])
                    ]
                    + SVerticalBox::Slot().FillHeight(0.38f).Padding(8.f, 0.f, 0.f, 0.f)
                    [
                        MakeCard(
                            TEXT("State"),
                            FWanaMemoryEditorStyle::Gold(),
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SNew(STextBlock).Text(FText::FromString(TEXT("WIT"))).Font(FWanaMemoryEditorStyle::BodyFont()).ColorAndOpacity(FWanaMemoryEditorStyle::Cyan())
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SAssignNew(WitText, STextBlock).Font(FWanaMemoryEditorStyle::SmallFont()).ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary()).AutoWrapText(true)
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SNew(STextBlock).Text(FText::FromString(TEXT("WAY"))).Font(FWanaMemoryEditorStyle::BodyFont()).ColorAndOpacity(FWanaMemoryEditorStyle::Violet())
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SAssignNew(WayText, STextBlock).Font(FWanaMemoryEditorStyle::SmallFont()).ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary()).AutoWrapText(true)
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SNew(STextBlock).Text(FText::FromString(TEXT("WAI"))).Font(FWanaMemoryEditorStyle::BodyFont()).ColorAndOpacity(FWanaMemoryEditorStyle::Gold())
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                                [
                                    SAssignNew(WaiText, STextBlock).Font(FWanaMemoryEditorStyle::SmallFont()).ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary()).AutoWrapText(true)
                                ]
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(STextBlock).Text(FText::FromString(TEXT("WAMI"))).Font(FWanaMemoryEditorStyle::BodyFont()).ColorAndOpacity(FWanaMemoryEditorStyle::TextPrimary())
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
                                [
                                    SAssignNew(WamiText, STextBlock).Font(FWanaMemoryEditorStyle::SmallFont()).ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary()).AutoWrapText(true)
                                ]
                            ])
                    ]
                ]
            ]
        ]
    ];

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &SWanaMemoryPanel::HandleTicker),
        0.5f);
    SyncSubscription();
    Refresh(true);
}

TSharedRef<SWidget> SWanaMemoryPanel::MakeCard(const FString& Title, const FLinearColor& Accent, TSharedRef<SWidget> Body) const
{
    return SNew(SBorder)
        .BorderImage(FWanaMemoryEditorStyle::PanelBrush())
        .Padding(12.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Title))
                .Font(FWanaMemoryEditorStyle::BodyFont())
                .ColorAndOpacity(Accent)
            ]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                Body
            ]
        ];
}

TSharedRef<SWidget> SWanaMemoryPanel::MakeTimelineRow(const FWanaMemoryTimelineItem& Item) const
{
    const FLinearColor SourceColor = Item.bPlayer
        ? FWanaMemoryEditorStyle::Gold()
        : (Item.bAi ? FWanaMemoryEditorStyle::Cyan() : FWanaMemoryEditorStyle::TextSecondary());
    const FString Importance = Item.bHasImportance
        ? FString::Printf(TEXT("importance %.2f"), Item.Importance)
        : FString(TEXT("importance —"));
    FString Meta = FString::Printf(TEXT("%s  ·  %s  ·  %s"), *Item.Timestamp, *Item.Kind, *Importance);
    if (!Item.ProviderId.IsEmpty())
    {
        Meta += FString::Printf(TEXT("  ·  %s"), *Item.ProviderId);
    }

    return SNew(SBorder)
        .BorderImage(FWanaMemoryEditorStyle::InsetBrush())
        .Padding(10.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
                [
                    SNew(SBorder)
                    .BorderImage(FWanaMemoryEditorStyle::InsetBrush())
                    .Padding(FMargin(6.f, 2.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Item.Source))
                        .Font(FWanaMemoryEditorStyle::SmallFont())
                        .ColorAndOpacity(SourceColor)
                    ]
                ]
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Meta))
                    .Font(FWanaMemoryEditorStyle::SmallFont())
                    .ColorAndOpacity(FWanaMemoryEditorStyle::TextMuted())
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item.Text))
                .Font(FWanaMemoryEditorStyle::BodyFont())
                .ColorAndOpacity(FWanaMemoryEditorStyle::TextPrimary())
                .AutoWrapText(true)
            ]
        ];
}

FString SWanaMemoryPanel::IdentityText(const TArray<FWanaMemoryIdentityItem>& States, const TCHAR* Kind)
{
    for (const FWanaMemoryIdentityItem& State : States)
    {
        if (State.StateKind == Kind)
        {
            return FString::Printf(TEXT("schema %d  ·  %s\n%s"), State.SchemaVersion, *State.UpdatedAt, *State.JsonBlob);
        }
    }
    return FString::Printf(TEXT("No %s state stored for this character."), Kind);
}

bool SWanaMemoryPanel::HandleTicker(float DeltaTime)
{
    (void)DeltaTime;
    SyncSubscription();
    if (IsPie() || bLastPie)
    {
        Refresh(false);
    }
    return true;
}

void SWanaMemoryPanel::HandleMemoryWritten(const FString& CharacterId, const FString& PlayerId, const FString& WriteKind)
{
    (void)CharacterId;
    (void)PlayerId;
    (void)WriteKind;
    Refresh(true);
}

void SWanaMemoryPanel::HandleIdCommitted(const FText& Text, ETextCommit::Type CommitType)
{
    (void)Text;
    (void)CommitType;
    Refresh(true);
}

void SWanaMemoryPanel::HandleEdgeSelected(const FString& CharacterId, const FString& PlayerId)
{
    if (CharacterBox.IsValid())
    {
        CharacterBox->SetText(FText::FromString(CharacterId));
    }
    if (PlayerBox.IsValid())
    {
        PlayerBox->SetText(FText::FromString(PlayerId));
    }
    Refresh(true);
}

FReply SWanaMemoryPanel::HandleRefreshClicked()
{
    Refresh(true);
    return FReply::Handled();
}

void SWanaMemoryPanel::SyncSubscription()
{
    UWanaMemorySubsystem* Subsystem = FindSubsystem();
    if (Subsystem == BoundSubsystem.Get())
    {
        return;
    }
    if (BoundSubsystem.IsValid() && DelegateHandle.IsValid())
    {
        BoundSubsystem->OnMemoryWrittenNative.Remove(DelegateHandle);
        DelegateHandle.Reset();
    }
    BoundSubsystem = Subsystem;
    if (Subsystem)
    {
        DelegateHandle = Subsystem->OnMemoryWrittenNative.AddSP(this, &SWanaMemoryPanel::HandleMemoryWritten);
    }
}

FString SWanaMemoryPanel::CurrentCharacterId() const
{
    return CharacterBox.IsValid() ? CharacterBox->GetText().ToString().TrimStartAndEnd() : FString();
}

FString SWanaMemoryPanel::CurrentPlayerId() const
{
    return PlayerBox.IsValid() ? PlayerBox->GetText().ToString().TrimStartAndEnd() : FString();
}

FString SWanaMemoryPanel::DatabasePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("WanaWorks"), TEXT("Memory"), TEXT("wana_memory.db"));
}

bool SWanaMemoryPanel::IsPie() const
{
    return GEditor && GEditor->PlayWorld != nullptr;
}

UWanaMemorySubsystem* SWanaMemoryPanel::FindSubsystem() const
{
    if (!GEditor)
    {
        return nullptr;
    }
    UWorld* World = GEditor->PlayWorld ? GEditor->PlayWorld : GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return nullptr;
    }
    UGameInstance* GameInstance = World->GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UWanaMemorySubsystem>() : nullptr;
}

void SWanaMemoryPanel::Refresh(bool bForce)
{
    const bool bPie = IsPie();
    FString Error;
    if (!View.IsOpen())
    {
        View.Open(DatabasePath(), Error);
    }

    const FString CharacterId = CurrentCharacterId();
    const FString PlayerId = CurrentPlayerId();
    TArray<FWanaMemoryEdge> Edges;
    TArray<FWanaMemoryTimelineItem> Timeline;
    TArray<FWanaMemoryHistoryPoint> History;
    TArray<FWanaMemoryIdentityItem> Identities;
    FString ReadError;
    const bool bHavePair = !CharacterId.IsEmpty() && !PlayerId.IsEmpty();
    if (View.IsOpen())
    {
        if (!View.ReadEdges(Edges, ReadError))
        {
            View.Close();
            Error = ReadError;
        }
        else if (bHavePair)
        {
            View.ReadTimeline(CharacterId, PlayerId, Timeline, ReadError);
            View.ReadHistory(CharacterId, PlayerId, History, ReadError);
        }
        if (View.IsOpen() && !CharacterId.IsEmpty())
        {
            View.ReadIdentities(CharacterId, Identities, ReadError);
        }
    }

    FString Signature = FString::Printf(TEXT("%s|%s|%d|%d"), *CharacterId, *PlayerId, Edges.Num(), Timeline.Num());
    for (const FWanaMemoryEdge& Edge : Edges)
    {
        Signature += FString::Printf(TEXT("|%s/%s/%.3f/%.3f/%.3f/%.3f"), *Edge.CharacterId, *Edge.PlayerId, Edge.Trust, Edge.Affinity, Edge.Fear, Edge.Respect);
    }
    for (const FWanaMemoryTimelineItem& Item : Timeline)
    {
        Signature += TEXT("|") + Item.Timestamp + Item.Kind + Item.Text;
    }
    for (const FWanaMemoryIdentityItem& State : Identities)
    {
        Signature += TEXT("|") + State.StateKind + State.JsonBlob;
    }
    for (const FWanaMemoryHistoryPoint& Point : History)
    {
        Signature += FString::Printf(TEXT("|%s/%.3f"), *Point.Timestamp, Point.Trust);
    }

    if (!bForce && Signature == LastSignature && bPie == bLastPie)
    {
        return;
    }
    LastSignature = Signature;
    bLastPie = bPie;

    if (TimelineBox.IsValid())
    {
        TimelineBox->ClearChildren();
        if (!bHavePair)
        {
            TimelineBox->AddSlot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Enter a character id and a player id, or select an edge.")))
                .Font(FWanaMemoryEditorStyle::SmallFont())
                .ColorAndOpacity(FWanaMemoryEditorStyle::TextMuted())
                .AutoWrapText(true)
            ];
        }
        else if (Timeline.Num() == 0)
        {
            TimelineBox->AddSlot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("No turns or memories for this pair yet.")))
                .Font(FWanaMemoryEditorStyle::SmallFont())
                .ColorAndOpacity(FWanaMemoryEditorStyle::TextMuted())
            ];
        }
        else
        {
            for (const FWanaMemoryTimelineItem& Item : Timeline)
            {
                TimelineBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                [
                    MakeTimelineRow(Item)
                ];
            }
        }
    }

    if (Graph.IsValid())
    {
        Graph->SetSelected(CharacterId, PlayerId);
        Graph->SetEdges(Edges);
    }
    if (EdgeList.IsValid())
    {
        EdgeList->ClearChildren();
        const int32 Shown = FMath::Min(Edges.Num(), 24);
        for (int32 Index = 0; Index < Shown; ++Index)
        {
            const FWanaMemoryEdge Edge = Edges[Index];
            const FString Label = FString::Printf(TEXT("%s  ->  %s    trust %.2f  affinity %.2f  fear %.2f  respect %.2f"),
                *Edge.CharacterId, *Edge.PlayerId, Edge.Trust, Edge.Affinity, Edge.Fear, Edge.Respect);
            EdgeList->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
            [
                SNew(SButton)
                .ButtonStyle(&FWanaMemoryEditorStyle::ButtonStyle())
                .OnClicked_Lambda([this, Edge]()
                {
                    HandleEdgeSelected(Edge.CharacterId, Edge.PlayerId);
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Label))
                    .Font(FWanaMemoryEditorStyle::SmallFont())
                    .ColorAndOpacity(FWanaMemoryEditorStyle::TextSecondary())
                ]
            ];
        }
    }

    if (HistoryText.IsValid())
    {
        if (!bHavePair)
        {
            HistoryText->SetText(FText::FromString(TEXT("Select an edge to see trust, affinity, fear, and respect history.")));
        }
        else if (History.Num() == 0)
        {
            HistoryText->SetText(FText::FromString(TEXT("No relationship history for this edge yet.")));
        }
        else
        {
            FString Lines;
            for (const FWanaMemoryHistoryPoint& Point : History)
            {
                Lines += FString::Printf(TEXT("%s   trust %.2f   affinity %.2f   fear %.2f   respect %.2f\n%s (%s)\n"),
                    *Point.Timestamp, Point.Trust, Point.Affinity, Point.Fear, Point.Respect, *Point.Reason, *Point.Source);
            }
            HistoryText->SetText(FText::FromString(Lines));
        }
    }

    if (WitText.IsValid()) WitText->SetText(FText::FromString(IdentityText(Identities, TEXT("WIT"))));
    if (WayText.IsValid()) WayText->SetText(FText::FromString(IdentityText(Identities, TEXT("WAY"))));
    if (WaiText.IsValid()) WaiText->SetText(FText::FromString(IdentityText(Identities, TEXT("WAI"))));
    if (WamiText.IsValid()) WamiText->SetText(FText::FromString(IdentityText(Identities, TEXT("WAMI"))));

    if (StatusText.IsValid())
    {
        const TCHAR* Live = bPie
            ? TEXT("PIE is running. This tab listens for memory writes and polls twice a second.")
            : TEXT("Reading the saved database. PIE will update this tab live.");
        if (View.IsOpen())
        {
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %d relationships  ·  %s"), Live, Edges.Num(), *DatabasePath())));
        }
        else
        {
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("No memory database yet. Talk to a character in PIE, then this tab fills in. %s"), *DatabasePath())));
        }
    }
}
