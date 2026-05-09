#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SMultiLineEditableText;
class SScrollBox;

class WANAWORKSUI_API SWanaWorksPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnAnalyzeClicked();
    FReply OnEnhanceClicked();
    FReply OnTestClicked();
    FReply OnBuildClicked();

    void AppendLog(const FString& Line);

    TSharedPtr<SMultiLineEditableText> OutputLog;
    FString LogText;
};
