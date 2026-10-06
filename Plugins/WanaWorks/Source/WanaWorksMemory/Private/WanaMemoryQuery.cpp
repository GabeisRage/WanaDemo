#include "WanaMemoryQuery.h"

#include "WanaMemoryStore.h"

namespace
{

std::string ToUtf8(const FString& Text)
{
    return std::string(TCHAR_TO_UTF8(*Text));
}

FString FromUtf8(const std::string& Text)
{
    return FString(UTF8_TO_TCHAR(Text.c_str()));
}

FString Fail(const WanaMemory::Status& Status, FString& OutError)
{
    OutError = FromUtf8(Status.Message);
    return OutError;
}

void ClassifySource(const std::string& RoleOrSource, bool bTurn, FWanaMemoryTimelineItem& Item)
{
    const FString Raw = FromUtf8(RoleOrSource);
    if (bTurn)
    {
        if (Raw == TEXT("user"))
        {
            Item.Source = TEXT("Player");
            Item.bPlayer = true;
        }
        else if (Raw == TEXT("assistant"))
        {
            Item.Source = TEXT("AI");
            Item.bAi = true;
        }
        else
        {
            Item.Source = Raw;
        }
        return;
    }

    if (Raw == TEXT("model") || Raw == TEXT("synthesized") || Raw == TEXT("conversation"))
    {
        Item.Source = TEXT("AI");
        Item.bAi = true;
    }
    else if (Raw == TEXT("manual"))
    {
        Item.Source = TEXT("Player");
        Item.bPlayer = true;
    }
    else
    {
        Item.Source = Raw.IsEmpty() ? FString(TEXT("Memory")) : Raw;
    }
}

} // namespace

struct FWanaMemoryDatabaseView::FImpl
{
    WanaMemory::MemoryStore Store;
};

FWanaMemoryDatabaseView::FWanaMemoryDatabaseView()
    : Impl(MakeUnique<FImpl>())
{
}

FWanaMemoryDatabaseView::~FWanaMemoryDatabaseView()
{
    Close();
}

bool FWanaMemoryDatabaseView::Open(const FString& Path, FString& OutError)
{
    Close();
    const WanaMemory::Status Opened = Impl->Store.OpenView(ToUtf8(Path));
    if (!Opened.bOk)
    {
        Fail(Opened, OutError);
        return false;
    }
    OutError.Reset();
    return true;
}

void FWanaMemoryDatabaseView::Close()
{
    if (Impl)
    {
        Impl->Store.Close();
    }
}

bool FWanaMemoryDatabaseView::IsOpen() const
{
    return Impl && Impl->Store.IsOpen();
}

bool FWanaMemoryDatabaseView::ReadTimeline(const FString& CharacterId, const FString& PlayerId, TArray<FWanaMemoryTimelineItem>& OutItems, FString& OutError)
{
    OutItems.Reset();
    if (!IsOpen())
    {
        OutError = TEXT("The memory database is not open.");
        return false;
    }
    std::vector<WanaMemory::Turn> Turns;
    const WanaMemory::Status TurnStatus = Impl->Store.GetRecentTurns(ToUtf8(CharacterId), ToUtf8(PlayerId), 200, Turns);
    if (!TurnStatus.bOk)
    {
        Fail(TurnStatus, OutError);
        return false;
    }
    std::vector<WanaMemory::SalientMemory> Memories;
    const WanaMemory::Status MemoryStatus = Impl->Store.ListMemories(ToUtf8(CharacterId), ToUtf8(PlayerId), 200, Memories);
    if (!MemoryStatus.bOk)
    {
        Fail(MemoryStatus, OutError);
        return false;
    }

    for (const WanaMemory::Turn& Turn : Turns)
    {
        FWanaMemoryTimelineItem Item;
        Item.Kind = TEXT("Turn");
        Item.Timestamp = FromUtf8(Turn.CreatedAt);
        Item.Text = FromUtf8(Turn.Content);
        Item.ProviderId = FromUtf8(Turn.ProviderId);
        ClassifySource(Turn.Role, true, Item);
        OutItems.Add(MoveTemp(Item));
    }
    for (const WanaMemory::SalientMemory& Memory : Memories)
    {
        FWanaMemoryTimelineItem Item;
        Item.Kind = TEXT("Memory");
        Item.Timestamp = FromUtf8(Memory.CreatedAt);
        Item.Text = FromUtf8(Memory.Content);
        Item.bHasImportance = true;
        Item.Importance = static_cast<float>(Memory.Importance);
        ClassifySource(Memory.Source, false, Item);
        OutItems.Add(MoveTemp(Item));
    }
    OutItems.Sort([](const FWanaMemoryTimelineItem& Left, const FWanaMemoryTimelineItem& Right)
    {
        if (Left.Timestamp != Right.Timestamp)
        {
            return Left.Timestamp < Right.Timestamp;
        }
        return Left.Kind < Right.Kind;
    });
    OutError.Reset();
    return true;
}

bool FWanaMemoryDatabaseView::ReadEdges(TArray<FWanaMemoryEdge>& OutEdges, FString& OutError)
{
    OutEdges.Reset();
    if (!IsOpen())
    {
        OutError = TEXT("The memory database is not open.");
        return false;
    }
    std::vector<WanaMemory::RelationshipRecord> Records;
    const WanaMemory::Status Status = Impl->Store.ListRelationships(Records);
    if (!Status.bOk)
    {
        Fail(Status, OutError);
        return false;
    }
    for (const WanaMemory::RelationshipRecord& Record : Records)
    {
        FWanaMemoryEdge Edge;
        Edge.CharacterId = FromUtf8(Record.CharacterId);
        Edge.PlayerId = FromUtf8(Record.PlayerId);
        Edge.Trust = static_cast<float>(Record.Scores.Trust);
        Edge.Affinity = static_cast<float>(Record.Scores.Affinity);
        Edge.Fear = static_cast<float>(Record.Scores.Fear);
        Edge.Respect = static_cast<float>(Record.Scores.Respect);
        Edge.UpdatedAt = FromUtf8(Record.UpdatedAt);
        OutEdges.Add(MoveTemp(Edge));
    }
    OutError.Reset();
    return true;
}

bool FWanaMemoryDatabaseView::ReadHistory(const FString& CharacterId, const FString& PlayerId, TArray<FWanaMemoryHistoryPoint>& OutPoints, FString& OutError)
{
    OutPoints.Reset();
    if (!IsOpen())
    {
        OutError = TEXT("The memory database is not open.");
        return false;
    }
    std::vector<WanaMemory::RelationshipEvent> Events;
    const WanaMemory::Status Status = Impl->Store.GetRelationshipHistory(ToUtf8(CharacterId), ToUtf8(PlayerId), 40, Events);
    if (!Status.bOk)
    {
        Fail(Status, OutError);
        return false;
    }
    for (const WanaMemory::RelationshipEvent& Event : Events)
    {
        FWanaMemoryHistoryPoint Point;
        Point.Timestamp = FromUtf8(Event.CreatedAt);
        Point.Trust = static_cast<float>(Event.Scores.Trust);
        Point.Affinity = static_cast<float>(Event.Scores.Affinity);
        Point.Fear = static_cast<float>(Event.Scores.Fear);
        Point.Respect = static_cast<float>(Event.Scores.Respect);
        Point.Reason = FromUtf8(Event.Delta.Reason);
        Point.Source = FromUtf8(Event.Delta.Source);
        OutPoints.Add(MoveTemp(Point));
    }
    OutError.Reset();
    return true;
}

bool FWanaMemoryDatabaseView::ReadIdentities(const FString& CharacterId, TArray<FWanaMemoryIdentityItem>& OutStates, FString& OutError)
{
    OutStates.Reset();
    if (!IsOpen())
    {
        OutError = TEXT("The memory database is not open.");
        return false;
    }
    std::vector<WanaMemory::IdentityState> States;
    const WanaMemory::Status Status = Impl->Store.ListIdentity(ToUtf8(CharacterId), States);
    if (!Status.bOk)
    {
        Fail(Status, OutError);
        return false;
    }
    for (const WanaMemory::IdentityState& State : States)
    {
        FWanaMemoryIdentityItem Item;
        Item.StateKind = FromUtf8(State.StateKind);
        Item.SchemaVersion = State.SchemaVersion;
        Item.JsonBlob = FromUtf8(State.JsonBlob);
        Item.UpdatedAt = FromUtf8(State.UpdatedAt);
        OutStates.Add(MoveTemp(Item));
    }
    OutError.Reset();
    return true;
}
