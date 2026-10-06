#include "WanaMemoryTypes.h"

#include "WanaMemoryUtil.h"

#include <cctype>

namespace WanaMemory
{

Status ValidatePair(const std::string& CharacterId, const std::string& PlayerId)
{
    if (Trim(CharacterId).empty() || Trim(PlayerId).empty())
    {
        return Status::Fail("character id and player id are required");
    }
    if (CharacterId.size() > static_cast<std::size_t>(kMaxIdChars) || PlayerId.size() > static_cast<std::size_t>(kMaxIdChars))
    {
        return Status::Fail("character id and player id must be 256 characters or fewer");
    }
    return Status::Ok();
}

std::string NormalizeStateKind(const std::string& StateKind, Status& OutStatus)
{
    const std::string Trimmed = Trim(StateKind);
    if (Trimmed.empty() || Trimmed.size() > 32)
    {
        OutStatus = Status::Fail("identity state kind must be 1 to 32 characters");
        return std::string();
    }
    std::string Upper;
    Upper.reserve(Trimmed.size());
    for (unsigned char Char : Trimmed)
    {
        if (!std::isalnum(Char) && Char != '_')
        {
            OutStatus = Status::Fail("identity state kind may contain only letters, digits, and underscore");
            return std::string();
        }
        Upper.push_back(static_cast<char>(std::toupper(Char)));
    }
    OutStatus = Status::Ok();
    return Upper;
}

bool IsAllowedTurnRole(const std::string& Role)
{
    return Role == "user" || Role == "assistant" || Role == "system" || Role == "tool";
}

} // namespace WanaMemory
