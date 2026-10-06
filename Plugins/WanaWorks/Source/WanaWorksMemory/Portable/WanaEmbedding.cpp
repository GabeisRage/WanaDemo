#include "WanaEmbedding.h"

#include "WanaMemoryUtil.h"

#include <cmath>

namespace WanaMemory
{
namespace
{

std::uint32_t HashToken(const std::string& Token, std::uint32_t Seed)
{
    std::uint32_t Hash = Seed;
    for (unsigned char Char : Token)
    {
        Hash ^= static_cast<std::uint32_t>(Char);
        Hash *= 16777619u;
    }
    return Hash;
}

} // namespace

std::string LocalHashEmbedding::Name() const
{
    return "local-hash";
}

int LocalHashEmbedding::Dimension() const
{
    return kDimension;
}

bool LocalHashEmbedding::Embed(const std::string& Text, std::vector<float>& OutVector)
{
    OutVector.assign(static_cast<std::size_t>(kDimension), 0.0f);
    const std::vector<std::string> Tokens = Tokenize(Text);
    if (Tokens.empty())
    {
        return false;
    }
    for (const std::string& Token : Tokens)
    {
        const std::uint32_t Bucket = HashToken(Token, 2166136261u);
        const std::uint32_t SignHash = HashToken(Token, 17u);
        const float Sign = (SignHash & 1u) ? 1.0f : -1.0f;
        OutVector[Bucket % static_cast<std::uint32_t>(kDimension)] += Sign;
    }
    double SumSquares = 0.0;
    for (float Value : OutVector)
    {
        SumSquares += static_cast<double>(Value) * static_cast<double>(Value);
    }
    if (SumSquares <= 0.0)
    {
        return false;
    }
    const float Inv = static_cast<float>(1.0 / std::sqrt(SumSquares));
    for (float& Value : OutVector)
    {
        Value *= Inv;
    }
    return true;
}

double CosineSimilarity(const std::vector<float>& Left, const std::vector<float>& Right)
{
    if (Left.empty() || Left.size() != Right.size())
    {
        return 0.0;
    }
    double Dot = 0.0;
    double LeftNorm = 0.0;
    double RightNorm = 0.0;
    for (std::size_t Index = 0; Index < Left.size(); ++Index)
    {
        const double A = Left[Index];
        const double B = Right[Index];
        Dot += A * B;
        LeftNorm += A * A;
        RightNorm += B * B;
    }
    if (LeftNorm <= 0.0 || RightNorm <= 0.0)
    {
        return 0.0;
    }
    return Dot / std::sqrt(LeftNorm * RightNorm);
}

} // namespace WanaMemory
