#pragma once

#include <string>
#include <utility>
#include <vector>

namespace WanaMemory
{
namespace Json
{

struct Value
{
    enum class Kind
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    Kind Type = Kind::Null;
    bool Bool = false;
    double Number = 0.0;
    std::string String;
    std::vector<Value> Array;
    std::vector<std::pair<std::string, Value>> Object;

    const Value* Find(const std::string& Key) const;
    bool Has(const std::string& Key) const;
    std::string GetString(const std::string& Key, const std::string& Fallback = std::string()) const;
    double GetNumber(const std::string& Key, double Fallback = 0.0) const;
    bool GetBool(const std::string& Key, bool Fallback = false) const;
    void Set(const std::string& Key, Value Child);
};

bool Parse(const std::string& Text, Value& OutValue, std::string& OutError);
std::string Stringify(const Value& Input);
std::string Quote(const std::string& Text);

Value MakeString(std::string Text);
Value MakeNumber(double Number);
Value MakeBool(bool Bool);
Value MakeObject();
Value MakeArray();

} // namespace Json
} // namespace WanaMemory
