#include "WanaMemoryJson.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace WanaMemory
{
namespace Json
{
namespace
{

class Parser
{
public:
    explicit Parser(const std::string& InText)
        : Text(InText)
    {
    }

    bool ParseRoot(Value& OutValue, std::string& OutError)
    {
        Skip();
        if (!ParseValue(OutValue))
        {
            OutError = Error.empty() ? "invalid JSON" : Error;
            return false;
        }
        Skip();
        if (Index != Text.size())
        {
            OutError = "unexpected trailing data in JSON";
            return false;
        }
        OutError.clear();
        return true;
    }

private:
    const std::string& Text;
    std::size_t Index = 0;
    int Depth = 0;
    std::string Error;

    void Fail(const char* Message)
    {
        if (Error.empty())
        {
            Error = Message;
        }
    }

    void Skip()
    {
        while (Index < Text.size() && std::isspace(static_cast<unsigned char>(Text[Index])))
        {
            ++Index;
        }
    }

    bool Consume(char Expected)
    {
        Skip();
        if (Index < Text.size() && Text[Index] == Expected)
        {
            ++Index;
            return true;
        }
        return false;
    }

    bool ParseValue(Value& OutValue)
    {
        if (Depth > 32)
        {
            Fail("JSON is nested too deeply");
            return false;
        }
        Skip();
        if (Index >= Text.size())
        {
            Fail("unexpected end of JSON");
            return false;
        }
        const char Char = Text[Index];
        if (Char == '{')
        {
            return ParseObject(OutValue);
        }
        if (Char == '[')
        {
            return ParseArray(OutValue);
        }
        if (Char == '"')
        {
            return ParseString(OutValue);
        }
        if (Char == 't' || Char == 'f')
        {
            return ParseBool(OutValue);
        }
        if (Char == 'n')
        {
            return ParseNull(OutValue);
        }
        if (Char == '-' || std::isdigit(static_cast<unsigned char>(Char)))
        {
            return ParseNumber(OutValue);
        }
        Fail("unexpected JSON value");
        return false;
    }

    bool ParseObject(Value& OutValue)
    {
        if (!Consume('{'))
        {
            Fail("expected object");
            return false;
        }
        OutValue = MakeObject();
        ++Depth;
        Skip();
        if (Consume('}'))
        {
            --Depth;
            return true;
        }
        while (Index < Text.size())
        {
            Value Key;
            if (!ParseString(Key))
            {
                return false;
            }
            if (!Consume(':'))
            {
                Fail("expected ':' in object");
                return false;
            }
            Value Child;
            if (!ParseValue(Child))
            {
                return false;
            }
            OutValue.Set(Key.String, std::move(Child));
            Skip();
            if (Consume('}'))
            {
                --Depth;
                return true;
            }
            if (!Consume(','))
            {
                Fail("expected ',' or '}' in object");
                return false;
            }
        }
        Fail("unterminated object");
        return false;
    }

    bool ParseArray(Value& OutValue)
    {
        if (!Consume('['))
        {
            Fail("expected array");
            return false;
        }
        OutValue = MakeArray();
        ++Depth;
        Skip();
        if (Consume(']'))
        {
            --Depth;
            return true;
        }
        while (Index < Text.size())
        {
            Value Child;
            if (!ParseValue(Child))
            {
                return false;
            }
            OutValue.Array.push_back(std::move(Child));
            Skip();
            if (Consume(']'))
            {
                --Depth;
                return true;
            }
            if (!Consume(','))
            {
                Fail("expected ',' or ']' in array");
                return false;
            }
        }
        Fail("unterminated array");
        return false;
    }

    bool ParseString(Value& OutValue)
    {
        Skip();
        if (Index >= Text.size() || Text[Index] != '"')
        {
            Fail("expected string");
            return false;
        }
        ++Index;
        std::string Decoded;
        while (Index < Text.size())
        {
            const unsigned char Char = static_cast<unsigned char>(Text[Index++]);
            if (Char == '"')
            {
                OutValue = MakeString(std::move(Decoded));
                return true;
            }
            if (Char == '\\')
            {
                if (Index >= Text.size())
                {
                    Fail("unterminated escape");
                    return false;
                }
                const char Escaped = Text[Index++];
                switch (Escaped)
                {
                case '"':
                case '\\':
                case '/':
                    Decoded.push_back(Escaped);
                    break;
                case 'b':
                    Decoded.push_back('\b');
                    break;
                case 'f':
                    Decoded.push_back('\f');
                    break;
                case 'n':
                    Decoded.push_back('\n');
                    break;
                case 'r':
                    Decoded.push_back('\r');
                    break;
                case 't':
                    Decoded.push_back('\t');
                    break;
                case 'u':
                    if (!AppendUnicode(Decoded))
                    {
                        return false;
                    }
                    break;
                default:
                    Fail("invalid JSON escape");
                    return false;
                }
            }
            else if (Char < 0x20)
            {
                Fail("raw control character in JSON string");
                return false;
            }
            else
            {
                Decoded.push_back(static_cast<char>(Char));
            }
        }
        Fail("unterminated string");
        return false;
    }

    bool AppendUnicode(std::string& Out)
    {
        if (Index + 4 > Text.size())
        {
            Fail("short unicode escape");
            return false;
        }
        unsigned Code = 0;
        for (int Digit = 0; Digit < 4; ++Digit)
        {
            const unsigned char Char = static_cast<unsigned char>(Text[Index++]);
            Code <<= 4;
            if (Char >= '0' && Char <= '9')
            {
                Code += static_cast<unsigned>(Char - '0');
            }
            else if (Char >= 'a' && Char <= 'f')
            {
                Code += static_cast<unsigned>(Char - 'a' + 10);
            }
            else if (Char >= 'A' && Char <= 'F')
            {
                Code += static_cast<unsigned>(Char - 'A' + 10);
            }
            else
            {
                Fail("invalid unicode escape");
                return false;
            }
        }
        if (Code >= 0xD800 && Code <= 0xDFFF)
        {
            Out.push_back('?');
            return true;
        }
        if (Code <= 0x7F)
        {
            Out.push_back(static_cast<char>(Code));
        }
        else if (Code <= 0x7FF)
        {
            Out.push_back(static_cast<char>(0xC0 | (Code >> 6)));
            Out.push_back(static_cast<char>(0x80 | (Code & 0x3F)));
        }
        else
        {
            Out.push_back(static_cast<char>(0xE0 | (Code >> 12)));
            Out.push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3F)));
            Out.push_back(static_cast<char>(0x80 | (Code & 0x3F)));
        }
        return true;
    }

    bool ParseBool(Value& OutValue)
    {
        if (Text.compare(Index, 4, "true") == 0)
        {
            Index += 4;
            OutValue = MakeBool(true);
            return true;
        }
        if (Text.compare(Index, 5, "false") == 0)
        {
            Index += 5;
            OutValue = MakeBool(false);
            return true;
        }
        Fail("invalid boolean");
        return false;
    }

    bool ParseNull(Value& OutValue)
    {
        if (Text.compare(Index, 4, "null") == 0)
        {
            Index += 4;
            OutValue = Value();
            return true;
        }
        Fail("invalid null");
        return false;
    }

    bool ParseNumber(Value& OutValue)
    {
        const std::size_t Start = Index;
        if (Text[Index] == '-')
        {
            ++Index;
        }
        if (Index >= Text.size() || !std::isdigit(static_cast<unsigned char>(Text[Index])))
        {
            Fail("invalid number");
            return false;
        }
        while (Index < Text.size() && std::isdigit(static_cast<unsigned char>(Text[Index])))
        {
            ++Index;
        }
        if (Index < Text.size() && Text[Index] == '.')
        {
            ++Index;
            if (Index >= Text.size() || !std::isdigit(static_cast<unsigned char>(Text[Index])))
            {
                Fail("invalid number");
                return false;
            }
            while (Index < Text.size() && std::isdigit(static_cast<unsigned char>(Text[Index])))
            {
                ++Index;
            }
        }
        if (Index < Text.size() && (Text[Index] == 'e' || Text[Index] == 'E'))
        {
            ++Index;
            if (Index < Text.size() && (Text[Index] == '+' || Text[Index] == '-'))
            {
                ++Index;
            }
            if (Index >= Text.size() || !std::isdigit(static_cast<unsigned char>(Text[Index])))
            {
                Fail("invalid number");
                return false;
            }
            while (Index < Text.size() && std::isdigit(static_cast<unsigned char>(Text[Index])))
            {
                ++Index;
            }
        }
        const std::string Slice = Text.substr(Start, Index - Start);
        char* End = nullptr;
        const double Number = std::strtod(Slice.c_str(), &End);
        if (!End || End == Slice.c_str())
        {
            Fail("invalid number");
            return false;
        }
        OutValue = MakeNumber(Number);
        return true;
    }
};

void StringifyInto(const Value& Input, std::string& Out)
{
    switch (Input.Type)
    {
    case Value::Kind::Null:
        Out += "null";
        break;
    case Value::Kind::Bool:
        Out += Input.Bool ? "true" : "false";
        break;
    case Value::Kind::Number:
    {
        char Buffer[64];
        if (std::isfinite(Input.Number) && std::floor(Input.Number) == Input.Number && std::fabs(Input.Number) < 1.0e15)
        {
            std::snprintf(Buffer, sizeof(Buffer), "%.0f", Input.Number);
        }
        else
        {
            std::snprintf(Buffer, sizeof(Buffer), "%.8g", Input.Number);
        }
        Out += Buffer;
        break;
    }
    case Value::Kind::String:
        Out += Quote(Input.String);
        break;
    case Value::Kind::Array:
        Out.push_back('[');
        for (std::size_t Index = 0; Index < Input.Array.size(); ++Index)
        {
            if (Index != 0)
            {
                Out.push_back(',');
            }
            StringifyInto(Input.Array[Index], Out);
        }
        Out.push_back(']');
        break;
    case Value::Kind::Object:
        Out.push_back('{');
        for (std::size_t Index = 0; Index < Input.Object.size(); ++Index)
        {
            if (Index != 0)
            {
                Out.push_back(',');
            }
            Out += Quote(Input.Object[Index].first);
            Out.push_back(':');
            StringifyInto(Input.Object[Index].second, Out);
        }
        Out.push_back('}');
        break;
    }
}

} // namespace

const Value* Value::Find(const std::string& Key) const
{
    if (Type != Kind::Object)
    {
        return nullptr;
    }
    for (const std::pair<std::string, Value>& Pair : Object)
    {
        if (Pair.first == Key)
        {
            return &Pair.second;
        }
    }
    return nullptr;
}

bool Value::Has(const std::string& Key) const
{
    return Find(Key) != nullptr;
}

std::string Value::GetString(const std::string& Key, const std::string& Fallback) const
{
    const Value* Child = Find(Key);
    if (!Child || Child->Type != Kind::String)
    {
        return Fallback;
    }
    return Child->String;
}

double Value::GetNumber(const std::string& Key, double Fallback) const
{
    const Value* Child = Find(Key);
    if (!Child || Child->Type != Kind::Number)
    {
        return Fallback;
    }
    return Child->Number;
}

bool Value::GetBool(const std::string& Key, bool Fallback) const
{
    const Value* Child = Find(Key);
    if (!Child || Child->Type != Kind::Bool)
    {
        return Fallback;
    }
    return Child->Bool;
}

void Value::Set(const std::string& Key, Value Child)
{
    Type = Kind::Object;
    for (std::pair<std::string, Value>& Pair : Object)
    {
        if (Pair.first == Key)
        {
            Pair.second = std::move(Child);
            return;
        }
    }
    Object.emplace_back(Key, std::move(Child));
}

bool Parse(const std::string& Text, Value& OutValue, std::string& OutError)
{
    Parser Local(Text);
    return Local.ParseRoot(OutValue, OutError);
}

std::string Stringify(const Value& Input)
{
    std::string Out;
    StringifyInto(Input, Out);
    return Out;
}

std::string Quote(const std::string& Text)
{
    std::string Out;
    Out.push_back('"');
    for (unsigned char Char : Text)
    {
        switch (Char)
        {
        case '"':
            Out += "\\\"";
            break;
        case '\\':
            Out += "\\\\";
            break;
        case '\b':
            Out += "\\b";
            break;
        case '\f':
            Out += "\\f";
            break;
        case '\n':
            Out += "\\n";
            break;
        case '\r':
            Out += "\\r";
            break;
        case '\t':
            Out += "\\t";
            break;
        default:
            if (Char < 0x20)
            {
                char Buffer[8];
                std::snprintf(Buffer, sizeof(Buffer), "\\u%04x", Char);
                Out += Buffer;
            }
            else
            {
                Out.push_back(static_cast<char>(Char));
            }
            break;
        }
    }
    Out.push_back('"');
    return Out;
}

Value MakeString(std::string Text)
{
    Value Result;
    Result.Type = Value::Kind::String;
    Result.String = std::move(Text);
    return Result;
}

Value MakeNumber(double Number)
{
    Value Result;
    Result.Type = Value::Kind::Number;
    Result.Number = Number;
    return Result;
}

Value MakeBool(bool Bool)
{
    Value Result;
    Result.Type = Value::Kind::Bool;
    Result.Bool = Bool;
    return Result;
}

Value MakeObject()
{
    Value Result;
    Result.Type = Value::Kind::Object;
    return Result;
}

Value MakeArray()
{
    Value Result;
    Result.Type = Value::Kind::Array;
    return Result;
}

} // namespace Json
} // namespace WanaMemory
