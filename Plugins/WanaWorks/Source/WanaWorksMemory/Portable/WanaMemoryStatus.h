#pragma once

#include <string>
#include <utility>

namespace WanaMemory
{

struct Status
{
    bool bOk = true;
    std::string Message;

    static Status Ok()
    {
        return Status();
    }

    static Status Fail(std::string InMessage)
    {
        Status Result;
        Result.bOk = false;
        Result.Message = std::move(InMessage);
        return Result;
    }
};

} // namespace WanaMemory
