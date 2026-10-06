#pragma once

#include "WanaMemoryQuery.h"

#include "Containers/Ticker.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;
class SVerticalBox;
class SWanaRelationshipGraph;
class UWanaMemorySubsystem;

class SWanaMemoryPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaMemoryPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SWanaMemoryPanel() override;

private:
    bool HandleTicker(float DeltaTime);
    void HandleMemoryWritten(const FString& CharacterId, const FString& PlayerId, const FString& WriteKind);
    void HandleIdCommitted(const FText& Text, ETextCommit::Type CommitType);
    void HandleEdgeSelected(const FString& CharacterId, const FString& PlayerId);
    FReply HandleRefreshClicked();
    void SyncSubscription();
    void Refresh(bool bForce);
    FString CurrentCharacterId() const;
    FString CurrentPlayerId() const;
    FString DatabasePath() const;
    bool IsPie() const;
    UWanaMemorySubsystem* FindSubsystem() const;
    TSharedRef<SWidget> MakeCard(const FString& Title, const FLinearColor& Accent, TSharedRef<SWidget> Body) const;
    TSharedRef<SWidget> MakeTimelineRow(const FWanaMemoryTimelineItem& Item) const;
    static FString IdentityText(const TArray<FWanaMemoryIdentityItem>& States, const TCHAR* Kind);

    TSharedPtr<SEditableTextBox> CharacterBox;
    TSharedPtr<SEditableTextBox> PlayerBox;
    TSharedPtr<SVerticalBox> TimelineBox;
    TSharedPtr<SWanaRelationshipGraph> Graph;
    TSharedPtr<SVerticalBox> EdgeList;
    TSharedPtr<STextBlock> HistoryText;
    TSharedPtr<STextBlock> WitText;
    TSharedPtr<STextBlock> WayText;
    TSharedPtr<STextBlock> WaiText;
    TSharedPtr<STextBlock> WamiText;
    TSharedPtr<STextBlock> StatusText;

    FWanaMemoryDatabaseView View;
    TWeakObjectPtr<UWanaMemorySubsystem> BoundSubsystem;
    FDelegateHandle DelegateHandle;
    FTSTicker::FDelegateHandle TickerHandle;
    FString LastSignature;
    bool bLastPie = false;
};
