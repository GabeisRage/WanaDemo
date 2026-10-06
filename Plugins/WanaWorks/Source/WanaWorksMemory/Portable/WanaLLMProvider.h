#pragma once

#include "WanaMemoryCallback.h"
#include "WanaMemoryTypes.h"

namespace WanaMemory
{

/* Stateless chat/completions provider. Implementations must not create
   vendor-side threads, assistants, files, or stored conversations. */
class ILLMProvider
{
public:
    virtual ~ILLMProvider() = default;
    virtual std::string ProviderId() const = 0;
    virtual void CompleteAsync(const LLMRequest& Request, ResultCallback<LLMResult> Callback) = 0;
};

} // namespace WanaMemory
