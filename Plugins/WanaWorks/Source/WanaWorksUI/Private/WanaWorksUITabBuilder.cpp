#include "WanaWorksUITabBuilder.h"

#include "Animation/AnimInstance.h"
#include "AssetThumbnail.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EditorViewportClient.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "PreviewScene.h"
#include "SEditorViewport.h"
#include "Styling/CoreStyle.h"
#include "WanaWorksCommandRegistry.h"
#include "WanaWorksUIStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WanaWorksUITabBuilder"

namespace
{
TSharedRef<SWidget> MakeSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const TSharedRef<SWidget>& Content,
    const TOptional<FText>& Description = TOptional<FText>(),
    const TOptional<FText>& StatusText = TOptional<FText>(),
    bool bProminent = false);
TSharedRef<SWidget> MakeCommandButton(const FWanaWorksUITabBuilderArgs& Args, const FWanaCommandDefinition& Definition);
TSharedRef<SWidget> MakeSandboxPreviewSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont);
FLinearColor GetWorkspaceAccentColor(const FString& WorkspaceLabel);
FText GetWorkspaceHeroBadge(const FString& WorkspaceLabel);
constexpr float WanaAISubsectionSpacing = 10.0f;
const FLinearColor SecondaryTextColor(0.735f, 0.805f, 0.930f, 1.0f);
const FLinearColor TertiaryTextColor(0.455f, 0.535f, 0.700f, 1.0f);
const FLinearColor InfoPanelColor(0.018f, 0.028f, 0.066f, 0.94f);
const FLinearColor PreviewPanelColor(0.002f, 0.006f, 0.020f, 0.996f);
const FLinearColor StudioPanelColor(0.010f, 0.016f, 0.040f, 0.982f);
const FLinearColor StudioPanelRaisedColor(0.022f, 0.032f, 0.074f, 0.992f);
const FLinearColor StudioPanelElevatedColor(0.048f, 0.064f, 0.132f, 1.0f);
const FLinearColor StudioOutlineColor(0.150f, 0.190f, 0.360f, 0.36f);
const FLinearColor StudioDividerColor(0.070f, 0.095f, 0.190f, 0.52f);
const FLinearColor StudioAccentColor(0.54f, 0.42f, 1.00f, 1.0f);
const FLinearColor StudioAccentBlueColor(0.20f, 0.62f, 1.00f, 1.0f);
const FLinearColor StudioAccentGoldColor(0.92f, 0.72f, 0.36f, 1.0f);
const FLinearColor StudioSuccessColor(0.36f, 0.82f, 0.64f, 1.0f);
const FLinearColor StudioShellColor(0.0008f, 0.0020f, 0.0100f, 1.0f);
const FLinearColor StudioShellRaisedColor(0.008f, 0.014f, 0.036f, 0.998f);
const FLinearColor StudioShellTrackColor(0.016f, 0.026f, 0.066f, 0.985f);
const FLinearColor StudioStageBackdropColor(0.001f, 0.004f, 0.018f, 1.0f);
const FLinearColor StudioMutedButtonColor(0.032f, 0.046f, 0.104f, 1.0f);
const FLinearColor StudioShadowColor(0.0f, 0.0f, 0.0f, 0.72f);
const FLinearColor StudioSoftSurfaceColor(0.032f, 0.046f, 0.108f, 0.74f);
const FLinearColor StudioGlassSurfaceColor(0.072f, 0.094f, 0.184f, 0.50f);
const FLinearColor StudioStageChromeColor(0.001f, 0.004f, 0.016f, 0.996f);
const FLinearColor StudioStageWashColor(0.016f, 0.034f, 0.098f, 0.94f);

FSlateFontInfo MakeStudioFont(const ANSICHAR* Typeface, int32 Size)
{
    return WanaWorksUIStyle::WanaFont(Typeface, Size);
}

bool ShouldRenderCompactValueChip(const FString& Value)
{
    FString NormalizedValue = Value.TrimStartAndEnd();
    TArray<FString> Tokens;
    NormalizedValue.ParseIntoArrayWS(Tokens);

    return !NormalizedValue.IsEmpty()
        && NormalizedValue.Len() <= 20
        && Tokens.Num() <= 3
        && !NormalizedValue.Contains(TEXT("/"))
        && !NormalizedValue.Contains(TEXT("\\"))
        && !NormalizedValue.Contains(TEXT("."));
}

bool IsLiveWorkspaceLabel(const FString& WorkspaceLabel)
{
    return WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase);
}

TSharedRef<SWidget> MakeStudioDivider(float Height = 1.0f, const FLinearColor& DividerColor = StudioDividerColor)
{
    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(DividerColor)
        [
            SNew(SBox)
            .HeightOverride(Height)
        ];
}

TSharedRef<SWidget> MakeStudioPill(
    const FText& Label,
    const FLinearColor& FillColor,
    const FLinearColor& TextColor = FLinearColor::White,
    int32 FontSize = 8,
    const FMargin& Padding = FMargin(10.0f, 4.0f))
{
    (void)TextColor;
    return WanaWorksUIStyle::WanaStatusPill(Label, FillColor, false, FontSize, Padding);
}

TSharedRef<SWidget> MakeDynamicWorkspacePill(TFunction<FString(void)> GetSelectedWorkspaceLabel)
{
    return SNew(SBorder)
        .Padding(FMargin(12.0f, 6.0f))
        .BorderBackgroundColor_Lambda([GetSelectedWorkspaceLabel]()
        {
            const FString WorkspaceLabel = GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI"));
            return GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.16f);
        })
        [
            SNew(STextBlock)
            .ColorAndOpacity(FLinearColor(0.95f, 0.97f, 1.0f, 1.0f))
            .Font(MakeStudioFont("Bold", 8))
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text_Lambda([GetSelectedWorkspaceLabel]()
            {
                const FString WorkspaceLabel = GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI"));
                return GetWorkspaceHeroBadge(WorkspaceLabel);
            })
        ];
}

struct FStudioSummaryRow
{
    FString Label;
    FString Value;
    bool bIsNote = false;
};

TArray<FStudioSummaryRow> ParseStudioSummaryRows(const FText& SummaryText, int32 MaxRows = MAX_int32)
{
    TArray<FStudioSummaryRow> ParsedRows;
    TArray<FString> RawLines;
    SummaryText.ToString().ParseIntoArrayLines(RawLines, true);

    for (const FString& RawLine : RawLines)
    {
        const FString TrimmedLine = RawLine.TrimStartAndEnd();

        if (TrimmedLine.IsEmpty())
        {
            continue;
        }

        FStudioSummaryRow& Row = ParsedRows.AddDefaulted_GetRef();
        const int32 ColonIndex = TrimmedLine.Find(TEXT(":"));

        if (ColonIndex > 0)
        {
            Row.Label = TrimmedLine.Left(ColonIndex).TrimStartAndEnd();
            Row.Value = TrimmedLine.Mid(ColonIndex + 1).TrimStartAndEnd();
        }
        else
        {
            Row.Value = TrimmedLine;
            Row.bIsNote = true;
        }

        if (ParsedRows.Num() >= MaxRows)
        {
            break;
        }
    }

    return ParsedRows;
}

TSharedRef<SWidget> BuildStudioSummaryRowsWidget(
    const FText& SummaryText,
    int32 MaxRows,
    float ValueWrapWidth = 220.0f,
    const FLinearColor& AccentColor = StudioAccentColor)
{
    const TArray<FStudioSummaryRow> Rows = ParseStudioSummaryRows(SummaryText, MaxRows);
    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    if (Rows.IsEmpty())
    {
        Layout->AddSlot()
        .AutoHeight()
        [
            SNew(SBorder)
            .Padding(FMargin(14.0f, 12.0f))
            .BorderBackgroundColor(StudioGlassSurfaceColor.CopyWithNewOpacity(0.32f))
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Regular", 9))
                .ColorAndOpacity(SecondaryTextColor)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(LOCTEXT("WanaWorksStudioSummaryEmpty", "No live details yet."))
            ]
        ];

        return Layout;
    }

    for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
    {
        const FStudioSummaryRow& Row = Rows[RowIndex];
        const bool bLeadRow = RowIndex == 0;

        if (Row.bIsNote || Row.Label.IsEmpty())
        {
            Layout->AddSlot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, RowIndex + 1 < Rows.Num() ? 10.0f : 0.0f)
            [
                WanaWorksUIStyle::WanaMetricBlock(
                    FText::GetEmpty(),
                    FText::FromString(Row.Value),
                    AccentColor,
                    bLeadRow,
                    ValueWrapWidth)
            ];

            continue;
        }

        Layout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, RowIndex + 1 < Rows.Num() ? 10.0f : 0.0f)
        [
            WanaWorksUIStyle::WanaMetricBlock(
                FText::FromString(Row.Label),
                FText::FromString(Row.Value),
                AccentColor,
                bLeadRow,
                ValueWrapWidth)
        ];
    }

    return Layout;
}

class SWanaWorksStudioSummaryCard : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksStudioSummaryCard)
        : _AccentColor(StudioAccentColor)
        , _MaxRows(6)
        , _ValueWrapWidth(220.0f)
    {
    }
        SLATE_ARGUMENT(FText, Title)
        SLATE_ARGUMENT(FText, Eyebrow)
        SLATE_ARGUMENT(FLinearColor, AccentColor)
        SLATE_ARGUMENT(int32, MaxRows)
        SLATE_ARGUMENT(float, ValueWrapWidth)
        SLATE_ARGUMENT(TFunction<FText(void)>, GetSummaryText)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Title = InArgs._Title;
        Eyebrow = InArgs._Eyebrow;
        AccentColor = InArgs._AccentColor;
        MaxRows = InArgs._MaxRows;
        ValueWrapWidth = InArgs._ValueWrapWidth;
        GetSummaryText = InArgs._GetSummaryText;

        const TSharedRef<SBox> BodyContent = SAssignNew(BodyBox, SBox);
        const TSharedRef<SWidget> LiveBadge = WanaWorksUIStyle::WanaStatusPill(
            LOCTEXT("WanaWorksStudioModuleBadge", "LIVE"),
            AccentColor,
            true,
            8,
            FMargin(10.0f, 5.0f));

        ChildSlot
        [
            WanaWorksUIStyle::WanaCard(
                Eyebrow,
                Title,
                AccentColor,
                BodyContent,
                LiveBadge,
                false)
        ];

        RefreshBody();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        const FString CurrentSummary = GetSummaryText ? GetSummaryText().ToString() : FString();

        if (CurrentSummary != CachedSummary)
        {
            RefreshBody();
        }
    }

private:
    void RefreshBody()
    {
        CachedSummary = GetSummaryText ? GetSummaryText().ToString() : FString();

        if (!BodyBox.IsValid())
        {
            return;
        }

        BodyBox->SetContent(BuildStudioSummaryRowsWidget(FText::FromString(CachedSummary), MaxRows, ValueWrapWidth, AccentColor));
    }

    FText Title;
    FText Eyebrow;
    FLinearColor AccentColor = StudioAccentColor;
    int32 MaxRows = 6;
    float ValueWrapWidth = 220.0f;
    TFunction<FText(void)> GetSummaryText;
    TSharedPtr<SBox> BodyBox;
    FString CachedSummary;
};

struct FWanaWorksResolvedPreviewSceneData
{
    USkeletalMesh* SkeletalMesh = nullptr;
    UStaticMesh* StaticMesh = nullptr;
    TSubclassOf<UAnimInstance> AnimClass = nullptr;
    FTransform ComponentTransform = FTransform::Identity;
    FBoxSphereBounds PreviewBounds = FBoxSphereBounds(FVector::ZeroVector, FVector(88.0f), 88.0f);
    FString DisplayLabel;
};

bool TryResolvePreviewSceneDataFromActor(const AActor* Actor, FWanaWorksResolvedPreviewSceneData& OutData)
{
    if (!Actor)
    {
        return false;
    }

    AActor* MutableActor = const_cast<AActor*>(Actor);
    TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    MutableActor->GetComponents(SkeletalMeshComponents);

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetSkeletalMeshAsset())
        {
            continue;
        }

        OutData.SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
        OutData.AnimClass = SkeletalMeshComponent->GetAnimClass();
        OutData.ComponentTransform = FTransform(
            SkeletalMeshComponent->GetRelativeRotation(),
            FVector::ZeroVector,
            SkeletalMeshComponent->GetRelativeScale3D());
        OutData.PreviewBounds = OutData.SkeletalMesh->GetBounds().TransformBy(OutData.ComponentTransform.ToMatrixWithScale());
        OutData.DisplayLabel = Actor->GetActorNameOrLabel();
        return true;
    }

    TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents;
    MutableActor->GetComponents(StaticMeshComponents);

    for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
    {
        if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
        {
            continue;
        }

        OutData.StaticMesh = StaticMeshComponent->GetStaticMesh();
        OutData.ComponentTransform = FTransform(
            StaticMeshComponent->GetRelativeRotation(),
            FVector::ZeroVector,
            StaticMeshComponent->GetRelativeScale3D());
        OutData.PreviewBounds = OutData.StaticMesh->GetBounds().TransformBy(OutData.ComponentTransform.ToMatrixWithScale());
        OutData.DisplayLabel = Actor->GetActorNameOrLabel();
        return true;
    }

    return false;
}

bool TryResolvePreviewSceneDataFromObject(UObject* PreviewObject, FWanaWorksResolvedPreviewSceneData& OutData)
{
    if (!PreviewObject)
    {
        return false;
    }

    if (AActor* PreviewActor = Cast<AActor>(PreviewObject))
    {
        return TryResolvePreviewSceneDataFromActor(PreviewActor, OutData);
    }

    if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(PreviewObject))
    {
        if (UClass* GeneratedClass = BlueprintAsset->GeneratedClass.Get())
        {
            if (TryResolvePreviewSceneDataFromActor(Cast<AActor>(GeneratedClass->GetDefaultObject()), OutData))
            {
                if (OutData.DisplayLabel.IsEmpty())
                {
                    OutData.DisplayLabel = BlueprintAsset->GetName();
                }

                return true;
            }
        }
    }

    if (UClass* PreviewClass = Cast<UClass>(PreviewObject))
    {
        if (AActor* DefaultActor = Cast<AActor>(PreviewClass->GetDefaultObject()))
        {
            if (TryResolvePreviewSceneDataFromActor(DefaultActor, OutData))
            {
                if (OutData.DisplayLabel.IsEmpty())
                {
                    OutData.DisplayLabel = PreviewClass->GetName();
                }

                return true;
            }
        }
    }

    if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(PreviewObject))
    {
        OutData.SkeletalMesh = SkeletalMesh;
        OutData.PreviewBounds = SkeletalMesh->GetBounds();
        OutData.DisplayLabel = SkeletalMesh->GetName();
        return true;
    }

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(PreviewObject))
    {
        OutData.StaticMesh = StaticMesh;
        OutData.PreviewBounds = StaticMesh->GetBounds();
        OutData.DisplayLabel = StaticMesh->GetName();
        return true;
    }

    return false;
}

FString GetPreviewObjectDisplayLabel(const UObject* PreviewObject)
{
    if (const AActor* PreviewActor = Cast<AActor>(PreviewObject))
    {
        return PreviewActor->GetActorNameOrLabel();
    }

    return PreviewObject ? PreviewObject->GetName() : FString(TEXT("Subject"));
}

FText GetWorkspacePreviewTitle(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioCharacterPreviewCardTitle", "Character Build Preview");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioLevelPreviewCardTitle", "Semantic World Preview");
    }

    return LOCTEXT("WanaWorksStudioAIPreviewCardTitle", "Character Intelligence Preview");
}

FText GetWorkspacePreviewEyebrow(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioCharacterPreviewCardEyebrow", "CHARACTER AUTHORING STAGE");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioLevelPreviewCardEyebrow", "SEMANTIC WORLD STAGE");
    }

    return LOCTEXT("WanaWorksStudioAIPreviewCardEyebrow", "CHARACTER INTELLIGENCE STAGE");
}

FText GetWorkspacePreviewStandbyTitle(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioCharacterPreviewStandbyTitle", "Character Build Preview Ready");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioLevelPreviewStandbyTitle", "Semantic World Stage Ready");
    }

    return LOCTEXT("WanaWorksStudioAIPreviewStandbyTitle", "Character Intelligence Stage Ready");
}

FText GetWorkspacePreviewStandbyNote(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioCharacterPreviewStandbyNote", "Choose a Character Blueprint or Pawn to wake the build preview with mesh, skeleton, rig, animation, movement, and output context.");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioLevelPreviewStandbyNote", "Scan environment context or select modular scene assets to begin shaping cover, obstacle, boundary, and movement-space meaning.");
    }

    return LOCTEXT("WanaWorksStudioAIPreviewStandbyNote", "Choose an AI Pawn or working subject to wake the intelligence stage with a live character preview and current stack context.");
}

FText GetWorkspacePreviewActiveLabel(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioCharacterPreviewActiveLabel", "CHARACTER SUBJECT");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioLevelPreviewActiveLabel", "SCENE ANCHOR");
    }

    return LOCTEXT("WanaWorksStudioAIPreviewActiveLabel", "AI SUBJECT");
}

class SWanaWorksLivePreviewViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksLivePreviewViewport)
    {
    }
        SLATE_ARGUMENT(UObject*, PreviewObject)
        SLATE_ARGUMENT(TFunction<FString(void)>, GetSelectedPreviewViewLabel)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        PreviewObject = InArgs._PreviewObject;
        GetSelectedPreviewViewLabel = InArgs._GetSelectedPreviewViewLabel;
        PreviewScene = MakeUnique<FPreviewScene>(
            FPreviewScene::ConstructionValues()
            .SetLightBrightness(9.0f)
            .SetSkyBrightness(3.6f)
            .SetLightRotation(FRotator(-34.0f, -38.0f, 0.0f)));
        PreviewScene->SetLightColor(FColor(252, 248, 238));
        ConfigureStudioLighting();

        ResolvePreviewContent();
        SEditorViewport::Construct(SEditorViewport::FArguments());
        ApplyPreviewView();
    }

    virtual ~SWanaWorksLivePreviewViewport() override
    {
        PreviewViewportClient.Reset();
        PreviewSkeletalMeshComponent = nullptr;
        PreviewStaticMeshComponent = nullptr;
        KeyLightComponent = nullptr;
        FillLightComponent = nullptr;
        RimLightComponent = nullptr;
        PreviewObject.Reset();
        PreviewScene.Reset();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        if (PreviewScene && PreviewScene->GetWorld())
        {
            PreviewScene->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
        }

        const FString CurrentViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString(TEXT("Overview"));

        if (CurrentViewLabel != CachedPreviewViewLabel)
        {
            CachedPreviewViewLabel = CurrentViewLabel;
            ApplyPreviewView();
        }
    }

private:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
    {
        PreviewViewportClient = MakeShared<FEditorViewportClient>(nullptr, PreviewScene.Get(), SharedThis(this));
        PreviewViewportClient->SetViewMode(VMI_Lit);
        PreviewViewportClient->SetRealtime(true);
        PreviewViewportClient->bUsingOrbitCamera = true;
        PreviewViewportClient->bSetListenerPosition = false;
        PreviewViewportClient->SetViewRotation(FRotator(-10.0f, -135.0f, 0.0f));
        PreviewViewportClient->EngineShowFlags.SetSelection(false);
        PreviewViewportClient->EngineShowFlags.SetGrid(false);
        PreviewViewportClient->EngineShowFlags.SetPivot(false);
        PreviewViewportClient->EngineShowFlags.SetModeWidgets(false);
        PreviewViewportClient->EngineShowFlags.SetSelectionOutline(false);
        PreviewViewportClient->EngineShowFlags.SetAmbientOcclusion(true);
        return PreviewViewportClient.ToSharedRef();
    }

    virtual TSharedPtr<SWidget> MakeViewportToolbar() override
    {
        return SNullWidget::NullWidget;
    }

    void ResolvePreviewContent()
    {
        ReleasePreviewComponents();
        ResolvedPreviewData = FWanaWorksResolvedPreviewSceneData();

        if (!TryResolvePreviewSceneDataFromObject(PreviewObject.Get(), ResolvedPreviewData))
        {
            return;
        }

        if (ResolvedPreviewData.SkeletalMesh)
        {
            PreviewSkeletalMeshComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
            PreviewSkeletalMeshComponent->SetSkeletalMeshAsset(ResolvedPreviewData.SkeletalMesh);
            PreviewSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PreviewSkeletalMeshComponent->SetUpdateAnimationInEditor(true);

            if (ResolvedPreviewData.AnimClass)
            {
                PreviewSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
                PreviewSkeletalMeshComponent->SetAnimInstanceClass(ResolvedPreviewData.AnimClass.Get());
            }

            PreviewScene->AddComponent(PreviewSkeletalMeshComponent, ResolvedPreviewData.ComponentTransform);
            return;
        }

        if (ResolvedPreviewData.StaticMesh)
        {
            PreviewStaticMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
            PreviewStaticMeshComponent->SetStaticMesh(ResolvedPreviewData.StaticMesh);
            PreviewStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PreviewScene->AddComponent(PreviewStaticMeshComponent, ResolvedPreviewData.ComponentTransform);
        }
    }

    UDirectionalLightComponent* AddStudioDirectionalLight(
        const FName ComponentName,
        const float Intensity,
        const FLinearColor& LightColor,
        const FRotator& Rotation)
    {
        if (!PreviewScene)
        {
            return nullptr;
        }

        UDirectionalLightComponent* LightComponent = NewObject<UDirectionalLightComponent>(GetTransientPackage(), ComponentName, RF_Transient);
        if (!LightComponent)
        {
            return nullptr;
        }

        LightComponent->SetIntensity(Intensity);
        LightComponent->SetLightColor(LightColor);
        PreviewScene->AddComponent(LightComponent, FTransform(Rotation));
        return LightComponent;
    }

    void ConfigureStudioLighting()
    {
        KeyLightComponent = AddStudioDirectionalLight(
            TEXT("WanaWorksPreviewKeyLight"),
            8.6f,
            FLinearColor(1.00f, 0.965f, 0.90f, 1.0f),
            FRotator(-30.0f, -35.0f, 0.0f));

        FillLightComponent = AddStudioDirectionalLight(
            TEXT("WanaWorksPreviewFillLight"),
            4.2f,
            FLinearColor(0.58f, 0.70f, 1.00f, 1.0f),
            FRotator(-10.0f, 122.0f, 0.0f));

        RimLightComponent = AddStudioDirectionalLight(
            TEXT("WanaWorksPreviewRimLight"),
            8.8f,
            FLinearColor(0.66f, 0.82f, 1.00f, 1.0f),
            FRotator(-16.0f, 145.0f, 0.0f));
    }

    void ReleasePreviewLighting()
    {
        auto ReleaseLightComponent = [this](UDirectionalLightComponent*& LightComponent)
        {
            UDirectionalLightComponent* ComponentToRelease = LightComponent;
            LightComponent = nullptr;

            if (!PreviewScene || !ComponentToRelease || !IsValid(ComponentToRelease))
            {
                return;
            }

            PreviewScene->RemoveComponent(ComponentToRelease);
            ComponentToRelease->DestroyComponent();
        };

        ReleaseLightComponent(KeyLightComponent);
        ReleaseLightComponent(FillLightComponent);
        ReleaseLightComponent(RimLightComponent);
    }

    void ReleasePreviewComponents()
    {
        USkeletalMeshComponent* SkeletalMeshComponentToRelease = PreviewSkeletalMeshComponent;
        PreviewSkeletalMeshComponent = nullptr;

        if (PreviewScene && SkeletalMeshComponentToRelease && IsValid(SkeletalMeshComponentToRelease))
        {
            PreviewScene->RemoveComponent(SkeletalMeshComponentToRelease);
            SkeletalMeshComponentToRelease->DestroyComponent();
        }

        UStaticMeshComponent* StaticMeshComponentToRelease = PreviewStaticMeshComponent;
        PreviewStaticMeshComponent = nullptr;

        if (PreviewScene && StaticMeshComponentToRelease && IsValid(StaticMeshComponentToRelease))
        {
            PreviewScene->RemoveComponent(StaticMeshComponentToRelease);
            StaticMeshComponentToRelease->DestroyComponent();
        }
    }

    void ApplyPreviewView()
    {
        if (!PreviewViewportClient.IsValid())
        {
            return;
        }

        const FBoxSphereBounds Bounds = ResolvedPreviewData.PreviewBounds.SphereRadius > KINDA_SMALL_NUMBER
            ? ResolvedPreviewData.PreviewBounds
            : FBoxSphereBounds(FVector::ZeroVector, FVector(88.0f), 88.0f);
        const float Radius = FMath::Max(Bounds.SphereRadius, 90.0f);
        const FVector LookAt = Bounds.Origin + FVector(0.0f, 0.0f, Radius * 0.08f);
        const FString ViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString(TEXT("Overview"));

        FRotator ViewRotation(-8.0f, -135.0f, 0.0f);
        float DistanceScale = 2.36f;

        if (ViewLabel.Equals(TEXT("Front"), ESearchCase::IgnoreCase))
        {
            ViewRotation = FRotator(-4.0f, 0.0f, 0.0f);
            DistanceScale = 2.00f;
        }
        else if (ViewLabel.Equals(TEXT("Back"), ESearchCase::IgnoreCase))
        {
            ViewRotation = FRotator(-4.0f, 180.0f, 0.0f);
            DistanceScale = 2.00f;
        }
        else if (ViewLabel.Equals(TEXT("Left"), ESearchCase::IgnoreCase))
        {
            ViewRotation = FRotator(-4.0f, 90.0f, 0.0f);
            DistanceScale = 1.96f;
        }
        else if (ViewLabel.Equals(TEXT("Right"), ESearchCase::IgnoreCase))
        {
            ViewRotation = FRotator(-4.0f, 270.0f, 0.0f);
            DistanceScale = 1.96f;
        }

        const float CameraDistance = Radius * DistanceScale;
        const FVector CameraLocation = LookAt - ViewRotation.Vector() * CameraDistance;

        PreviewViewportClient->SetLookAtLocation(LookAt);
        PreviewViewportClient->SetViewRotation(ViewRotation);
        PreviewViewportClient->SetViewLocation(CameraLocation);
        PreviewViewportClient->Invalidate();
        Invalidate();
    }

    TWeakObjectPtr<UObject> PreviewObject;
    TFunction<FString(void)> GetSelectedPreviewViewLabel;
    TUniquePtr<FPreviewScene> PreviewScene;
    TSharedPtr<FEditorViewportClient> PreviewViewportClient;
    FWanaWorksResolvedPreviewSceneData ResolvedPreviewData;
    USkeletalMeshComponent* PreviewSkeletalMeshComponent = nullptr;
    UStaticMeshComponent* PreviewStaticMeshComponent = nullptr;
    UDirectionalLightComponent* KeyLightComponent = nullptr;
    UDirectionalLightComponent* FillLightComponent = nullptr;
    UDirectionalLightComponent* RimLightComponent = nullptr;
    FString CachedPreviewViewLabel;
};

class SWanaWorksSandboxPreviewCard : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksSandboxPreviewCard)
    {
    }
        SLATE_ARGUMENT(TFunction<UObject*(void)>, GetPreviewObject)
        SLATE_ARGUMENT(TFunction<FText(void)>, GetPreviewSummaryText)
        SLATE_ARGUMENT(TFunction<FString(void)>, GetSelectedPreviewViewLabel)
        SLATE_ARGUMENT(TFunction<FString(void)>, GetSelectedWorkspaceLabel)
        SLATE_ARGUMENT(TFunction<void(void)>, OnFocusPreviewSubject)
        SLATE_ARGUMENT(TFunction<void(const FString&)>, OnSelectPreviewView)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        GetPreviewObject = InArgs._GetPreviewObject;
        GetPreviewSummaryText = InArgs._GetPreviewSummaryText;
        GetSelectedPreviewViewLabel = InArgs._GetSelectedPreviewViewLabel;
        GetSelectedWorkspaceLabel = InArgs._GetSelectedWorkspaceLabel;
        OnFocusPreviewSubject = InArgs._OnFocusPreviewSubject;
        OnSelectPreviewView = InArgs._OnSelectPreviewView;
        ThumbnailPool = MakeShared<FAssetThumbnailPool>(16, false);
        const FString WorkspaceLabel = GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI"));
        const FLinearColor WorkspaceAccentColor = GetWorkspaceAccentColor(WorkspaceLabel);

        ChildSlot
        [
            SNew(SBorder)
            .Padding(0.0f)
            .BorderBackgroundColor(WorkspaceAccentColor.CopyWithNewOpacity(0.22f))
            [
                SNew(SBorder)
                .Padding(22.0f)
                .BorderBackgroundColor(PreviewPanelColor.CopyWithNewOpacity(0.998f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(TertiaryTextColor)
                                .Font(MakeStudioFont("Bold", 8))
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(GetWorkspacePreviewEyebrow(WorkspaceLabel))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Font(MakeStudioFont("Bold", 22))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text_Lambda([this]()
                                {
                                    return GetWorkspacePreviewTitle(GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI")));
                                })
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(12.0f, 0.0f, 12.0f, 0.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(12.0f, 6.0f))
                            .BorderBackgroundColor(WorkspaceAccentColor.CopyWithNewOpacity(0.24f))
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(FLinearColor(0.95f, 0.92f, 1.0f, 1.0f))
                                .Font(MakeStudioFont("Bold", 8))
                                .Text_Lambda([this]()
                                {
                                    return FText::FromString(GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString(TEXT("Overview")));
                                })
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SButton)
                            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                            .ContentPadding(FMargin(0.0f))
                            .OnClicked_Lambda([this]()
                            {
                                if (OnFocusPreviewSubject)
                                {
                                    OnFocusPreviewSubject();
                                }

                                return FReply::Handled();
                            })
                            [
                            MakeStudioPill(
                                LOCTEXT("WanaWorksSandboxPreviewFocusButton", "Focus Subject"),
                                WorkspaceAccentColor.CopyWithNewOpacity(0.15f),
                                FLinearColor(0.80f, 0.89f, 1.0f, 1.0f),
                                8,
                                FMargin(14.0f, 7.0f))
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SNew(SWrapBox)
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 8.0f))
                        [
                            MakeStageViewButton(TEXT("Overview"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 8.0f))
                        [
                            MakeStageViewButton(TEXT("Front"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 8.0f))
                        [
                            MakeStageViewButton(TEXT("Back"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 8.0f))
                        [
                            MakeStageViewButton(TEXT("Left"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 8.0f))
                        [
                            MakeStageViewButton(TEXT("Right"))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(PreviewContentBox, SBox)
                        .MinDesiredHeight(620.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 14.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(SummaryBox, SBox)
                    ]
                ]
            ]
        ];

        RefreshPreviewContent();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        UObject* PreviewObject = GetPreviewObject ? GetPreviewObject() : nullptr;

        const FString CurrentSummary = GetPreviewSummaryText ? GetPreviewSummaryText().ToString() : FString();
        const FString CurrentViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString();

        if (PreviewObject != CachedPreviewObject.Get()
            || CurrentSummary != CachedPreviewSummary
            || CurrentViewLabel != CachedPreviewViewLabel)
        {
            RefreshPreviewContent();
        }
    }

private:
    TSharedRef<SWidget> MakeStageViewButton(const FString& ViewLabel)
    {
        return SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(0.0f))
            .OnClicked_Lambda([this, ViewLabel]()
            {
                if (OnSelectPreviewView)
                {
                    OnSelectPreviewView(ViewLabel);
                }

                return FReply::Handled();
            })
            [
                SNew(SBorder)
                .Padding(FMargin(0.0f))
                .BorderBackgroundColor_Lambda([this, ViewLabel]()
                {
                    const FLinearColor AccentColor = GetWorkspaceAccentColor(GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI")));
                    const bool bIsActive = GetSelectedPreviewViewLabel
                        && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                    return bIsActive
                        ? AccentColor.CopyWithNewOpacity(0.42f)
                        : StudioGlassSurfaceColor.CopyWithNewOpacity(0.34f);
                })
                [
                    SNew(SBox)
                    .MinDesiredWidth(78.0f)
                    [
                        SNew(SBorder)
                        .Padding(FMargin(17.0f, 9.0f))
                        .BorderBackgroundColor_Lambda([this, ViewLabel]()
                        {
                            const FLinearColor AccentColor = GetWorkspaceAccentColor(GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI")));
                            const bool bIsActive = GetSelectedPreviewViewLabel
                                && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                            return bIsActive
                                ? AccentColor.CopyWithNewOpacity(0.18f)
                                : FLinearColor(0.012f, 0.020f, 0.052f, 0.46f);
                        })
                        [
                            SNew(STextBlock)
                            .Justification(ETextJustify::Center)
                            .ColorAndOpacity_Lambda([this, ViewLabel]()
                            {
                                const bool bIsActive = GetSelectedPreviewViewLabel
                                    && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                                return bIsActive
                                    ? FLinearColor(0.98f, 0.95f, 1.0f, 1.0f)
                                    : SecondaryTextColor.CopyWithNewOpacity(0.88f);
                            })
                            .Font(MakeStudioFont("Bold", 9))
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(FText::FromString(ViewLabel))
                        ]
                    ]
                ]
            ];
    }

    void RefreshPreviewContent()
    {
        CachedPreviewObject = GetPreviewObject ? GetPreviewObject() : nullptr;
        CachedPreviewSummary = GetPreviewSummaryText ? GetPreviewSummaryText().ToString() : FString();
        CachedPreviewViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString();

        if (!PreviewContentBox.IsValid())
        {
            return;
        }

        if (UObject* PreviewObject = CachedPreviewObject.Get())
        {
            const FString WorkspaceLabel = GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI"));
            const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);
            const FText ActiveSubjectLabel = GetWorkspacePreviewActiveLabel(WorkspaceLabel);
            FWanaWorksResolvedPreviewSceneData PreviewSceneData;
            const bool bSupportsLivePreview = TryResolvePreviewSceneDataFromObject(PreviewObject, PreviewSceneData);

            TSharedRef<SWidget> PreviewStageWidget = SNullWidget::NullWidget;

            if (bSupportsLivePreview)
            {
                AssetThumbnail.Reset();
                PreviewStageWidget = SNew(SWanaWorksLivePreviewViewport)
                    .PreviewObject(PreviewObject)
                    .GetSelectedPreviewViewLabel(GetSelectedPreviewViewLabel);
            }
            else
            {
                AssetThumbnail = MakeShared<FAssetThumbnail>(PreviewObject, 900, 560, ThumbnailPool);

                FAssetThumbnailConfig ThumbnailConfig;
                ThumbnailConfig.bAllowFadeIn = false;
                PreviewStageWidget = AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig);
            }

            const TSharedRef<SWidget> TopOverlay =
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
                    [
                        WanaWorksUIStyle::WanaStatusPill(
                            LOCTEXT("WanaWorksStudioPreviewLiveBadge", "STAGE LIVE"),
                            AccentColor,
                            true,
                            8,
                            FMargin(12.0f, 6.0f))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        WanaWorksUIStyle::WanaStatusPill(
                            FText::FromString(CachedPreviewViewLabel.IsEmpty() ? TEXT("Overview") : CachedPreviewViewLabel),
                            AccentColor,
                            false,
                            8,
                            FMargin(12.0f, 6.0f))
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(TertiaryTextColor)
                    .Font(MakeStudioFont("Bold", 8))
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(GetWorkspacePreviewEyebrow(WorkspaceLabel))
                ];

            const TSharedRef<SWidget> BottomOverlay =
                SNew(SBorder)
                .Padding(FMargin(20.0f, 15.0f))
                .BorderBackgroundColor(FLinearColor(0.002f, 0.007f, 0.024f, 0.90f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(TertiaryTextColor)
                            .Font(MakeStudioFont("Bold", 8))
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(ActiveSubjectLabel)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(FLinearColor::White)
                            .Font(MakeStudioFont("Bold", 14))
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(FText::FromString(GetPreviewObjectDisplayLabel(PreviewObject)))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Bottom)
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(SecondaryTextColor.CopyWithNewOpacity(0.88f))
                        .AutoWrapText(true)
                        .Font(MakeStudioFont("Regular", 9))
                        .Text(LOCTEXT("WanaWorksStudioPreviewFooterNote", "Use Focus Subject for full world-space inspection."))
                    ]
                ];

            PreviewContentBox->SetContent(
                WanaWorksUIStyle::WanaHeroStage(
                    AccentColor,
                    PreviewStageWidget,
                    TopOverlay,
                    BottomOverlay));
        }
        else
        {
            AssetThumbnail.Reset();
            const FString WorkspaceLabel = GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI"));
            const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);

            const TSharedRef<SWidget> TopOverlay =
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    WanaWorksUIStyle::WanaStatusPill(
                        LOCTEXT("WanaWorksStudioPreviewStandbyBadge", "STAGE READY"),
                        AccentColor,
                        true,
                        8,
                        FMargin(12.0f, 6.0f))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    WanaWorksUIStyle::WanaStatusPill(
                        FText::FromString(CachedPreviewViewLabel.IsEmpty() ? TEXT("Overview") : CachedPreviewViewLabel),
                        AccentColor,
                        false,
                        8,
                        FMargin(12.0f, 6.0f))
                ];

            const TSharedRef<SWidget> EmptyStageContent =
                SNew(SBorder)
                .Padding(46.0f)
                .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.095f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SNew(STextBlock)
                        .Justification(ETextJustify::Center)
                        .ColorAndOpacity(FLinearColor::White)
                        .Font(MakeStudioFont("Bold", 22))
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text(GetWorkspacePreviewStandbyTitle(WorkspaceLabel))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        SNew(SBox)
                        .MaxDesiredWidth(460.0f)
                        [
                            SNew(STextBlock)
                            .Justification(ETextJustify::Center)
                            .AutoWrapText(true)
                            .Font(MakeStudioFont("Regular", 10))
                            .ColorAndOpacity(SecondaryTextColor)
                            .Text(GetWorkspacePreviewStandbyNote(WorkspaceLabel))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 22.0f, 0.0f, 0.0f)
                    .HAlign(HAlign_Center)
                    [
                        SNew(SBox)
                        .WidthOverride(220.0f)
                        [
                            MakeStudioDivider(2.0f, AccentColor.CopyWithNewOpacity(0.70f))
                        ]
                    ]
                ];

            PreviewContentBox->SetContent(
                WanaWorksUIStyle::WanaHeroStage(
                    AccentColor,
                    EmptyStageContent,
                    TopOverlay));
        }

        if (SummaryBox.IsValid())
        {
            const FLinearColor SummaryAccentColor = GetWorkspaceAccentColor(GetSelectedWorkspaceLabel ? GetSelectedWorkspaceLabel() : FString(TEXT("AI")));
            SummaryBox->SetContent(
                SNew(SBorder)
                .Padding(0.0f)
                .BorderBackgroundColor(SummaryAccentColor.CopyWithNewOpacity(0.075f))
                [
                    SNew(SBorder)
                    .Padding(14.0f)
                    .BorderBackgroundColor(StudioSoftSurfaceColor.CopyWithNewOpacity(0.78f))
                    [
                        BuildStudioSummaryRowsWidget(FText::FromString(CachedPreviewSummary), 4, 340.0f, SummaryAccentColor)
                    ]
                ]);
        }
    }

    TFunction<UObject*(void)> GetPreviewObject;
    TFunction<FText(void)> GetPreviewSummaryText;
    TFunction<FString(void)> GetSelectedPreviewViewLabel;
    TFunction<FString(void)> GetSelectedWorkspaceLabel;
    TFunction<void(void)> OnFocusPreviewSubject;
    TFunction<void(const FString&)> OnSelectPreviewView;
    TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
    TSharedPtr<FAssetThumbnail> AssetThumbnail;
    TSharedPtr<SBox> PreviewContentBox;
    TSharedPtr<SBox> SummaryBox;
    TWeakObjectPtr<UObject> CachedPreviewObject;
    FString CachedPreviewSummary;
    FString CachedPreviewViewLabel;
};

FString NormalizeStatusLabel(const FString& StatusLabel)
{
    return StatusLabel.TrimStartAndEnd().ToUpper();
}

FString GetStatusLabelFromMessage(const FText& StatusText)
{
    const FString LowerStatus = StatusText.ToString().TrimStartAndEnd().ToLower();

    if (LowerStatus.Contains(TEXT("warning")) ||
        LowerStatus.Contains(TEXT("no actors")) ||
        LowerStatus.Contains(TEXT("missing")) ||
        LowerStatus.Contains(TEXT("could not")) ||
        LowerStatus.Contains(TEXT("failed")) ||
        LowerStatus.Contains(TEXT("invalid")))
    {
        return TEXT("WARNING");
    }

    if (LowerStatus.Contains(TEXT("already")) ||
        LowerStatus.Contains(TEXT("preserved")) ||
        LowerStatus.Contains(TEXT("safe")))
    {
        return TEXT("SAFE");
    }

    if (LowerStatus.Contains(TEXT("live")) || LowerStatus.Contains(TEXT("evaluat")))
    {
        return TEXT("LIVE");
    }

    if (LowerStatus.Contains(TEXT("ready")) ||
        LowerStatus.Contains(TEXT("applied")) ||
        LowerStatus.Contains(TEXT("ensured")) ||
        LowerStatus.Contains(TEXT("complete")) ||
        LowerStatus.Contains(TEXT("initialized")))
    {
        return TEXT("READY");
    }

    return TEXT("ACTIVE");
}

FLinearColor GetStatusBadgeColor(const FString& StatusLabel)
{
    const FString NormalizedStatus = NormalizeStatusLabel(StatusLabel);

    if (NormalizedStatus == TEXT("READY"))
    {
        return FLinearColor(0.22f, 0.66f, 0.42f, 0.94f);
    }

    if (NormalizedStatus == TEXT("LIVE"))
    {
        return FLinearColor(0.16f, 0.62f, 0.78f, 0.94f);
    }

    if (NormalizedStatus == TEXT("SAFE"))
    {
        return FLinearColor(0.14f, 0.52f, 0.58f, 0.94f);
    }

    if (NormalizedStatus == TEXT("WARNING"))
    {
        return FLinearColor(0.80f, 0.42f, 0.18f, 0.94f);
    }

    return FLinearColor(0.32f, 0.42f, 0.76f, 0.94f);
}

TSharedRef<SWidget> MakeStatusBadge(const FText& StatusText)
{
    const FString NormalizedStatus = NormalizeStatusLabel(StatusText.ToString());

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(GetStatusBadgeColor(NormalizedStatus).CopyWithNewOpacity(0.44f))
        [
            SNew(SBorder)
            .Padding(FMargin(10.0f, 4.0f))
            .BorderBackgroundColor(GetStatusBadgeColor(NormalizedStatus).CopyWithNewOpacity(0.26f))
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 8))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(FText::FromString(NormalizedStatus))
            ]
        ];
}

TSharedRef<SWidget> MakeReadOnlyInfoPanel(const FText& Title, TFunction<FText(void)> GetBodyText)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Font(MakeStudioFont("Bold", 8))
            .ColorAndOpacity(TertiaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Title)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.36f))
            [
                SNew(SBorder)
                .Padding(12.0f)
                .BorderBackgroundColor(InfoPanelColor)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text_Lambda([GetBodyText]()
                    {
                        return GetBodyText ? GetBodyText() : FText::GetEmpty();
                    })
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeFixedWidthButton(const FText& ButtonText, TFunction<void(void)> OnPressed, float WidthOverride = 156.0f)
{
    return SNew(SBox)
        .WidthOverride(WidthOverride)
        .HeightOverride(38.0f)
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
            [
                SNew(SButton)
                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                .ContentPadding(FMargin(0.0f))
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .OnClicked_Lambda([OnPressed]()
                {
                    if (OnPressed)
                    {
                        OnPressed();
                    }

                    return FReply::Handled();
                })
                [
                    SNew(SBorder)
                    .Padding(FMargin(14.0f, 10.0f))
                    .BorderBackgroundColor(StudioMutedButtonColor)
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 9))
                        .ColorAndOpacity(FLinearColor(0.95f, 0.97f, 1.0f, 1.0f))
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text(ButtonText)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStringPickerControl(
    const FText& Label,
    const TArray<TSharedPtr<FString>>* Options,
    TFunction<TSharedPtr<FString>(void)> GetSelectedOption,
    TFunction<void(TSharedPtr<FString>)> OnSelectionChanged,
    const FText& DefaultText,
    float WidthOverride = 320.0f)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            SNew(STextBlock)
            .Font(MakeStudioFont("Bold", 8))
            .ColorAndOpacity(TertiaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Label)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(WidthOverride)
            [
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.44f))
                [
                    SNew(SBorder)
                    .Padding(FMargin(14.0f, 8.0f))
                    .BorderBackgroundColor(StudioMutedButtonColor)
                    [
                        SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(Options)
                        .InitiallySelectedItem(GetSelectedOption ? GetSelectedOption() : nullptr)
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                        {
                            return SNew(STextBlock)
                                .Font(MakeStudioFont("Bold", 9))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                        })
                        .OnSelectionChanged_Lambda([OnSelectionChanged](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                        {
                            if (OnSelectionChanged)
                            {
                                OnSelectionChanged(SelectedItem);
                            }
                        })
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 10))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text_Lambda([GetSelectedOption, DefaultText]()
                            {
                                const TSharedPtr<FString> SelectedOption = GetSelectedOption ? GetSelectedOption() : nullptr;
                                return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : DefaultText;
                            })
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeSandboxPreviewSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksSandboxPreviewSection", "Workspace Preview"),
        SNew(SWanaWorksSandboxPreviewCard)
        .GetPreviewObject(Args.GetSandboxPreviewObject)
        .GetPreviewSummaryText(Args.GetSandboxPreviewSummaryText)
        .GetSelectedPreviewViewLabel(Args.GetSelectedPreviewViewLabel)
        .OnFocusPreviewSubject(Args.OnFocusSandboxPreviewSubject)
        .OnSelectPreviewView(Args.OnPreviewViewSelected),
        LOCTEXT("WanaWorksSandboxPreviewDescription", "A lightweight embedded preview card for the current working subject or finalized output. WanaWorks is the workspace, so this stays focused on the subject you are shaping inside the tool."),
        LOCTEXT("WanaWorksSandboxPreviewStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCharacterIntelligenceControlCard(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioAccentColor.CopyWithNewOpacity(0.28f))
        [
            SNew(SBorder)
            .Padding(14.0f)
            .BorderBackgroundColor(StudioPanelElevatedColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(StudioAccentColor.CopyWithNewOpacity(0.86f))
                            .Text(LOCTEXT("WanaWorksStudioAIControlsEyebrow", "WAI / WAY CONTROLS"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 13))
                            .ColorAndOpacity(FLinearColor::White)
                            .Text(LOCTEXT("WanaWorksStudioAIControlsTitle", "Identity & Relationship"))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Top)
                    [
                        MakeStudioPill(
                            LOCTEXT("WanaWorksStudioAIControlsPill", "LIVE"),
                            StudioSuccessColor.CopyWithNewOpacity(0.18f),
                            StudioSuccessColor,
                            7,
                            FMargin(8.0f, 3.0f))
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .AutoWrapText(true)
                    .Text(LOCTEXT("WanaWorksStudioAIControlsDescription", "Set the AI role, assign a safe relationship target, and choose the WAY state used by Test without exposing old helper-button stacks."))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    MakeStringPickerControl(
                        LOCTEXT("WanaWorksStudioAIIdentityRolePickerLabel", "AI Identity / Role"),
                        Args.CharacterIntelligenceIdentityRoleOptions,
                        Args.GetSelectedCharacterIntelligenceIdentityRoleOption,
                        Args.OnCharacterIntelligenceIdentityRoleOptionSelected,
                        LOCTEXT("WanaWorksStudioAIIdentityRolePickerDefault", "Neutral"),
                        238.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    MakeStringPickerControl(
                        LOCTEXT("WanaWorksStudioAITargetPickerLabel", "Relationship Target"),
                        Args.CharacterIntelligenceTargetOptions,
                        Args.GetSelectedCharacterIntelligenceTargetOption,
                        Args.OnCharacterIntelligenceTargetOptionSelected,
                        LOCTEXT("WanaWorksStudioAITargetPickerDefault", "No Target"),
                        238.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStringPickerControl(
                        LOCTEXT("WanaWorksStudioAIRelationshipPickerLabel", "WAY Relationship"),
                        Args.CharacterIntelligenceRelationshipOptions,
                        Args.GetSelectedCharacterIntelligenceRelationshipOption,
                        Args.OnCharacterIntelligenceRelationshipOptionSelected,
                        LOCTEXT("WanaWorksStudioAIRelationshipPickerDefault", "Unknown"),
                        238.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .Padding(FMargin(12.0f, 10.0f))
                    .BorderBackgroundColor(StudioSoftSurfaceColor)
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Regular", 9))
                        .ColorAndOpacity(SecondaryTextColor)
                        .AutoWrapText(true)
                        .Text_Lambda([GetCharacterIntelligenceControlSummaryText = Args.GetCharacterIntelligenceControlSummaryText]()
                        {
                            return GetCharacterIntelligenceControlSummaryText
                                ? GetCharacterIntelligenceControlSummaryText()
                                : LOCTEXT("WanaWorksStudioAIControlsSummaryFallback", "Choose an identity, target, and relationship state, then run Test to evaluate the recommended behavior.");
                        })
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeCoreWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCoreWorkflowSection", "Core Workflow"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCoreWorkflowGuidanceLabel", "How This Workspace Flows"),
                []()
                {
                    return LOCTEXT("WanaWorksCoreWorkflowGuidanceText", "Choose a subject, inspect it here, create a safe working copy, apply enhancement, test the result, then build a clean final asset. WanaWorks keeps the original source untouched.");
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCreateWorkingCopyButton", "Create Copy"),
                    [OnCreateWorkingCopy = Args.OnCreateWorkingCopy]()
                    {
                        if (OnCreateWorkingCopy)
                        {
                            OnCreateWorkingCopy();
                        }
                    },
                    170.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreApplyEnhancementButton", "Apply Enhancement"),
                    [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                    {
                        if (OnApplyCharacterEnhancement)
                        {
                            OnApplyCharacterEnhancement();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreTestSubjectButton", "Test Subject"),
                    [OnApplyStarterAndTestTarget = Args.OnApplyStarterAndTestTarget]()
                    {
                        if (OnApplyStarterAndTestTarget)
                        {
                            OnApplyStarterAndTestTarget();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreBuildFinalButton", "Build Final"),
                    [OnFinalizeSandboxBuild = Args.OnFinalizeSandboxBuild]()
                    {
                        if (OnFinalizeSandboxBuild)
                        {
                            OnFinalizeSandboxBuild();
                        }
                    },
                    160.0f)
            ]
        ],
        LOCTEXT("WanaWorksCoreWorkflowDescription", "Primary actions only. The rest of the setup, pair testing, presets, and internal tools stay available in the secondary details area."),
        LOCTEXT("WanaWorksCoreWorkflowStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeWorkspaceStatusSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWorkspaceStatusSection", "Live Status"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusSubjectLabel", "Detected Stack"),
                [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
                {
                    return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusAnimationLabel", "Animation Integration"),
                [GetAnimationIntegrationText = Args.GetAnimationIntegrationText]()
                {
                    return GetAnimationIntegrationText ? GetAnimationIntegrationText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusBehaviorLabel", "Behavior Results"),
                [GetBehaviorResultsText = Args.GetBehaviorResultsText]()
                {
                    return GetBehaviorResultsText ? GetBehaviorResultsText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusPhysicalLabel", "Physical State"),
                [GetPhysicalStateText = Args.GetPhysicalStateText]()
                {
                    return GetPhysicalStateText ? GetPhysicalStateText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusEnhancementLabel", "Enhancement Results"),
                [GetEnhancementResultsText = Args.GetEnhancementResultsText]()
                {
                    return GetEnhancementResultsText ? GetEnhancementResultsText() : FText::GetEmpty();
                })
        ],
        LOCTEXT("WanaWorksWorkspaceStatusDescription", "Live status stays visible on the right so the main workspace feels like software, not a stack of utility panels."),
        LOCTEXT("WanaWorksWorkspaceStatusStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCollapsibleWorkspaceDetailsSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& Content)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SBorder)
            .Padding(14.0f)
            .BorderBackgroundColor(FLinearColor(0.045f, 0.06f, 0.10f, 0.92f))
            [
                SNew(SExpandableArea)
                .InitiallyCollapsed(true)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .BodyBorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .HeaderPadding(FMargin(0.0f))
                .HeaderContent()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                        .ColorAndOpacity(TertiaryTextColor)
                        .Text(Title)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(SecondaryTextColor)
                        .Text(Description)
                    ]
                ]
                .BodyContent()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 12.0f, 0.0f, 12.0f)
                    [
                        MakeStudioDivider()
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        Content
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeWorkflowWorkspaceSection(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& ControlsContent)
{
    return MakeSection(
        SectionHeaderFont,
        Title,
        SNew(SSplitter)
        .PhysicalSplitterHandleSize(2.0f)
        + SSplitter::Slot()
        .Value(0.25f)
        [
            ControlsContent
        ]
        + SSplitter::Slot()
        .Value(0.40f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 10.0f)
            [
                MakeSandboxPreviewSection(Args, SectionHeaderFont)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeCoreWorkflowSection(Args, SectionHeaderFont)
            ]
        ]
        + SSplitter::Slot()
        .Value(0.35f)
        [
            MakeWorkspaceStatusSection(Args, SectionHeaderFont)
        ],
        Description,
        LOCTEXT("WanaWorksSandboxWorkspaceStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCharacterEnhancementSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCharacterEnhancementSection", "Enhancement Setup"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementSummaryLabel", "Selected Subject Summary"),
                [GetCharacterEnhancementSummaryText = Args.GetCharacterEnhancementSummaryText]()
                {
                    return GetCharacterEnhancementSummaryText ? GetCharacterEnhancementSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementChainLabel", "Live System Chain"),
                [GetCharacterEnhancementChainText = Args.GetCharacterEnhancementChainText]()
                {
                    return GetCharacterEnhancementChainText ? GetCharacterEnhancementChainText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementWorkflowInfoLabel", "Workflow Guidance"),
                [GetCharacterEnhancementWorkflowText = Args.GetCharacterEnhancementWorkflowText]()
                {
                    return GetCharacterEnhancementWorkflowText ? GetCharacterEnhancementWorkflowText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksCharacterEnhancementWorkflowLabel", "Setup Workflow"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBox)
            .WidthOverride(260.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.EnhancementWorkflowOptions)
                .InitiallySelectedItem(Args.GetSelectedEnhancementWorkflowOption ? Args.GetSelectedEnhancementWorkflowOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnEnhancementWorkflowOptionSelected = Args.OnEnhancementWorkflowOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnEnhancementWorkflowOptionSelected)
                    {
                        OnEnhancementWorkflowOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedEnhancementWorkflowOption = Args.GetSelectedEnhancementWorkflowOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedEnhancementWorkflowOption ? GetSelectedEnhancementWorkflowOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksCharacterEnhancementWorkflowDefault", "Use Original Character");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksCharacterEnhancementPresetLabel", "Starter Preset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(220.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.EnhancementPresetOptions)
                .InitiallySelectedItem(Args.GetSelectedEnhancementPresetOption ? Args.GetSelectedEnhancementPresetOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnEnhancementPresetOptionSelected = Args.OnEnhancementPresetOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnEnhancementPresetOptionSelected)
                    {
                        OnEnhancementPresetOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedEnhancementPresetOption = Args.GetSelectedEnhancementPresetOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedEnhancementPresetOption ? GetSelectedEnhancementPresetOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksCharacterEnhancementDefault", "Identity Only");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            MakeFixedWidthButton(
                LOCTEXT("WanaWorksApplyCharacterEnhancementButton", "Apply WanaAI Enhancement"),
                [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                {
                    if (OnApplyCharacterEnhancement)
                    {
                        OnApplyCharacterEnhancement();
                    }
                },
                220.0f)
        ],
        LOCTEXT("WanaWorksCharacterEnhancementDescription", "Choose how you want to enhance the current subject. Create Working Copy is the safest testing path, and every option adds WanaAI on top of the existing setup instead of replacing it."),
        LOCTEXT("WanaWorksReadyStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeSubjectSetupSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksSubjectSetupSection", "Subject Setup"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksSubjectSetupSummaryLabel", "Selected Subject"),
                [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
                {
                    return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksSubjectStackSummaryLabel", "Detected Stack"),
                [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
                {
                    return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupApplyButton", "Apply WanaAI Enhancement"),
                    [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                    {
                        if (OnApplyCharacterEnhancement)
                        {
                            OnApplyCharacterEnhancement();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupAIControllerButton", "Create AI-Ready Controller"),
                    [OnCreateAIReadyController = Args.OnCreateAIReadyController]()
                    {
                        if (OnCreateAIReadyController)
                        {
                            OnCreateAIReadyController();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupConvertButton", "Convert to AI-Ready Subject"),
                    [OnConvertToAIReadySubject = Args.OnConvertToAIReadySubject]()
                    {
                        if (OnConvertToAIReadySubject)
                        {
                            OnConvertToAIReadySubject();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupSaveButton", "Save Progress"),
                    [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
                    {
                        if (OnSaveSubjectProgress)
                        {
                            OnSaveSubjectProgress();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupRestoreButton", "Restore Last Saved State"),
                    [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
                    {
                        if (OnRestoreSavedSubjectProgress)
                        {
                            OnRestoreSavedSubjectProgress();
                        }
                    },
                    220.0f)
            ]
        ],
        LOCTEXT("WanaWorksSubjectSetupDescription", "Step 1. Start with the selected subject. WanaWorks enhances your existing Character BP, AI Controller, and Animation Blueprint stack without replacing it."),
        LOCTEXT("WanaWorksSubjectSetupStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeAnimationIntegrationSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAnimationIntegrationSection", "Animation Integration"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAnimationIntegrationSummaryLabel", "Current Animation Stack"),
            [GetAnimationIntegrationText = Args.GetAnimationIntegrationText]()
            {
                return GetAnimationIntegrationText ? GetAnimationIntegrationText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksAnimationIntegrationDescription", "WanaWorks detects the linked Animation Blueprint from the selected pawn and reports whether the working-copy or finalized path can auto-attach and auto-wire the safe animation bridge without touching the original asset."),
        LOCTEXT("WanaWorksAnimationIntegrationStatus", "SAFE"));
}

TSharedRef<SWidget> MakeAnimationHookUsageSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAnimationHookUsageSection", "Animation Hook Usage"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAnimationHookUsageSummaryLabel", "How To Use The Current Hook State"),
            [GetAnimationHookUsageText = Args.GetAnimationHookUsageText]()
            {
                return GetAnimationHookUsageText ? GetAnimationHookUsageText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksAnimationHookUsageDescription", "Read the current hook state from the owner's WAYPlayerProfileComponent inside your existing Anim BP. For supported working subjects, WanaWorks can also auto-fill matching Anim BP fields or component references without replacing the Animation Blueprint."),
        LOCTEXT("WanaWorksAnimationHookUsageStatus", "READY"));
}

TSharedRef<SWidget> MakePhysicalStateSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksPhysicalStateSection", "Physical State"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksPhysicalStateSummaryLabel", "Current Body-State Layer"),
            [GetPhysicalStateText = Args.GetPhysicalStateText]()
            {
                return GetPhysicalStateText ? GetPhysicalStateText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksPhysicalStateDescription", "This is a lightweight, readable body-state layer for characters. WanaWorks does not replace locomotion, the current Anim BP, or AI control here."),
        LOCTEXT("WanaWorksPhysicalStateStatus", "ACTIVE"));
}

TSharedRef<SWidget> MakePresetsSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksPresetsSection", "Presets"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksPresetSummaryLabel", "Selected Preset Summary"),
                [GetWorkflowPresetSummaryText = Args.GetWorkflowPresetSummaryText]()
                {
                    return GetWorkflowPresetSummaryText ? GetWorkflowPresetSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksPresetDropdownLabel", "Preset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(220.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.WorkflowPresetOptions)
                .InitiallySelectedItem(Args.GetSelectedWorkflowPresetOption ? Args.GetSelectedWorkflowPresetOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnWorkflowPresetOptionSelected = Args.OnWorkflowPresetOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnWorkflowPresetOptionSelected)
                    {
                        OnWorkflowPresetOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedWorkflowPresetOption = Args.GetSelectedWorkflowPresetOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedWorkflowPresetOption ? GetSelectedWorkflowPresetOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksPresetDefault", "Full WanaAI Starter");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksPresetAutonomyLabel", "Core Workflow"),
                []()
                {
                    return LOCTEXT("WanaWorksPresetAutonomyText", "Enhance already uses the selected preset profile automatically. Open the preset controls below only when you want to force-apply the profile now or save a reusable variation.");
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            MakeCollapsibleWorkspaceDetailsSection(
                SectionHeaderFont,
                LOCTEXT("WanaWorksPresetActionsTitle", "Preset Actions"),
                LOCTEXT("WanaWorksPresetActionsDescription", "These are secondary controls for saving the current setup or manually forcing the preset now. The main workflow already stays preset-aware."),
                SNew(SWrapBox)
                + SWrapBox::Slot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    MakeFixedWidthButton(
                        LOCTEXT("WanaWorksApplyPresetButton", "Apply Preset"),
                        [OnApplyWorkflowPreset = Args.OnApplyWorkflowPreset]()
                        {
                            if (OnApplyWorkflowPreset)
                            {
                                OnApplyWorkflowPreset();
                            }
                        },
                        160.0f)
                ]
                + SWrapBox::Slot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    MakeFixedWidthButton(
                        LOCTEXT("WanaWorksSavePresetButton", "Save Current as Preset"),
                        [OnSaveWorkflowPreset = Args.OnSaveWorkflowPreset]()
                        {
                            if (OnSaveWorkflowPreset)
                            {
                                OnSaveWorkflowPreset();
                            }
                        },
                        190.0f)
                ])
        ],
        LOCTEXT("WanaWorksPresetsDescription", "Pick a reusable WanaWorks starter profile here. The main workflow uses the selected profile automatically while the manual preset controls stay tucked into this secondary area."),
        LOCTEXT("WanaWorksPresetsStatus", "READY"));
}

TSharedRef<SWidget> MakeValidationWorkflowSection(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const FText& StatusText,
    bool bIncludeEnvironmentReadiness)
{
    const FWanaCommandDefinition* ClassifyDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("classify_selected")));

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationGuidedLabel", "Guided Test"),
            [GetGuidedWorkflowSummaryText = Args.GetGuidedWorkflowSummaryText]()
            {
                return GetGuidedWorkflowSummaryText ? GetGuidedWorkflowSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationMainFlowLabel", "Main Workflow"),
            []()
            {
                return LOCTEXT("WanaWorksValidationMainFlowText", "Use Test on the main workflow strip for the guided evaluation pass. The helper controls below stay folded away for manual observer-target setup or deeper diagnostics only.");
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationLiveTestLabel", "Live Test"),
            [GetLiveTestSummaryText = Args.GetLiveTestSummaryText]()
            {
                return GetLiveTestSummaryText ? GetLiveTestSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationSandboxLabel", "Working Copy Pair"),
            [GetTestSandboxSummaryText = Args.GetTestSandboxSummaryText]()
            {
                return GetTestSandboxSummaryText ? GetTestSandboxSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationSandboxBuildLabel", "Build Final"),
            []()
            {
                return LOCTEXT("WanaWorksValidationSandboxBuildText", "When the working subject looks right, Build Final creates a separate finalized asset under /Game/WanaWorks/Builds. The original source and the working copy both stay preserved.");
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksValidationManualPairTitle", "Manual Pair Controls"),
            LOCTEXT("WanaWorksValidationManualPairDescription", "Use these only when you want explicit observer-target assignments or manual focus control beyond the main workflow."),
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksUseSelectedObserverButton_Validation", "Use Selected as Observer"),
                    [OnUseSelectedAsSandboxObserver = Args.OnUseSelectedAsSandboxObserver]()
                    {
                        if (OnUseSelectedAsSandboxObserver)
                        {
                            OnUseSelectedAsSandboxObserver();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksUseSelectedTargetButton_Validation", "Use Selected as Target"),
                    [OnUseSelectedAsSandboxTarget = Args.OnUseSelectedAsSandboxTarget]()
                    {
                        if (OnUseSelectedAsSandboxTarget)
                        {
                            OnUseSelectedAsSandboxTarget();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksEvaluateSandboxPairButton_Validation", "Evaluate Working Pair"),
                    [OnEvaluateSandboxPair = Args.OnEvaluateSandboxPair]()
                    {
                        if (OnEvaluateSandboxPair)
                        {
                            OnEvaluateSandboxPair();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksFocusObserverButton_Validation", "Focus Observer"),
                    [OnFocusSandboxObserver = Args.OnFocusSandboxObserver]()
                    {
                        if (OnFocusSandboxObserver)
                        {
                            OnFocusSandboxObserver();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksFocusTargetButton_Validation", "Focus Target"),
                    [OnFocusSandboxTarget = Args.OnFocusSandboxTarget]()
                    {
                        if (OnFocusSandboxTarget)
                        {
                            OnFocusSandboxTarget();
                        }
                    },
                    160.0f)
            ])
    ];

    if (bIncludeEnvironmentReadiness)
    {
        Layout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationWITLabel", "WIT Environment Readiness"),
                [GetWITEnvironmentReadinessText = Args.GetWITEnvironmentReadinessText]()
                {
                    return GetWITEnvironmentReadinessText ? GetWITEnvironmentReadinessText() : FText::GetEmpty();
                })
        ];

        Layout->AddSlot()
        .AutoHeight()
        [
            MakeCollapsibleWorkspaceDetailsSection(
                SectionHeaderFont,
                LOCTEXT("WanaWorksValidationDiagnosticsTitle", "Manual Diagnostics"),
                LOCTEXT("WanaWorksValidationDiagnosticsDescription", "Use these deeper checks only when you want to classify the current subject or inspect environment fit beyond the Analyze step."),
                SNew(SWrapBox)
                + SWrapBox::Slot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    ClassifyDefinition
                        ? MakeCommandButton(Args, *ClassifyDefinition)
                        : MakeFixedWidthButton(FText::FromString(TEXT("Classify Selected")), []() {})
                ]
                + SWrapBox::Slot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    MakeFixedWidthButton(
                        LOCTEXT("WanaWorksScanEnvironmentReadinessButton_Validation", "Scan Environment Readiness"),
                        [OnScanEnvironmentReadiness = Args.OnScanEnvironmentReadiness]()
                        {
                            if (OnScanEnvironmentReadiness)
                            {
                                OnScanEnvironmentReadiness();
                            }
                        },
                        210.0f)
                ])
        ];
    }

    return MakeSection(SectionHeaderFont, Title, Layout, Description, StatusText);
}

TSharedRef<SWidget> MakeResultsWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksResultsWorkflowSection", "Results"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsEnhancementLabel", "Enhancement Results"),
                [GetEnhancementResultsText = Args.GetEnhancementResultsText]()
                {
                    return GetEnhancementResultsText ? GetEnhancementResultsText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsBehaviorLabel", "Behavior Results"),
                [GetBehaviorResultsText = Args.GetBehaviorResultsText]()
                {
                    return GetBehaviorResultsText ? GetBehaviorResultsText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsSavedProgressLabel", "Saved Progress"),
                [GetSavedSubjectProgressText = Args.GetSavedSubjectProgressText]()
                {
                    return GetSavedSubjectProgressText ? GetSavedSubjectProgressText() : FText::GetEmpty();
                })
        ],
        LOCTEXT("WanaWorksResultsWorkflowDescription", "Step 4. Review the latest enhancement state, current behavior status, and saved subject progress without digging through the output log."),
        LOCTEXT("WanaWorksResultsWorkflowStatus", "SAFE"));
}

TSharedRef<SWidget> MakeCommandButton(const FWanaWorksUITabBuilderArgs& Args, const FWanaCommandDefinition& Definition)
{
    return MakeFixedWidthButton(
        FText::FromString(Definition.DisplayLabel),
        [OnExecuteCommandText = Args.OnExecuteCommandText, CommandTextValue = FString(Definition.CommandText)]()
        {
            if (OnExecuteCommandText)
            {
                OnExecuteCommandText(CommandTextValue);
            }
        });
}

TSharedRef<SWidget> MakeSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const TSharedRef<SWidget>& Content,
    const TOptional<FText>& Description,
    const TOptional<FText>& StatusText,
    bool bProminent)
{
    const FSlateFontInfo DescriptionFont = MakeStudioFont("Regular", 9);
    const FSlateFontInfo EffectiveHeaderFont = bProminent ? MakeStudioFont("Bold", 13) : SectionHeaderFont;
    TSharedRef<SVerticalBox> SectionLayout = SNew(SVerticalBox);

    SectionLayout->AddSlot()
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Font(EffectiveHeaderFont)
            .ColorAndOpacity(FLinearColor::White)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Title)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            StatusText.IsSet()
                ? StaticCastSharedRef<SWidget>(MakeStatusBadge(StatusText.GetValue()))
                : StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D::ZeroVector))
        ]
    ];

    if (Description.IsSet())
    {
        SectionLayout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Font(DescriptionFont)
            .AutoWrapText(true)
            .ColorAndOpacity(SecondaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Description.GetValue())
        ];
    }

    SectionLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 12.0f, 0.0f, 14.0f)
    [
        MakeStudioDivider(1.0f, StudioDividerColor.CopyWithNewOpacity(0.80f))
    ];

    SectionLayout->AddSlot()
    .AutoHeight()
    [
        Content
    ];

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(bProminent ? 0.58f : 0.40f))
        [
            SNew(SBorder)
            .Padding(bProminent ? FMargin(20.0f, 18.0f) : FMargin(16.0f, 14.0f))
            .BorderBackgroundColor(bProminent ? StudioPanelRaisedColor : StudioPanelColor)
            [
                SectionLayout
            ]
        ];
}

TSharedRef<SWidget> MakeWrappedButtonSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont, const FText& Title, std::initializer_list<const TCHAR*> CommandIds, const TOptional<FText>& Description = TOptional<FText>())
{
    TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox);

    for (const TCHAR* CommandId : CommandIds)
    {
        if (const FWanaCommandDefinition* Definition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(CommandId)))
        {
            WrapBox->AddSlot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    MakeCommandButton(Args, *Definition)
                ];
        }
    }

    return MakeSection(SectionHeaderFont, Title, WrapBox, Description);
}

TSharedRef<SWidget> MakeWaySection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    const FWanaCommandDefinition* EnsureDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("ensure_relationship_profile")));
    const FWanaCommandDefinition* ShowDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("show_relationship_profile")));
    const FWanaCommandDefinition* ApplyDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("apply_relationship_state")));

    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWAYSection", "WAY"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBorder)
            .Padding(10.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text_Lambda([GetRelationshipSummaryText = Args.GetRelationshipSummaryText]()
                {
                    return GetRelationshipSummaryText ? GetRelationshipSummaryText() : FText::GetEmpty();
                })
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksRelationshipStateLabel", "Relationship State For Target"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.RelationshipStateOptions)
                .InitiallySelectedItem(Args.GetSelectedRelationshipStateOption ? Args.GetSelectedRelationshipStateOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnRelationshipStateOptionSelected = Args.OnRelationshipStateOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnRelationshipStateOptionSelected)
                    {
                        OnRelationshipStateOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedRelationshipStateOption = Args.GetSelectedRelationshipStateOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedRelationshipStateOption ? GetSelectedRelationshipStateOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksRelationshipStateDefault", "Neutral");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                EnsureDefinition
                    ? MakeCommandButton(Args, *EnsureDefinition)
                    : MakeFixedWidthButton(FText::FromString(TEXT("Ensure Relationship Profile")), []() {})
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                ShowDefinition
                    ? MakeCommandButton(Args, *ShowDefinition)
                    : MakeFixedWidthButton(FText::FromString(TEXT("Show Relationship Profile")), []() {})
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    FText::FromString(ApplyDefinition ? ApplyDefinition->DisplayLabel : TEXT("Apply Relationship State")),
                    [OnApplyRelationshipState = Args.OnApplyRelationshipState]()
                    {
                        if (OnApplyRelationshipState)
                        {
                            OnApplyRelationshipState();
                        }
                    })
            ]
        ],
        LOCTEXT("WanaWorksWAYDescription", "Controls how an AI interprets and relates to another actor over time."));
}

TSharedRef<SWidget> MakeIdentitySection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksIdentitySection", "Identity"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBorder)
            .Padding(10.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text_Lambda([GetIdentitySummaryText = Args.GetIdentitySummaryText]()
                {
                    return GetIdentitySummaryText ? GetIdentitySummaryText() : FText::GetEmpty();
                })
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksFactionTagLabel", "Faction Tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 10.0f)
            [
                SNew(SEditableTextBox)
                .HintText(LOCTEXT("WanaWorksFactionTagHint", "Enter faction tag"))
                .Text_Lambda([GetIdentityFactionTagText = Args.GetIdentityFactionTagText]()
                {
                    return GetIdentityFactionTagText ? GetIdentityFactionTagText() : FText::GetEmpty();
                })
                .OnTextChanged_Lambda([OnIdentityFactionTagTextChanged = Args.OnIdentityFactionTagTextChanged](const FText& NewText)
                {
                    if (OnIdentityFactionTagTextChanged)
                    {
                        OnIdentityFactionTagTextChanged(NewText);
                    }
                })
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksIdentitySeedLabel", "Default Relationship Seed"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.RelationshipStateOptions)
                .InitiallySelectedItem(Args.GetSelectedIdentitySeedStateOption ? Args.GetSelectedIdentitySeedStateOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnIdentitySeedStateOptionSelected = Args.OnIdentitySeedStateOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnIdentitySeedStateOptionSelected)
                    {
                        OnIdentitySeedStateOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedIdentitySeedStateOption = Args.GetSelectedIdentitySeedStateOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedIdentitySeedStateOption ? GetSelectedIdentitySeedStateOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksIdentitySeedDefault", "Neutral");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksEnsureIdentityButton", "Ensure Identity Component"),
                    [OnEnsureIdentityComponent = Args.OnEnsureIdentityComponent]()
                    {
                        if (OnEnsureIdentityComponent)
                        {
                            OnEnsureIdentityComponent();
                        }
                    })
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksApplyIdentityButton", "Apply Identity"),
                    [OnApplyIdentity = Args.OnApplyIdentity]()
                    {
                        if (OnApplyIdentity)
                        {
                            OnApplyIdentity();
                        }
                    })
            ]
        ],
        LOCTEXT("WanaWorksIdentityDescription", "Defines how this actor is recognized by other systems, including faction and relationship seed data."));
}

TSharedRef<SWidget> MakeSubjectPathSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& Content,
    bool bInitiallyCollapsed = true)
{
    return SNew(SBorder)
        .Padding(12.0f)
        .BorderBackgroundColor(FLinearColor(0.06f, 0.09f, 0.13f, 0.84f))
        [
            SNew(SExpandableArea)
            .InitiallyCollapsed(bInitiallyCollapsed)
            .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
            .BodyBorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
            .HeaderPadding(FMargin(0.0f))
            .HeaderContent()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .Text(Title)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(Description)
                ]
            ]
            .BodyContent()
            [
                SNew(SBorder)
                .Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
                .BorderBackgroundColor(FLinearColor::Transparent)
                [
                    Content
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeAIPawnWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    TSharedRef<SWrapBox> ActionRow = SNew(SWrapBox);

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnCreateControllerButton", "Create AI-Ready Controller"),
            [OnCreateAIReadyController = Args.OnCreateAIReadyController]()
            {
                if (OnCreateAIReadyController)
                {
                    OnCreateAIReadyController();
                }
            },
            220.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnSaveProgressButton", "Save Progress"),
            [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
            {
                if (OnSaveSubjectProgress)
                {
                    OnSaveSubjectProgress();
                }
            },
            160.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnRestoreProgressButton", "Restore Last Saved State"),
            [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
            {
                if (OnRestoreSavedSubjectProgress)
                {
                    OnRestoreSavedSubjectProgress();
                }
            },
            220.0f)
    ];

    TSharedRef<SVerticalBox> WorkspaceControls = SNew(SVerticalBox);

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeStringPickerControl(
            LOCTEXT("WanaWorksAIPawnAssetPickerLabel", "AI Pawn Asset"),
            Args.AIPawnAssetOptions,
            Args.GetSelectedAIPawnAssetOption,
            Args.OnAIPawnAssetOptionSelected,
            LOCTEXT("WanaWorksAIPawnAssetPickerDefault", "(Choose AI Pawn)"))
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnBindingLabel", "Selected AI Pawn Binding"),
            [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
            {
                return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnStackLabel", "AI Pawn Stack"),
            [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
            {
                return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnGuidanceLabel", "Path Guidance"),
            []()
            {
                return LOCTEXT("WanaWorksAIPawnGuidanceText", "Choose an AI Pawn directly in WanaWorks, inspect it in the workspace, and keep the primary flow simple: create a copy, enhance it, test it, and build the final asset. Editor selection remains only a fallback shortcut.");
            })
    ];

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWorkflowWorkspaceSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnWorkspaceTitle", "AI Workspace"),
            LOCTEXT("WanaWorksAIPawnWorkspaceDescription", "Work from the picked AI subject on the left, shape it in the center, and keep live status on the right. WanaWorks is the workspace, so the main flow stays inside this surface."),
            WorkspaceControls)
    ];

    TSharedRef<SVerticalBox> SecondaryLayout = SNew(SVerticalBox);

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        ActionRow
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakePresetsSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeCharacterEnhancementSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeValidationWorkflowSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnValidationSection", "Testing & Pair Tools"),
            LOCTEXT("WanaWorksAIPawnValidationDescription", "Use this secondary area for deeper pair setup, live evaluation, WIT readiness, and explicit testing tools after the main workspace flow is already in motion."),
            LOCTEXT("WanaWorksAIPawnValidationStatus", "LIVE"),
            true)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeAnimationHookUsageSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeResultsWorkflowSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWrappedButtonSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnWAISection", "WAI"),
            {
                TEXT("add_memory_test"),
                TEXT("show_memory")
            },
            LOCTEXT("WanaWorksAIPawnWAIDescription", "Keeps memory and internal state tools close when you need deeper AI-side inspection."))
    ];

    Layout->AddSlot()
    .AutoHeight()
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnDetailsTitle", "Details & Tools"),
            LOCTEXT("WanaWorksAIPawnDetailsDescription", "Open this when you want presets, saved progress, explicit pair setup, WIT scans, WAY controls, and the deeper testing tools without cluttering the main workspace surface."),
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SecondaryLayout
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeWaySection(Args, SectionHeaderFont)
            ])
    ];

    return MakeSubjectPathSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAIPawnPathTitle", "AI Pawn"),
        LOCTEXT("WanaWorksAIPawnPathDescription", "Primary path for AI-driven pawns. Pick the subject, preview it, run the core workflow, then open details only when you need deeper tools."),
        Layout);
}

TSharedRef<SWidget> MakeCharacterPawnWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    TSharedRef<SWrapBox> ActionRow = SNew(SWrapBox);

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnConvertButton", "Convert to AI-Ready Subject"),
            [OnConvertToAIReadySubject = Args.OnConvertToAIReadySubject]()
            {
                if (OnConvertToAIReadySubject)
                {
                    OnConvertToAIReadySubject();
                }
            },
            220.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnSaveProgressButton", "Save Progress"),
            [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
            {
                if (OnSaveSubjectProgress)
                {
                    OnSaveSubjectProgress();
                }
            },
            160.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnRestoreProgressButton", "Restore Last Saved State"),
            [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
            {
                if (OnRestoreSavedSubjectProgress)
                {
                    OnRestoreSavedSubjectProgress();
                }
            },
            220.0f)
    ];

    TSharedRef<SVerticalBox> WorkspaceControls = SNew(SVerticalBox);

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeStringPickerControl(
            LOCTEXT("WanaWorksCharacterPawnAssetPickerLabel", "Character Pawn Asset"),
            Args.CharacterPawnAssetOptions,
            Args.GetSelectedCharacterPawnAssetOption,
            Args.OnCharacterPawnAssetOptionSelected,
            LOCTEXT("WanaWorksCharacterPawnAssetPickerDefault", "(Choose Character Pawn)"))
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnBindingLabel", "Selected Character Pawn Binding"),
            [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
            {
                return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnStackLabel", "Character Pawn Stack"),
            [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
            {
                return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnGuidanceLabel", "Path Guidance"),
            []()
            {
                return LOCTEXT("WanaWorksCharacterPawnGuidanceText", "Choose a Character Pawn directly in WanaWorks, inspect it in the workspace, and keep the flow simple: create a copy, enhance it, test it, and build the final asset. Editor selection still works as a secondary fallback.");
            })
    ];

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWorkflowWorkspaceSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnWorkspaceTitle", "Character Workspace"),
            LOCTEXT("WanaWorksCharacterPawnWorkspaceDescription", "Keep subject setup on the left, the main working view in the center, and live results on the right so the workflow feels like one clean workspace."),
            WorkspaceControls)
    ];

    TSharedRef<SVerticalBox> SecondaryLayout = SNew(SVerticalBox);

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        ActionRow
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeIdentitySection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakePresetsSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeCharacterEnhancementSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeValidationWorkflowSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnValidationSection", "Testing & Pair Tools"),
            LOCTEXT("WanaWorksCharacterPawnValidationDescription", "Use this secondary area for live evaluation, pair assignment, and deeper workspace testing after the main character flow is already set up."),
            LOCTEXT("WanaWorksCharacterPawnValidationStatus", "LIVE"),
            false)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeAnimationHookUsageSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnWAYAccessLabel", "Relationship Access"),
            []()
            {
                return LOCTEXT("WanaWorksCharacterPawnWAYAccessText", "WAY is strongest in the AI Pawn path after this subject is converted or paired for AI-style testing. That keeps relationship tools available without overloading the main character workspace.");
            })
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeResultsWorkflowSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    [
        MakeWrappedButtonSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnWAISection", "WAI"),
            {
                TEXT("add_memory_test"),
                TEXT("show_memory")
            },
            LOCTEXT("WanaWorksCharacterPawnWAIDescription", "Lets a character-focused subject use the same memory and internal-state tools without forcing an AI replacement workflow."))
    ];

    Layout->AddSlot()
    .AutoHeight()
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnDetailsTitle", "Details & Tools"),
            LOCTEXT("WanaWorksCharacterPawnDetailsDescription", "Open this when you want Identity editing, saved progress, presets, deeper testing, or the lower-level helper tools without cluttering the main workspace surface."),
            SecondaryLayout)
    ];

    return MakeSubjectPathSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCharacterPawnPathTitle", "Character Pawn"),
        LOCTEXT("WanaWorksCharacterPawnPathDescription", "Primary path for character-first setup. Pick the subject, preview it, run the core workflow, then open details only when you need the deeper tools."),
        Layout);
}

TSharedRef<SWidget> MakeWanaAISection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWanaAISection", "WanaAI"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing + 4.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWanaAIPathChooserLabel", "Choose a Subject Path"),
                []()
                {
                    return LOCTEXT("WanaWorksWanaAIPathChooserText", "Pick the path that matches the subject you want to shape inside WanaWorks. The main flow is simple: choose a subject, preview it, create a working copy, apply enhancement, test it, then build a clean final asset.");
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAIPawnWorkflowSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeCharacterPawnWorkflowSection(Args, SectionHeaderFont)
        ],
        TOptional<FText>(),
        LOCTEXT("WanaWorksActiveStatus", "ACTIVE"));
}

TSharedRef<SWidget> MakeAdvancedCommandSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAdvancedCommandSection", "Advanced Command"),
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SEditableTextBox)
            .HintText(LOCTEXT("WanaWorksCommandHint", "Enter command"))
            .Text_Lambda([GetCommandText = Args.GetCommandText]()
            {
                return GetCommandText ? GetCommandText() : FText::GetEmpty();
            })
            .OnTextChanged_Lambda([OnCommandTextChanged = Args.OnCommandTextChanged](const FText& NewText)
            {
                if (OnCommandTextChanged)
                {
                    OnCommandTextChanged(NewText);
                }
            })
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .WidthOverride(120.0f)
            .HeightOverride(30.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(LOCTEXT("WanaWorksRunButton", "Run"))
                .OnClicked_Lambda([OnRunCommand = Args.OnRunCommand]()
                {
                    if (OnRunCommand)
                    {
                        OnRunCommand();
                    }

                    return FReply::Handled();
                })
            ]
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(120.0f)
            .HeightOverride(30.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(LOCTEXT("WanaWorksClearLogButton", "Clear Log"))
                .OnClicked_Lambda([OnClearLog = Args.OnClearLog]()
                {
                    if (OnClearLog)
                    {
                        OnClearLog();
                    }

                    return FReply::Handled();
                })
            ]
        ]);
}

TSharedRef<SWidget> MakeOutputSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksOutputSection", "Logs & Output"),
        SNew(SBorder)
        .Padding(10.0f)
        .BorderBackgroundColor(FLinearColor(0.05f, 0.08f, 0.11f, 0.85f))
        [
            SNew(SBox)
            .MinDesiredHeight(180.0f)
            [
                SNew(SMultiLineEditableTextBox)
                .IsReadOnly(true)
                .Text_Lambda([GetLogText = Args.GetLogText]()
                {
                    return GetLogText ? GetLogText() : FText::GetEmpty();
                })
            ]
        ],
        LOCTEXT("WanaWorksOutputDescription", "Secondary details for action results, testing feedback, and structured observer-target summaries."));
}

struct FStudioWorkspaceEntry
{
    FString Label;
    FText Subtitle;
};

FText GetWorkspaceNavigationLabel(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioNavCharacterIntelligenceLabel", "Character Intelligence");
    }

    return FText::FromString(WorkspaceLabel);
}

FText GetWorkspaceHeroTitle(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterIntelligenceTitle", "AI / Character Intelligence");
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterBuildingTitle", "Character Building");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroLevelDesignTitle", "Level Design");
    }

    return FText::FromString(WorkspaceLabel);
}

FText GetWorkspaceHeroSubtitle(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterIntelligenceSubtitle", "Orchestrate WAI/WAMI, WanaAnimation-lite, WanaCombat-lite, WAY-lite, WIT world meaning, and WanaUI-guided workflow over your existing UE5 character stack without forcing replacement.");
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterBuildingSubtitle", "Augment existing skeletal mesh, rig, animation blueprint, identity, and character-stack readiness non-destructively through the WanaWorks authoring layer.");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroLevelDesignSubtitle", "Frame WIT as semantic world understanding so cover meaning, obstacle meaning, movement meaning, and scene context live in the level-facing workspace instead of cluttering character intelligence.");
    }

    return LOCTEXT("WanaWorksStudioHeroFutureWorkspaceSubtitle", "Platform workspace shell present. Capability expansion can land here in a later phase.");
}

FText GetWorkspaceHeroBadge(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterIntelligenceBadge", "CHARACTER INTELLIGENCE");
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroCharacterBuildingBadge", "CHARACTER BUILDING");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return LOCTEXT("WanaWorksStudioHeroLevelDesignBadge", "LEVEL DESIGN");
    }

    return LOCTEXT("WanaWorksStudioHeroFutureWorkspaceBadge", "PLATFORM WORKSPACE");
}

FLinearColor GetWorkspaceAccentColor(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return StudioAccentColor;
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return StudioAccentGoldColor;
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.22f, 0.82f, 0.78f, 1.0f);
    }

    return FLinearColor(0.48f, 0.62f, 0.92f, 1.0f);
}

bool IsWorkspaceActive(const FWanaWorksUITabBuilderArgs& Args, const FString& WorkspaceLabel)
{
    return Args.GetSelectedWorkspaceLabel
        && Args.GetSelectedWorkspaceLabel().Equals(WorkspaceLabel, ESearchCase::IgnoreCase);
}

TSharedRef<SWidget> MakeStudioHeroStage(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString WorkspaceLabel = Args.GetSelectedWorkspaceLabel
        ? Args.GetSelectedWorkspaceLabel()
        : TEXT("AI");

    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.26f))
        [
            SNew(SBorder)
            .Padding(14.0f)
            .BorderBackgroundColor(StudioStageChromeColor)
            [
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioStageWashColor.CopyWithNewOpacity(0.98f))
                [
                    SNew(SWanaWorksSandboxPreviewCard)
                    .GetPreviewObject(Args.GetSandboxPreviewObject)
                    .GetPreviewSummaryText(Args.GetSandboxPreviewSummaryText)
                    .GetSelectedPreviewViewLabel(Args.GetSelectedPreviewViewLabel)
                    .GetSelectedWorkspaceLabel(Args.GetSelectedWorkspaceLabel)
                    .OnFocusPreviewSubject(Args.OnFocusSandboxPreviewSubject)
                    .OnSelectPreviewView(Args.OnPreviewViewSelected)
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioStatusCard(
    const FText& Title,
    const FText& Eyebrow,
    TFunction<FText(void)> GetSummaryText,
    const FLinearColor& AccentColor,
    int32 MaxRows = 6,
    float ValueWrapWidth = 220.0f)
{
    return SNew(SWanaWorksStudioSummaryCard)
        .Title(Title)
        .Eyebrow(Eyebrow)
        .AccentColor(AccentColor)
        .GetSummaryText(GetSummaryText)
        .MaxRows(MaxRows)
        .ValueWrapWidth(ValueWrapWidth);
}

TSharedRef<SWidget> MakeWorkspaceRailButton(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel,
    const FText& Subtitle)
{
    const bool bLiveWorkspace = IsLiveWorkspaceLabel(WorkspaceLabel);
    const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);
    const FText DisplayLabel = GetWorkspaceNavigationLabel(WorkspaceLabel);

    return WanaWorksUIStyle::WanaWorkspaceRailItem(
        DisplayLabel,
        Subtitle,
        [Args, WorkspaceLabel, bLiveWorkspace]()
        {
            if (IsWorkspaceActive(Args, WorkspaceLabel))
            {
                return LOCTEXT("WanaWorksStudioNavStateActive", "ACTIVE");
            }

            return bLiveWorkspace
                ? LOCTEXT("WanaWorksStudioNavStateAvailable", "AVAILABLE")
                : LOCTEXT("WanaWorksStudioNavStateFuture", "COMING LATER");
        },
        WanaWorksUIStyle::GetWorkspaceIconName(WorkspaceLabel),
        AccentColor,
        bLiveWorkspace,
        [Args, WorkspaceLabel]()
        {
            return IsWorkspaceActive(Args, WorkspaceLabel);
        },
        [OnWorkspaceSelected = Args.OnWorkspaceSelected, WorkspaceLabel]()
        {
            if (OnWorkspaceSelected)
            {
                OnWorkspaceSelected(WorkspaceLabel);
            }
        });
}

TSharedRef<SWidget> MakeStudioNavigationRail(const FWanaWorksUITabBuilderArgs& Args)
{
    const TArray<FStudioWorkspaceEntry> WorkspaceEntries =
    {
        { TEXT("AI"), LOCTEXT("WanaWorksStudioNavAISubtitle", "WAI/WAMI, WIT, animation, combat, UI") },
        { TEXT("Character Building"), LOCTEXT("WanaWorksStudioNavCharacterSubtitle", "Rig, animation, identity, build readiness") },
        { TEXT("Level Design"), LOCTEXT("WanaWorksStudioNavLevelSubtitle", "Semantic world meaning and scene context") },
        { TEXT("Logic & Blueprints"), LOCTEXT("WanaWorksStudioNavLogicSubtitle", "UE5-native orchestration lane") },
        { TEXT("Physics"), LOCTEXT("WanaWorksStudioNavPhysicsSubtitle", "Future physical middleware lane") },
        { TEXT("Audio"), LOCTEXT("WanaWorksStudioNavAudioSubtitle", "Future audio integration lane") },
        { TEXT("UI / UX"), LOCTEXT("WanaWorksStudioNavUISubtitle", "WanaUI product-surface lane") },
        { TEXT("Optimize"), LOCTEXT("WanaWorksStudioNavOptimizeSubtitle", "Future optimization lane") },
        { TEXT("Build & Deploy"), LOCTEXT("WanaWorksStudioNavBuildSubtitle", "Future shipping and deploy lane") }
    };

    TSharedRef<SVerticalBox> WorkspaceButtons = SNew(SVerticalBox);

    for (const FStudioWorkspaceEntry& WorkspaceEntry : WorkspaceEntries)
    {
        WorkspaceButtons->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 7.0f)
        [
            MakeWorkspaceRailButton(Args, WorkspaceEntry.Label, WorkspaceEntry.Subtitle)
        ];
    }

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.32f))
        [
            SNew(SBorder)
            .Padding(18.0f)
            .BorderBackgroundColor(StudioShellRaisedColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 22.0f)
                [
                    SNew(SBorder)
                    .Padding(20.0f)
                    .BorderBackgroundColor(FLinearColor(0.028f, 0.033f, 0.058f, 0.98f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 20))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(LOCTEXT("WanaWorksStudioBrand", "WanaWorks"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioBrandEyebrow", "MODULAR UE5 CHARACTER PLATFORM"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Regular", 9))
                            .ColorAndOpacity(SecondaryTextColor)
                            .AutoWrapText(true)
                            .Text(LOCTEXT("WanaWorksStudioBrandSubtitle", "Non-destructive orchestration layer for character intelligence, animation, combat response, and editor-side authoring over UE5-native systems."))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 14.0f, 0.0f, 0.0f)
                        [
                            MakeStudioDivider(1.0f, StudioAccentColor.CopyWithNewOpacity(0.45f))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 8))
                    .ColorAndOpacity(TertiaryTextColor)
                    .Text(LOCTEXT("WanaWorksStudioWorkspacesLabel", "WORKSPACES"))
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    WorkspaceButtons
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(16.0f)
                    .BorderBackgroundColor(FLinearColor(0.026f, 0.031f, 0.054f, 0.98f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioProjectLabel", "PROJECT"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(FLinearColor::White)
                            .Font(MakeStudioFont("Bold", 11))
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(LOCTEXT("WanaWorksStudioProjectValue", "WanaDemo"))
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioSubjectColumn(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    const FText PickerLabel = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIPickerLabel", "AI Pawn Asset")
        : LOCTEXT("WanaWorksStudioCharacterPickerLabel", "Character Pawn Asset");
    const FText PickerDefault = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIPickerDefault", "(Choose AI Pawn)")
        : LOCTEXT("WanaWorksStudioCharacterPickerDefault", "(Choose Character Pawn)");

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
        [
            SNew(SBorder)
            .Padding(22.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 8))
                    .ColorAndOpacity(TertiaryTextColor)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelEyebrow", "SUBJECT STACK")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelEyebrow", "CHARACTER BUILD"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 17))
                    .ColorAndOpacity(FLinearColor::White)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelTitle", "AI Subject")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelTitle", "Character Build Subject"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 8.0f, 0.0f, 18.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelSubtitle", "Select the AI-facing subject and let WanaWorks surface stack readiness automatically.")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelSubtitle", "Select the character-facing subject and let WanaWorks surface identity fit, animation readiness, and final build context."))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    MakeStringPickerControl(
                        PickerLabel,
                        bAIWorkspace ? Args.AIPawnAssetOptions : Args.CharacterPawnAssetOptions,
                        bAIWorkspace ? Args.GetSelectedAIPawnAssetOption : Args.GetSelectedCharacterPawnAssetOption,
                        bAIWorkspace ? Args.OnAIPawnAssetOptionSelected : Args.OnCharacterPawnAssetOptionSelected,
                        PickerDefault,
                        260.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioSelectedSubjectCard", "Selected Subject"),
                        LOCTEXT("WanaWorksStudioSelectedSubjectEyebrow", "CURRENT"),
                        Args.GetSubjectSetupSummaryText,
                        GetWorkspaceAccentColor(bAIWorkspace ? TEXT("AI") : TEXT("Character Building")),
                        4,
                        190.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioDetectedStackCard", "Detected Stack"),
                        LOCTEXT("WanaWorksStudioDetectedStackEyebrow", "LIVE"),
                        Args.GetSubjectStackSummaryText,
                        FLinearColor(0.17f, 0.54f, 0.60f, 1.0f),
                        8,
                        190.0f)
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioStatusColumn(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            bAIWorkspace
                ? MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioAnimationCardTitle", "Animation Integration"),
                    LOCTEXT("WanaWorksStudioAnimationCardEyebrow", "INTEGRATION"),
                    Args.GetAnimationIntegrationText,
                    StudioSuccessColor,
                    7,
                    210.0f)
                : MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioCharacterIdentityCardTitle", "Identity Profile"),
                    LOCTEXT("WanaWorksStudioCharacterIdentityCardEyebrow", "CHARACTER"),
                    Args.GetIdentitySummaryText,
                    StudioAccentGoldColor,
                    6,
                    210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioPhysicalCardTitle", "Physical State")
                    : LOCTEXT("WanaWorksStudioCharacterAnimationCardTitle", "Animation Integration"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioPhysicalCardEyebrow", "BODY")
                    : LOCTEXT("WanaWorksStudioCharacterAnimationCardEyebrow", "BRIDGE"),
                bAIWorkspace ? Args.GetPhysicalStateText : Args.GetAnimationIntegrationText,
                bAIWorkspace
                    ? StudioAccentBlueColor
                    : StudioSuccessColor,
                7,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioBehaviorCardTitle", "Behavior Results")
                    : LOCTEXT("WanaWorksStudioCharacterPhysicalCardTitle", "Physical State"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioBehaviorCardEyebrow", "LIVE AI")
                    : LOCTEXT("WanaWorksStudioCharacterPhysicalCardEyebrow", "BODY"),
                bAIWorkspace ? Args.GetBehaviorResultsText : Args.GetPhysicalStateText,
                bAIWorkspace
                    ? StudioSuccessColor
                    : StudioAccentBlueColor,
                8,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioEnvironmentCardTitle", "Environment Fit")
                    : LOCTEXT("WanaWorksStudioCharacterBuildCardTitle", "Build Readiness"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioEnvironmentCardEyebrow", "ANALYZE")
                    : LOCTEXT("WanaWorksStudioCharacterBuildCardEyebrow", "OUTPUT"),
                bAIWorkspace ? Args.GetWITEnvironmentReadinessText : Args.GetEnhancementResultsText,
                bAIWorkspace
                    ? FLinearColor(0.25f, 0.74f, 0.74f, 1.0f)
                    : StudioAccentGoldColor,
                8,
                210.0f)
        ];
}

TSharedRef<SWidget> MakeWorkflowTile(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed)
{
    return WanaWorksUIStyle::WanaActionTile(
        StepNumber,
        Title,
        Description,
        WanaWorksUIStyle::GetWorkflowIconName(Title.ToString()),
        AccentColor,
        OnPressed);
}

TSharedRef<SWidget> MakeStudioWorkflowStrip(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    const FLinearColor WorkspaceAccent = GetWorkspaceAccentColor(bAIWorkspace ? TEXT("AI") : TEXT("Character Building"));
    const TSharedRef<SWidget> FlowBadge = WanaWorksUIStyle::WanaStatusPill(
        bAIWorkspace
            ? LOCTEXT("WanaWorksStudioWorkflowStripAIBadge", "WanaUI ORCHESTRATION")
            : LOCTEXT("WanaWorksStudioWorkflowStripCharacterBadge", "CHARACTER AUTHORING FLOW"),
        WorkspaceAccent,
        true,
        8,
        FMargin(12.0f, 6.0f));

    const TSharedRef<SWidget> WorkflowTiles =
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("1"),
                LOCTEXT("WanaWorksStudioWorkflowEnhanceTitle", "Enhance"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioWorkflowEnhanceAIText", "Attach the character-intelligence orchestration layer to the existing Character BP, AI Controller, Behavior Tree or State Tree, and Anim BP stack without destructive replacement.")
                    : LOCTEXT("WanaWorksStudioWorkflowEnhanceCharacterText", "Augment the current character stack non-destructively across skeletal mesh, rig, animation blueprint, identity, and build-readiness context."),
                WorkspaceAccent,
                Args.OnEnhanceWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("2"),
                LOCTEXT("WanaWorksStudioWorkflowTestTitle", "Test"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioWorkflowTestAIText", "Run the WanaUI-guided character-intelligence pass with WAI/WAMI state, WanaAnimation-lite hooks, WanaCombat-lite response, and WAY-lite adaptation in view.")
                    : LOCTEXT("WanaWorksStudioWorkflowTestCharacterText", "Run the character-facing readiness pass and validate preview, movement/playability state, animation bridge safety, and authored stack integrity."),
                StudioAccentBlueColor,
                Args.OnTestWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("3"),
                LOCTEXT("WanaWorksStudioWorkflowAnalyzeTitle", "Analyze"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioWorkflowAnalyzeAIText", "Review controller, Behavior Tree or State Tree readiness, animation integration, physical behavior state, relationship context, WIT environment awareness, and suggested AI improvements before optimizing.")
                    : LOCTEXT("WanaWorksStudioWorkflowAnalyzeCharacterText", "Inspect mesh, skeleton/rig, animation blueprint, movement/playability, camera/control readiness, character systems, and suggested character build improvements before optimizing."),
                FLinearColor(0.22f, 0.73f, 0.78f, 1.0f),
                Args.OnAnalyzeWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            MakeWorkflowTile(
                TEXT("4"),
                LOCTEXT("WanaWorksStudioWorkflowBuildTitle", "Build"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioWorkflowBuildAIText", "Save the finalized character-intelligence output quietly to /Game/WanaWorks/Builds while preserving the original source stack and keeping the workspace in focus.")
                    : LOCTEXT("WanaWorksStudioWorkflowBuildCharacterText", "Save the finalized character build asset quietly to /Game/WanaWorks/Builds while preserving the original source stack and keeping the workspace in focus."),
                StudioAccentGoldColor,
                Args.OnFinalizeSandboxBuild)
        ];

    return WanaWorksUIStyle::WanaCard(
        LOCTEXT("WanaWorksStudioWorkflowStripEyebrow", "CORE FLOW"),
        LOCTEXT("WanaWorksStudioWorkflowStripTitle", "Enhance. Test. Analyze. Build."),
        WorkspaceAccent,
        WorkflowTiles,
        FlowBadge,
        true);
}

TSharedRef<SWidget> MakeStudioDetailsArea(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont, bool bAIWorkspace)
{
    TSharedRef<SVerticalBox> WorkspaceDetails = SNew(SVerticalBox);

    if (bAIWorkspace)
    {
        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakePresetsSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeCharacterEnhancementSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeValidationWorkflowSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioAIDetailsValidationTitle", "AI Testing & Pair Tools"),
                LOCTEXT("WanaWorksStudioAIDetailsValidationDescription", "Use deeper validation, pair setup, and WIT tools here when you want more than the main studio workflow."),
                LOCTEXT("WanaWorksStudioAIDetailsValidationStatus", "LIVE"),
                true)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAnimationHookUsageSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeResultsWorkflowSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioAIWAISection", "WAI"),
                {
                    TEXT("add_memory_test"),
                    TEXT("show_memory")
                },
                LOCTEXT("WanaWorksStudioAIWAIDescription", "Memory and internal-state tools for deeper AI-side inspection."))
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        [
            MakeWaySection(Args, SectionHeaderFont)
        ];
    }
    else
    {
        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeIdentitySection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakePresetsSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeCharacterEnhancementSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeValidationWorkflowSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationTitle", "Character Testing & Pair Tools"),
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationDescription", "Use deeper testing, pair setup, and conversion-side tools here when the main character workflow needs more control."),
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationStatus", "LIVE"),
                false)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAnimationHookUsageSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeResultsWorkflowSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioCharacterWAISection", "WAI"),
                {
                    TEXT("add_memory_test"),
                    TEXT("show_memory")
                },
                LOCTEXT("WanaWorksStudioCharacterWAIDescription", "Memory and internal-state tools for character-focused setup without cluttering the main shell."))
        ];
    }

    TSharedRef<SWidget> SceneTools = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioWeatherSection", "Weather"),
                {
                    TEXT("weather_clear"),
                    TEXT("weather_overcast"),
                    TEXT("weather_storm")
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioSceneSection", "Scene"),
                {
                    TEXT("spawn_cube"),
                    TEXT("list_selection")
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeAdvancedCommandSection(Args, SectionHeaderFont)
        ];

    TSharedRef<SWidget> OutputTools = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeOutputSection(Args, SectionHeaderFont)
        ];

    return MakeCollapsibleWorkspaceDetailsSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksStudioDetailsTitle", "Advanced Controls & Output"),
        LOCTEXT("WanaWorksStudioDetailsDescription", "Open this secondary drawer only when you need presets, deep testing, scene helpers, or logs beyond the main workflow surface."),
        SNew(SWrapBox)
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 14.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(430.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioWorkflowToolsTitle", "Workflow Tools"),
                    LOCTEXT("WanaWorksStudioWorkflowToolsDescription", "Presets, deeper testing, system guidance, and advanced subject controls stay here as a secondary lane."),
                    WorkspaceDetails)
            ]
        ]
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 14.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(360.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioSceneToolsTitle", "Scene & Utility Tools"),
                    LOCTEXT("WanaWorksStudioSceneToolsDescription", "Environment presets, quick scene actions, and command utilities remain available without crowding the workspace."),
                    SceneTools)
            ]
        ]
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 0.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(420.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioLogsToolsTitle", "Logs & Build Output"),
                    LOCTEXT("WanaWorksStudioLogsToolsDescription", "Quiet build references and detailed output stay here when you need the deeper audit trail."),
                    OutputTools)
            ]
        ]);
}

TSharedRef<SWidget> MakeActiveStudioWorkspace(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FString& WorkspaceLabel)
{
    const bool bAIWorkspace = WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase);
    const FText WorkspaceTitle = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAITitle", "AI Workspace")
        : LOCTEXT("WanaWorksStudioCharacterTitle", "Character Building");
    const FText WorkspaceDescription = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIDescription", "Run the premium AI workflow with only the live subject, hero preview, status cards, and the four core actions in view.")
        : LOCTEXT("WanaWorksStudioCharacterDescription", "Focus on character identity, animation bridge quality, physical response, and clean build readiness inside a dedicated studio workspace.");

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeStudioPill(
                    bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAIWorkspaceChip", "AUTONOMOUS AI")
                        : LOCTEXT("WanaWorksStudioCharacterWorkspaceChip", "CHARACTER STUDIO"),
                    GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.16f),
                    FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                    8,
                    FMargin(12.0f, 6.0f))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(WorkspaceTitle)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Font(MakeStudioFont("Regular", 10))
                .ColorAndOpacity(SecondaryTextColor)
                .Text(WorkspaceDescription)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(bAIWorkspace ? 0.24f : 0.25f)
            [
                MakeStudioSubjectColumn(Args, bAIWorkspace)
            ]
            + SSplitter::Slot()
            .Value(bAIWorkspace ? 0.46f : 0.43f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(bAIWorkspace ? 0.30f : 0.32f)
            [
                MakeStudioStatusColumn(Args, bAIWorkspace)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 0.0f)
        [
            MakeStudioWorkflowStrip(Args, bAIWorkspace)
        ];
}

TSharedRef<SWidget> MakeLevelDesignWorkspaceSurface(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .Text(LOCTEXT("WanaWorksStudioLevelDesignTitle", "Level Design"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .ColorAndOpacity(SecondaryTextColor)
                .Text(LOCTEXT("WanaWorksStudioLevelDesignDescription", "Scene-side utilities live here so the AI workspace can stay clean, minimal, and fully focused on the four-step subject workflow."))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.26f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeSection(
                        SectionHeaderFont,
                        LOCTEXT("WanaWorksStudioLevelWorkspaceToolsTitle", "Scene Workspace"),
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            MakeReadOnlyInfoPanel(
                                LOCTEXT("WanaWorksStudioLevelWorkspaceToolsLabel", "Intent"),
                                []()
                                {
                                    return LOCTEXT("WanaWorksStudioLevelWorkspaceToolsText", "This lane now owns the quick scene and utility actions that were removed from the AI workspace surface.");
                                })
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            MakeWrappedButtonSection(
                                Args,
                                SectionHeaderFont,
                                LOCTEXT("WanaWorksStudioLevelWeatherSection", "Weather"),
                                {
                                    TEXT("weather_clear"),
                                    TEXT("weather_overcast"),
                                    TEXT("weather_storm")
                                })
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            MakeWrappedButtonSection(
                                Args,
                                SectionHeaderFont,
                                LOCTEXT("WanaWorksStudioLevelSceneSection", "Scene"),
                                {
                                    TEXT("spawn_cube"),
                                    TEXT("list_selection")
                                })
                        ],
                        LOCTEXT("WanaWorksStudioLevelWorkspaceSectionDescription", "Environment and scene actions stay available here without polluting the AI or Character workspaces."),
                        LOCTEXT("WanaWorksStudioLevelWorkspaceSectionStatus", "AVAILABLE"))
                ]
            ]
            + SSplitter::Slot()
            .Value(0.46f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(0.28f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelStatusTitle", "Workspace Status"),
                        LOCTEXT("WanaWorksStudioLevelStatusEyebrow", "LIVE"),
                        Args.GetStatusText,
                        FLinearColor(0.20f, 0.46f, 0.76f, 1.0f),
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelPreviewTitle", "Preview Subject"),
                        LOCTEXT("WanaWorksStudioLevelPreviewEyebrow", "STAGE"),
                        Args.GetSandboxPreviewSummaryText,
                        FLinearColor(0.42f, 0.28f, 0.82f, 1.0f),
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelSavedTitle", "Saved Progress"),
                        LOCTEXT("WanaWorksStudioLevelSavedEyebrow", "PROJECT"),
                        Args.GetSavedSubjectProgressText,
                        FLinearColor(0.18f, 0.62f, 0.58f, 1.0f),
                        4,
                        210.0f)
                ]
            ]
        ];
}

TSharedRef<SWidget> BuildFutureWorkspacePlaceholder(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(FText::FromString(WorkspaceLabel))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeStudioPill(
                    LOCTEXT("WanaWorksStudioFutureWorkspaceBadge", "COMING LATER"),
                    GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.18f),
                    GetWorkspaceAccentColor(WorkspaceLabel),
                    8,
                    FMargin(12.0f, 6.0f))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 6.0f, 0.0f, 16.0f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Font(MakeStudioFont("Regular", 10))
            .ColorAndOpacity(SecondaryTextColor)
            .Text(LOCTEXT("WanaWorksStudioFutureWorkspaceText", "This lane is part of the broader WanaWorks platform shell already, but its production workflow is intentionally staged for a later pass. Character Intelligence, Character Building, and Level Design are the live orchestration workspaces right now."))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.28f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioFutureIntentTitle", "Workspace Intent"),
                    LOCTEXT("WanaWorksStudioFutureIntentEyebrow", "PLATFORM"),
                    []()
                    {
                        return LOCTEXT("WanaWorksStudioFutureIntentText", "Status: Future platform lane\nRole: Middleware extension surface\nPreview: Shared workspace stage\nReadiness: Shell present\nNext Step: Expand module capability in a later phase");
                    },
                    GetWorkspaceAccentColor(WorkspaceLabel),
                    5,
                    220.0f)
            ]
            + SSplitter::Slot()
            .Value(0.42f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(0.30f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioFuturePlatformTitle", "Platform Status"),
                    LOCTEXT("WanaWorksStudioFuturePlatformEyebrow", "LIVE SHELL"),
                    Args.GetSavedSubjectProgressText,
                    FLinearColor(0.19f, 0.46f, 0.74f, 1.0f),
                    5,
                    220.0f)
            ]
        ];
}

TSharedRef<SWidget> MakeWorkspaceContextPanel(
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& Content)
{
    const TSharedRef<SWidget> PanelContent =
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 20.0f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Font(MakeStudioFont("Regular", 10))
            .ColorAndOpacity(SecondaryTextColor)
            .Text(Description)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            Content
        ];

    return WanaWorksUIStyle::WanaCard(
        Eyebrow,
        Title,
        AccentColor,
        PanelContent,
        TSharedPtr<SWidget>(),
        true);
}

void AddWrappedCommandButton(
    TSharedRef<SWrapBox> ButtonRow,
    const FWanaWorksUITabBuilderArgs& Args,
    const TCHAR* CommandId)
{
    if (const FWanaCommandDefinition* Definition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(CommandId)))
    {
        ButtonRow->AddSlot()
        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
        [
            MakeCommandButton(Args, *Definition)
        ];
    }
}

TSharedRef<SWidget> MakeWorkspaceStageShell(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel,
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const TArray<FText>& ContextChips)
{
    const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);
    TSharedRef<SWrapBox> ChipRow = SNew(SWrapBox);

    for (const FText& ChipLabel : ContextChips)
    {
        ChipRow->AddSlot()
        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
        [
            MakeStudioPill(
                ChipLabel,
                AccentColor.CopyWithNewOpacity(0.15f),
                FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                8,
                FMargin(12.0f, 6.0f))
        ];
    }

    const TSharedRef<SWidget> StageContent =
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Font(MakeStudioFont("Regular", 11))
            .ColorAndOpacity(SecondaryTextColor)
            .Text(Description)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            ChipRow
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioHeroStage(Args)
        ];

    return WanaWorksUIStyle::WanaCard(
        Eyebrow,
        Title,
        AccentColor,
        StageContent,
        TSharedPtr<SWidget>(),
        true);
}

TSharedRef<SWidget> BuildCharacterIntelligenceWorkspaceBody(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString WorkspaceLabel(TEXT("AI"));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.20f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioAILeftEyebrow", "CHARACTER INTELLIGENCE"),
                    LOCTEXT("WanaWorksStudioAILeftTitle", "AI Diagnosis Context"),
                    LOCTEXT("WanaWorksStudioAILeftDescription", "Pick the NPC or AI Pawn, then let WanaWorks read controller readiness, behavior structure, animation blueprint, identity, relationship, and WIT context before it optimizes anything."),
                    StudioAccentColor,
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                    [
                        MakeStringPickerControl(
                            LOCTEXT("WanaWorksStudioAIOnlyPickerLabel", "AI Pawn Asset"),
                            Args.AIPawnAssetOptions,
                            Args.GetSelectedAIPawnAssetOption,
                            Args.OnAIPawnAssetOptionSelected,
                            LOCTEXT("WanaWorksStudioAIOnlyPickerDefault", "(Choose AI Pawn)"),
                            260.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAISelectedSubjectCardTitle", "AI Subject"),
                            LOCTEXT("WanaWorksStudioAISelectedSubjectCardEyebrow", "NPC / AI PAWN"),
                            Args.GetSubjectSetupSummaryText,
                            StudioAccentColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIStackCardTitle", "Detected AI Stack"),
                            LOCTEXT("WanaWorksStudioAIStackCardEyebrow", "CONTROLLER / BT / BLACKBOARD"),
                            Args.GetSubjectStackSummaryText,
                            FLinearColor(0.18f, 0.60f, 0.67f, 1.0f),
                            7,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIBehaviorStructureCardTitle", "Behavior Structure"),
                            LOCTEXT("WanaWorksStudioAIBehaviorStructureCardEyebrow", "BT / STATE TREE / BLACKBOARD"),
                            Args.GetSubjectStackSummaryText,
                            StudioAccentBlueColor,
                            5,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeCharacterIntelligenceControlCard(Args)
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.56f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    WorkspaceLabel,
                    LOCTEXT("WanaWorksStudioAIStageEyebrow", "NPC INTELLIGENCE WORKSPACE"),
                    LOCTEXT("WanaWorksStudioAIStageTitle", "AI Subject Preview"),
                    LOCTEXT("WanaWorksStudioAIStageDescription", "Preview the selected NPC while WanaWorks evaluates controller readiness, behavior structure, animation hooks, physical response, WAI/WAMI identity, WAY-lite relationship context, and WIT environment awareness before making non-destructive improvements."),
                    {
                        LOCTEXT("WanaWorksStudioAIStageChipController", "AI Controller"),
                        LOCTEXT("WanaWorksStudioAIStageChipBehavior", "BT / State Tree"),
                        LOCTEXT("WanaWorksStudioAIStageChipOne", "WAI / WAMI"),
                        LOCTEXT("WanaWorksStudioAIStageChipTwo", "WanaAnimation-lite"),
                        LOCTEXT("WanaWorksStudioAIStageChipThree", "WanaCombat-lite"),
                        LOCTEXT("WanaWorksStudioAIStageChipFour", "WIT World Meaning")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.24f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIAnimationStatusTitle", "Animation Readiness"),
                            LOCTEXT("WanaWorksStudioAIAnimationStatusEyebrow", "WANANIMATION LAYER"),
                        Args.GetAnimationIntegrationText,
                        StudioSuccessColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIPhysicalStatusTitle", "Physical State"),
                            LOCTEXT("WanaWorksStudioAIPhysicalStatusEyebrow", "WANACOMBAT-LITE READINESS"),
                        Args.GetPhysicalStateText,
                        StudioAccentBlueColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIBehaviorStatusTitle", "Behavior Results"),
                            LOCTEXT("WanaWorksStudioAIBehaviorStatusEyebrow", "AI DIAGNOSIS"),
                        Args.GetBehaviorResultsText,
                        StudioSuccessColor,
                        8,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIEnvironmentStatusTitle", "Environment Awareness"),
                            LOCTEXT("WanaWorksStudioAIEnvironmentStatusEyebrow", "WIT SEMANTIC SUPPORT"),
                        Args.GetWITEnvironmentReadinessText,
                        FLinearColor(0.24f, 0.74f, 0.78f, 1.0f),
                        8,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAISuggestedImprovementsTitle", "Suggested Improvements"),
                            LOCTEXT("WanaWorksStudioAISuggestedImprovementsEyebrow", "ANALYZE BEFORE OPTIMIZE"),
                        Args.GetEnhancementResultsText,
                        StudioAccentGoldColor,
                        7,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioWorkflowStrip(Args, true)
        ];
}

TSharedRef<SWidget> BuildCharacterBuildingWorkspaceBody(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString WorkspaceLabel(TEXT("Character Building"));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.20f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioCharacterLeftEyebrow", "CHARACTER BUILD"),
                    LOCTEXT("WanaWorksStudioCharacterLeftTitle", "Character Asset Context"),
                    LOCTEXT("WanaWorksStudioCharacterLeftDescription", "Prepare a playable character or character asset as a clean build target: Blueprint subject, skeletal mesh, skeleton or rig, animation blueprint, movement/playability readiness, profile, and final output."),
                    StudioAccentGoldColor,
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                    [
                        MakeStringPickerControl(
                            LOCTEXT("WanaWorksStudioCharacterOnlyPickerLabel", "Character Blueprint / Pawn"),
                            Args.CharacterPawnAssetOptions,
                            Args.GetSelectedCharacterPawnAssetOption,
                            Args.OnCharacterPawnAssetOptionSelected,
                            LOCTEXT("WanaWorksStudioCharacterOnlyPickerDefault", "(Choose Character Blueprint)"),
                            260.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterSelectedSubjectTitle", "Character Subject"),
                            LOCTEXT("WanaWorksStudioCharacterSelectedSubjectEyebrow", "BLUEPRINT / PAWN"),
                            Args.GetSubjectSetupSummaryText,
                            StudioAccentGoldColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterStackTitle", "Character Asset Stack"),
                            LOCTEXT("WanaWorksStudioCharacterStackEyebrow", "MESH / SKELETON / COMPONENTS"),
                            Args.GetSubjectStackSummaryText,
                            FLinearColor(0.18f, 0.56f, 0.72f, 1.0f),
                            7,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterRigAnimationTitle", "Rig & Animation Readiness"),
                            LOCTEXT("WanaWorksStudioCharacterRigAnimationEyebrow", "SKELETON / ANIM BP"),
                            Args.GetAnimationIntegrationText,
                            StudioSuccessColor,
                            6,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterIdentityFocusTitle", "Character Profile"),
                            LOCTEXT("WanaWorksStudioCharacterIdentityFocusEyebrow", "IDENTITY / AUTHORING"),
                            Args.GetIdentitySummaryText,
                            StudioAccentGoldColor,
                            6,
                            188.0f)
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.57f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    WorkspaceLabel,
                    LOCTEXT("WanaWorksStudioCharacterStageEyebrow", "PLAYABLE CHARACTER WORKSPACE"),
                    LOCTEXT("WanaWorksStudioCharacterStageTitle", "Character Build Preview"),
                    LOCTEXT("WanaWorksStudioCharacterStageDescription", "Preview the selected character as an authored asset while WanaWorks checks mesh, skeleton/rig, animation blueprint, movement component, camera/control assumptions, character systems, and clean final output readiness."),
                    {
                        LOCTEXT("WanaWorksStudioCharacterStageChipOne", "Skeletal Mesh"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipTwo", "Skeleton / Rig"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipThree", "Animation Blueprint"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipFour", "Movement Component"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipControl", "Camera / Control"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipFive", "Build Output")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.23f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterMovementRightTitle", "Movement / Playability"),
                        LOCTEXT("WanaWorksStudioCharacterMovementRightEyebrow", "CONTROL READINESS"),
                        Args.GetPhysicalStateText,
                        StudioAccentBlueColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterSystemsRightTitle", "Character Systems"),
                        LOCTEXT("WanaWorksStudioCharacterSystemsRightEyebrow", "COMPONENT READINESS"),
                        Args.GetCharacterEnhancementSummaryText,
                        StudioSuccessColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterCameraControlTitle", "Camera / Control Setup"),
                        LOCTEXT("WanaWorksStudioCharacterCameraControlEyebrow", "PLAYABILITY CONTEXT"),
                        []()
                        {
                            return LOCTEXT("WanaWorksStudioCharacterCameraControlSummary", "Camera Setup: Preserved from existing Blueprint\nControl Setup: Existing input/controller path preserved\nPlayability Note: WanaWorks reports readiness without replacing project input, camera, or controller architecture.");
                        },
                        FLinearColor(0.36f, 0.50f, 0.86f, 1.0f),
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterBuildRightTitle", "Build Readiness"),
                        LOCTEXT("WanaWorksStudioCharacterBuildRightEyebrow", "ANALYZE BEFORE BUILD"),
                        Args.GetEnhancementResultsText,
                        StudioAccentGoldColor,
                        6,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterOutputRightTitle", "Output Status"),
                        LOCTEXT("WanaWorksStudioCharacterOutputRightEyebrow", "FINAL ASSET"),
                        Args.GetSavedSubjectProgressText,
                        StudioSuccessColor,
                        7,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioWorkflowStrip(Args, false)
        ];
}

TSharedRef<SWidget> MakeLevelWorkspaceWorkflowStrip(const FWanaWorksUITabBuilderArgs& Args)
{
    const FLinearColor LevelAccentColor = GetWorkspaceAccentColor(TEXT("Level Design"));

    const TSharedRef<SWidget> FlowBadge = WanaWorksUIStyle::WanaStatusPill(
        LOCTEXT("WanaWorksStudioLevelWorkflowBadge", "LEVEL DESIGN"),
        LevelAccentColor,
        true,
        8,
        FMargin(12.0f, 6.0f));

    const TSharedRef<SWidget> WorkflowTiles =
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("1"),
                LOCTEXT("WanaWorksStudioLevelWorkflowEnhanceTile", "Enhance"),
                LOCTEXT("WanaWorksStudioLevelWorkflowEnhanceText", "Prepare the semantic world context so level-facing meaning can augment the active UE5-native character stack without polluting Character Intelligence."),
                LevelAccentColor,
                Args.OnEnhanceWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("2"),
                LOCTEXT("WanaWorksStudioLevelWorkflowTestTile", "Test"),
                LOCTEXT("WanaWorksStudioLevelWorkflowTestText", "Run a level-facing pass so the current subject, space meaning, and scene context stay readable inside the world workspace."),
                StudioAccentBlueColor,
                Args.OnTestWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 12.0f, 0.0f)
        [
            MakeWorkflowTile(
                TEXT("3"),
                LOCTEXT("WanaWorksStudioLevelWorkflowAnalyzeTile", "Analyze"),
                LOCTEXT("WanaWorksStudioLevelWorkflowAnalyzeText", "Use WIT semantic world understanding to inspect cover meaning, obstacle meaning, movement-space limits, and environment readiness."),
                LevelAccentColor,
                Args.OnAnalyzeWorkspace)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            MakeWorkflowTile(
                TEXT("4"),
                LOCTEXT("WanaWorksStudioLevelWorkflowBuildTile", "Build"),
                LOCTEXT("WanaWorksStudioLevelWorkflowBuildText", "Save a clean output quietly while the world-stage workspace remains in focus and the underlying project architecture stays untouched."),
                StudioAccentGoldColor,
                Args.OnFinalizeSandboxBuild)
        ];

    return WanaWorksUIStyle::WanaCard(
        LOCTEXT("WanaWorksStudioLevelWorkflowEyebrow", "LEVEL FLOW"),
        LOCTEXT("WanaWorksStudioLevelWorkflowTitle", "Enhance. Test. Analyze. Build."),
        LevelAccentColor,
        WorkflowTiles,
        FlowBadge,
        true);
}

TSharedRef<SWidget> BuildLevelDesignWorkspaceBody(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.23f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioLevelLeftEyebrow", "LEVEL CONTEXT"),
                    LOCTEXT("WanaWorksStudioLevelLeftTitle", "Semantic World Workspace"),
                    LOCTEXT("WanaWorksStudioLevelLeftDescription", "Level Design now owns the scene-side tools and WIT concepts that do not belong on the Character Intelligence face. Here, WIT means semantic world understanding: cover meaning, obstacle meaning, movement meaning, and environment context."),
                    GetWorkspaceAccentColor(TEXT("Level Design")),
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioLevelWorldCardTitle", "Semantic World Model"),
                            LOCTEXT("WanaWorksStudioLevelWorldCardEyebrow", "WIT"),
                            Args.GetWITEnvironmentReadinessText,
                            GetWorkspaceAccentColor(TEXT("Level Design")),
                            6,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioLevelContextCardTitle", "Workspace Status"),
                            LOCTEXT("WanaWorksStudioLevelContextCardEyebrow", "SCENE"),
                            Args.GetStatusText,
                            StudioAccentBlueColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioLevelSceneContextTitle", "Scene Context"),
                            LOCTEXT("WanaWorksStudioLevelSceneContextEyebrow", "ENVIRONMENT LANE"),
                            []()
                            {
                                return LOCTEXT("WanaWorksStudioLevelSceneContextText", "Mode: Semantic world understanding\nFocus: cover, obstacle, boundary, and movement-space meaning\nPrimary action: Analyze through WIT\nUtility tools: secondary and hidden from the main studio face");
                            },
                            GetWorkspaceAccentColor(TEXT("Level Design")),
                            5,
                            188.0f)
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.52f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    TEXT("Level Design"),
                    LOCTEXT("WanaWorksStudioLevelStageEyebrow", "WORLD ORCHESTRATION LAYER"),
                    LOCTEXT("WanaWorksStudioLevelStageTitle", "Semantic Environment Stage"),
                    LOCTEXT("WanaWorksStudioLevelStageDescription", "This stage presents the subject inside scene context so world meaning, cover meaning, boundary pressure, preview focus, and level-side output all feel like one orchestration surface instead of a utility drawer."),
                    {
                        LOCTEXT("WanaWorksStudioLevelStageChipOne", "Cover Meaning"),
                        LOCTEXT("WanaWorksStudioLevelStageChipTwo", "Obstacle Meaning"),
                        LOCTEXT("WanaWorksStudioLevelStageChipThree", "Scene Context"),
                        LOCTEXT("WanaWorksStudioLevelStageChipFour", "WIT World Sense")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.25f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightWITTitle", "Environment Meaning"),
                        LOCTEXT("WanaWorksStudioLevelRightWITEyebrow", "LIVE WIT"),
                        Args.GetWITEnvironmentReadinessText,
                        GetWorkspaceAccentColor(TEXT("Level Design")),
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightPreviewTitle", "Active Stage Subject"),
                        LOCTEXT("WanaWorksStudioLevelRightPreviewEyebrow", "PREVIEW"),
                        Args.GetSandboxPreviewSummaryText,
                        StudioAccentBlueColor,
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightStatusTitle", "Live Workspace Status"),
                        LOCTEXT("WanaWorksStudioLevelRightStatusEyebrow", "STATUS"),
                        Args.GetStatusText,
                        StudioSuccessColor,
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightSavedTitle", "Saved Progress"),
                        LOCTEXT("WanaWorksStudioLevelRightSavedEyebrow", "PROJECT"),
                        Args.GetSavedSubjectProgressText,
                        StudioAccentGoldColor,
                        4,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeLevelWorkspaceWorkflowStrip(Args)
        ];
}

TSharedRef<SWidget> BuildWorkspaceBodyForLabel(const FWanaWorksUITabBuilderArgs& Args, const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return BuildCharacterIntelligenceWorkspaceBody(Args);
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return BuildCharacterBuildingWorkspaceBody(Args);
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return BuildLevelDesignWorkspaceBody(Args);
    }

    return BuildFutureWorkspacePlaceholder(Args, WorkspaceLabel);
}

class SWanaWorksWorkspaceBodySwitcher : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksWorkspaceBodySwitcher)
    {
    }
        SLATE_ARGUMENT(FWanaWorksUITabBuilderArgs, BuilderArgs)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        BuilderArgs = InArgs._BuilderArgs;

        ChildSlot
        [
            SAssignNew(BodyBox, SBox)
        ];

        RefreshBody();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        const FString CurrentWorkspaceLabel = GetCurrentWorkspaceLabel();

        if (!CurrentWorkspaceLabel.Equals(CachedWorkspaceLabel, ESearchCase::IgnoreCase))
        {
            RefreshBody();
        }
    }

private:
    FString GetCurrentWorkspaceLabel() const
    {
        return BuilderArgs.GetSelectedWorkspaceLabel
            ? BuilderArgs.GetSelectedWorkspaceLabel()
            : FString(TEXT("AI"));
    }

    void RefreshBody()
    {
        CachedWorkspaceLabel = GetCurrentWorkspaceLabel();

        if (BodyBox.IsValid())
        {
            BodyBox->SetContent(BuildWorkspaceBodyForLabel(BuilderArgs, CachedWorkspaceLabel));
        }
    }

    FWanaWorksUITabBuilderArgs BuilderArgs;
    TSharedPtr<SBox> BodyBox;
    FString CachedWorkspaceLabel;
};

FString GetSelectedWorkspaceLabelOrDefault(TFunction<FString(void)> GetSelectedWorkspaceLabel)
{
    return GetSelectedWorkspaceLabel
        ? GetSelectedWorkspaceLabel()
        : FString(TEXT("AI"));
}

TSharedRef<SWidget> MakeTopSearchSurface()
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioAccentBlueColor.CopyWithNewOpacity(0.30f))
        [
            SNew(SBorder)
            .Padding(FMargin(16.0f, 11.0f))
            .BorderBackgroundColor(StudioGlassSurfaceColor.CopyWithNewOpacity(0.76f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    WanaWorksUIStyle::WanaStatusPill(
                        LOCTEXT("WanaWorksTopSearchMode", "SEARCH"),
                        StudioAccentBlueColor,
                        false,
                        7,
                        FMargin(9.0f, 4.0f))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                .Padding(11.0f, 0.0f, 8.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Regular", 10))
                    .ColorAndOpacity(SecondaryTextColor.CopyWithNewOpacity(0.92f))
                    .Text(LOCTEXT("WanaWorksTopSearchPlaceholder", "Search WanaWorks studio, subjects, systems..."))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBorder)
                    .Padding(FMargin(7.0f, 3.0f))
                    .BorderBackgroundColor(FLinearColor(0.012f, 0.020f, 0.052f, 0.96f))
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 7))
                        .ColorAndOpacity(TertiaryTextColor.CopyWithNewOpacity(0.86f))
                        .Text(LOCTEXT("WanaWorksTopSearchShortcut", "CTRL K"))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeTopWorkspaceHeader(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
        {
            return GetWorkspaceAccentColor(GetSelectedWorkspaceLabelOrDefault(GetSelectedWorkspaceLabel)).CopyWithNewOpacity(0.38f);
        })
        [
            SNew(SBorder)
            .Padding(FMargin(16.0f, 10.0f))
            .BorderBackgroundColor(StudioSoftSurfaceColor.CopyWithNewOpacity(0.90f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Fill)
                .Padding(0.0f, 2.0f, 13.0f, 2.0f)
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                    {
                        return GetWorkspaceAccentColor(GetSelectedWorkspaceLabelOrDefault(GetSelectedWorkspaceLabel)).CopyWithNewOpacity(0.78f);
                    })
                    [
                        SNew(SBox)
                        .WidthOverride(3.0f)
                    ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 7))
                        .ColorAndOpacity(TertiaryTextColor.CopyWithNewOpacity(0.90f))
                        .Text(LOCTEXT("WanaWorksTopWorkspaceEyebrow", "ACTIVE WORKSPACE"))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Font(MakeStudioFont("Bold", 17))
                        .ColorAndOpacity(FLinearColor::White)
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                        {
                            return GetWorkspaceHeroTitle(GetSelectedWorkspaceLabelOrDefault(GetSelectedWorkspaceLabel));
                        })
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 8))
                        .ColorAndOpacity_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                        {
                            return GetWorkspaceAccentColor(GetSelectedWorkspaceLabelOrDefault(GetSelectedWorkspaceLabel)).CopyWithNewOpacity(0.92f);
                        })
                        .Text_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                        {
                            return GetWorkspaceHeroBadge(GetSelectedWorkspaceLabelOrDefault(GetSelectedWorkspaceLabel));
                        })
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeTopCommandCenter(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioAccentColor.CopyWithNewOpacity(0.34f))
        [
            SNew(SBorder)
            .Padding(FMargin(15.0f, 11.0f))
            .BorderBackgroundColor(FLinearColor(0.018f, 0.026f, 0.066f, 0.98f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 8))
                    .ColorAndOpacity(StudioAccentColor.CopyWithNewOpacity(0.92f))
                    .Text(LOCTEXT("WanaWorksTopCommandLabel", "COMMAND CENTER"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor.CopyWithNewOpacity(0.92f))
                    .Text_Lambda([GetCommandText = Args.GetCommandText]()
                    {
                        const FText CommandText = GetCommandText ? GetCommandText() : FText::GetEmpty();
                        return CommandText.IsEmpty()
                            ? LOCTEXT("WanaWorksTopCommandPlaceholder", "Ready for orchestration commands")
                            : CommandText;
                    })
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeTopStatusModule(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioSuccessColor.CopyWithNewOpacity(0.32f))
        [
            SNew(SBorder)
            .Padding(FMargin(14.0f, 10.0f))
            .BorderBackgroundColor(FLinearColor(0.016f, 0.032f, 0.054f, 0.98f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 10.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor(StudioSuccessColor.CopyWithNewOpacity(0.92f))
                    [
                        SNew(SBox)
                        .WidthOverride(7.0f)
                        .HeightOverride(7.0f)
                    ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 7))
                        .ColorAndOpacity(TertiaryTextColor.CopyWithNewOpacity(0.86f))
                        .Text(LOCTEXT("WanaWorksTopStatusLabel", "SYSTEM STATE"))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Font(MakeStudioFont("Bold", 9))
                        .ColorAndOpacity(FLinearColor(0.92f, 1.0f, 0.96f, 1.0f))
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text_Lambda([GetStatusText = Args.GetStatusText]()
                        {
                            return GetStatusText ? GetStatusText() : FText::GetEmpty();
                        })
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioTopChrome(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
        [
            SNew(SBorder)
            .Padding(FMargin(18.0f, 14.0f))
            .BorderBackgroundColor(FLinearColor(0.006f, 0.012f, 0.034f, 0.992f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(0.30f)
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 14.0f, 0.0f)
                [
                    MakeTopSearchSurface()
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.32f)
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 14.0f, 0.0f)
                [
                    MakeTopWorkspaceHeader(Args)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.22f)
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 14.0f, 0.0f)
                [
                    MakeTopCommandCenter(Args)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.16f)
                .VAlign(VAlign_Center)
                [
                    MakeTopStatusModule(Args)
                ]
            ]
        ];
}
}

namespace WanaWorksUITabBuilder
{
TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(20.0f)
        .BorderBackgroundColor(StudioShellColor)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 20.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(296.0f)
                [
                    MakeStudioNavigationRail(Args)
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    MakeStudioTopChrome(Args)
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SBorder)
                    .Padding(1.0f)
                    .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.50f))
                    [
                        SNew(SBorder)
                        .Padding(20.0f)
                        .BorderBackgroundColor(StudioShellRaisedColor.CopyWithNewOpacity(0.998f))
                        [
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    [
                                        MakeDynamicWorkspacePill(Args.GetSelectedWorkspaceLabel)
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 12.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .Font(MakeStudioFont("Bold", 26))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .ShadowColorAndOpacity(StudioShadowColor)
                                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                                        .Text_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                                        {
                                            const FString WorkspaceLabel = GetSelectedWorkspaceLabel
                                                ? GetSelectedWorkspaceLabel()
                                                : FString(TEXT("AI"));
                                            return GetWorkspaceHeroTitle(WorkspaceLabel);
                                        })
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .AutoWrapText(true)
                                        .Font(MakeStudioFont("Regular", 10))
                                        .ColorAndOpacity(SecondaryTextColor)
                                        .Text_Lambda([GetSelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel]()
                                        {
                                            const FString WorkspaceLabel = GetSelectedWorkspaceLabel
                                                ? GetSelectedWorkspaceLabel()
                                                : FString(TEXT("AI"));
                                            return GetWorkspaceHeroSubtitle(WorkspaceLabel);
                                        })
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                                    [
                                        MakeStudioDivider(1.0f, StudioAccentColor.CopyWithNewOpacity(0.34f))
                                    ]
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                                [
                                    SNew(SWanaWorksWorkspaceBodySwitcher)
                                    .BuilderArgs(Args)
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
}
}

#undef LOCTEXT_NAMESPACE
