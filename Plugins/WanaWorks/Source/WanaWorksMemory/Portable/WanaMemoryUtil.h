#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace WanaMemory
{

inline double ClampRange(double Value, double Lo, double Hi)
{
    if (Value < Lo)
    {
        return Lo;
    }
    if (Value > Hi)
    {
        return Hi;
    }
    return Value;
}

inline double Clamp01(double Value)
{
    return ClampRange(Value, 0.0, 1.0);
}

inline double ClampAbs(double Value, double MaxAbs)
{
    return ClampRange(Value, -MaxAbs, MaxAbs);
}

inline bool NearZero(double Value)
{
    return Value < 0.000001 && Value > -0.000001;
}

inline std::string Trim(const std::string& Text)
{
    std::size_t Begin = 0;
    while (Begin < Text.size() && std::isspace(static_cast<unsigned char>(Text[Begin])))
    {
        ++Begin;
    }
    std::size_t End = Text.size();
    while (End > Begin && std::isspace(static_cast<unsigned char>(Text[End - 1])))
    {
        --End;
    }
    return Text.substr(Begin, End - Begin);
}

inline std::string ToLowerAscii(std::string Text)
{
    for (char& Char : Text)
    {
        Char = static_cast<char>(std::tolower(static_cast<unsigned char>(Char)));
    }
    return Text;
}

inline bool EqualsIgnoreCase(const std::string& Left, const std::string& Right)
{
    return ToLowerAscii(Left) == ToLowerAscii(Right);
}

inline bool ContainsIgnoreCase(const std::string& Haystack, const char* Needle)
{
    if (!Needle || Needle[0] == '\0')
    {
        return false;
    }
    const std::string LowerHaystack = ToLowerAscii(Haystack);
    const std::string LowerNeedle = ToLowerAscii(std::string(Needle));
    return LowerHaystack.find(LowerNeedle) != std::string::npos;
}

inline std::string FormatFixed(double Value, int Digits)
{
    char Buffer[64];
    std::snprintf(Buffer, sizeof(Buffer), "%.*f", Digits, Value);
    return std::string(Buffer);
}

inline std::string FormatSigned(double Value)
{
    char Buffer[64];
    std::snprintf(Buffer, sizeof(Buffer), "%+.2f", Value);
    return std::string(Buffer);
}

std::string UtcNow();
bool EnsureParentDirectory(const std::string& FilePath);
bool ReadEntireFile(const std::string& Path, std::string& OutText, std::string& OutError);
bool WriteEntireFile(const std::string& Path, const std::string& Text, std::string& OutError);

std::vector<std::string> Tokenize(const std::string& Text);
std::string JoinTokens(const std::vector<std::string>& Tokens);

} // namespace WanaMemory
