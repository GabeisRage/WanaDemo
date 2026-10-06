#pragma once

#include <string>
#include <vector>

namespace WanaMemory
{

/* Local embedding providers only. Do not implement this with a vendor
   embeddings API: that would send studio memory off the machine. */
class IEmbeddingProvider
{
public:
    virtual ~IEmbeddingProvider() = default;
    virtual std::string Name() const = 0;
    virtual int Dimension() const = 0;
    virtual bool Embed(const std::string& Text, std::vector<float>& OutVector) = 0;
};

class LocalHashEmbedding final : public IEmbeddingProvider
{
public:
    static constexpr int kDimension = 64;

    std::string Name() const override;
    int Dimension() const override;
    bool Embed(const std::string& Text, std::vector<float>& OutVector) override;
};

double CosineSimilarity(const std::vector<float>& Left, const std::vector<float>& Right);

} // namespace WanaMemory
