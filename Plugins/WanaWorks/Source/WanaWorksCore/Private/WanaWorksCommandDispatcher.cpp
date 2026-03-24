#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksCommandRegistry.h"

FWanaCommandResponse FWanaCommandDispatcher::Dispatch(const FWanaCommandRequest& Request)
{
    FWanaCommandResponse Response;

    const FString TrimmedCommand = Request.CommandText.TrimStartAndEnd();

    if (TrimmedCommand.IsEmpty())
    {
        Response.StatusMessage = TEXT("Status: Enter a command to run.");
        Response.OutputLines.Add(TEXT("Run requested with no command."));
        return Response;
    }

    FString CommandName;
    FString CommandArguments;

    if (TrimmedCommand.Split(TEXT(" "), &CommandName, &CommandArguments))
    {
        CommandArguments = CommandArguments.TrimStartAndEnd();
    }
    else
    {
        CommandName = TrimmedCommand;
    }

    CommandName = CommandName.ToLower();

    if (const FWanaCommandDefinition* HelpDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("help")));
        HelpDefinition && TrimmedCommand.Equals(HelpDefinition->CommandText, ESearchCase::IgnoreCase))
    {
        Response.bSucceeded = true;
        Response.StatusMessage = HelpDefinition->StatusMessage;
        Response.OutputLines.Add(TEXT("Available commands:"));

        for (const FWanaCommandDefinition& Definition : WanaWorksCommandRegistry::GetCommandDefinitions())
        {
            if (Definition.bShowInHelp)
            {
                Response.OutputLines.Add(Definition.CommandText);
            }
        }

        return Response;
    }

    if (const FWanaCommandDefinition* ClearDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("clear")));
        ClearDefinition && TrimmedCommand.Equals(ClearDefinition->CommandText, ESearchCase::IgnoreCase))
    {
        Response.bSucceeded = true;
        Response.bClearLog = true;
        Response.StatusMessage = ClearDefinition->StatusMessage;
        return Response;
    }

    if (CommandName == TEXT("echo"))
    {
        if (CommandArguments.IsEmpty())
        {
            Response.StatusMessage = TEXT("Status: echo requires text.");
            Response.OutputLines.Add(TEXT("Usage: echo <text>"));
            return Response;
        }

        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: Echo complete.");
        Response.OutputLines.Add(CommandArguments);
        return Response;
    }

    if (const FWanaCommandDefinition* ApplyRelationshipDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("apply_relationship_state"))))
    {
        static const FString ApplyRelationshipPrefix = TEXT("apply relationship state");

        if (TrimmedCommand.StartsWith(ApplyRelationshipPrefix, ESearchCase::IgnoreCase))
        {
            const FString RelationshipStateArgument = TrimmedCommand.Mid(ApplyRelationshipPrefix.Len()).TrimStartAndEnd();

            if (RelationshipStateArgument.IsEmpty())
            {
                Response.StatusMessage = TEXT("Status: Relationship state is required.");
                Response.OutputLines.Add(FString::Printf(TEXT("Usage: %s"), ApplyRelationshipDefinition->CommandText));
                return Response;
            }

            Response.bSucceeded = true;
            Response.Action = EWanaCommandAction::ApplyRelationshipState;
            Response.ActionArgument = RelationshipStateArgument;
            Response.StatusMessage = ApplyRelationshipDefinition->StatusMessage;
            return Response;
        }
    }

    if (const FWanaCommandDefinition* Definition = WanaWorksCommandRegistry::FindCommandDefinitionByCommandText(TrimmedCommand);
        Definition && Definition->Action != EWanaCommandAction::None)
    {
        Response.bSucceeded = true;
        Response.Action = Definition->Action;
        Response.StatusMessage = Definition->StatusMessage;
        return Response;
    }

    Response.StatusMessage = FString::Printf(TEXT("Status: Unknown command '%s'."), *CommandName);
    Response.OutputLines.Add(FString::Printf(TEXT("Unknown command: %s"), *CommandName));
    Response.OutputLines.Add(TEXT("Type 'help' for available commands."));
    return Response;
}
