#include "WanaMemoryUtil.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#  include <direct.h>
#endif

namespace WanaMemory
{
namespace
{

bool IsStopToken(const std::string& Token)
{
    static const char* Stop[] = {
        "the", "and", "you", "for", "that", "with", "this", "your",
        "are", "was", "from", "have", "has", "but", "not", "its",
        "she", "him", "her", "they", "them", "just", "about"
    };
    for (const char* Word : Stop)
    {
        if (Token == Word)
        {
            return true;
        }
    }
    return false;
}

bool MakeOneDirectory(const std::string& Path)
{
    if (Path.empty() || Path == "." || Path == "/")
    {
        return true;
    }
#if defined(_WIN32)
    if (Path.size() == 2 && Path[1] == ':')
    {
        return true;
    }
    const int Result = _mkdir(Path.c_str());
#else
    const int Result = mkdir(Path.c_str(), 0755);
#endif
    if (Result == 0)
    {
        return true;
    }
    struct stat Info;
    if (stat(Path.c_str(), &Info) == 0 && (Info.st_mode & S_IFDIR))
    {
        return true;
    }
    return false;
}

} // namespace

std::string UtcNow()
{
    const std::time_t Now = std::time(nullptr);
    std::tm Utc{};
#if defined(_WIN32)
    gmtime_s(&Utc, &Now);
#else
    gmtime_r(&Now, &Utc);
#endif
    char Buffer[32];
    if (std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%dT%H:%M:%SZ", &Utc) == 0)
    {
        return std::string("1970-01-01T00:00:00Z");
    }
    return std::string(Buffer);
}

bool EnsureParentDirectory(const std::string& FilePath)
{
    if (FilePath.empty() || FilePath == ":memory:")
    {
        return true;
    }
    const std::size_t Slash = FilePath.find_last_of("/\\");
    if (Slash == std::string::npos)
    {
        return true;
    }
    const std::string Directory = FilePath.substr(0, Slash);
    if (Directory.empty())
    {
        return true;
    }

    std::string Current;
    std::size_t Index = 0;
    if (Directory[0] == '/' || Directory[0] == '\\')
    {
        Current.push_back(Directory[0]);
        Index = 1;
    }
#if defined(_WIN32)
    if (Directory.size() >= 2 && Directory[1] == ':')
    {
        Current = Directory.substr(0, 2);
        Index = 2;
        if (Directory.size() > 2 && (Directory[2] == '/' || Directory[2] == '\\'))
        {
            Current.push_back(Directory[2]);
            Index = 3;
        }
    }
#endif
    while (Index < Directory.size())
    {
        const std::size_t Next = Directory.find_first_of("/\\", Index);
        const std::string Part = Directory.substr(Index, Next == std::string::npos ? std::string::npos : Next - Index);
        if (!Part.empty() && Part != ".")
        {
            if (!Current.empty() && Current.back() != '/' && Current.back() != '\\')
            {
                Current.push_back('/');
            }
            Current += Part;
            if (!MakeOneDirectory(Current))
            {
                return false;
            }
        }
        if (Next == std::string::npos)
        {
            break;
        }
        Index = Next + 1;
    }
    return true;
}

bool ReadEntireFile(const std::string& Path, std::string& OutText, std::string& OutError)
{
    std::ifstream Stream(Path.c_str(), std::ios::in | std::ios::binary);
    if (!Stream)
    {
        OutError = "could not read " + Path;
        return false;
    }
    std::ostringstream Buffer;
    Buffer << Stream.rdbuf();
    if (!Stream && !Stream.eof())
    {
        OutError = "could not read " + Path;
        return false;
    }
    OutText = Buffer.str();
    OutError.clear();
    return true;
}

bool WriteEntireFile(const std::string& Path, const std::string& Text, std::string& OutError)
{
    if (!EnsureParentDirectory(Path))
    {
        OutError = "could not create the directory for " + Path;
        return false;
    }
    std::ofstream Stream(Path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!Stream)
    {
        OutError = "could not write " + Path;
        return false;
    }
    Stream.write(Text.data(), static_cast<std::streamsize>(Text.size()));
    if (!Stream)
    {
        OutError = "could not write " + Path;
        return false;
    }
    OutError.clear();
    return true;
}

std::vector<std::string> Tokenize(const std::string& Text)
{
    std::vector<std::string> Tokens;
    std::string Current;
    const auto Flush = [&Tokens, &Current]()
    {
        if (Current.size() >= 3 && !IsStopToken(Current))
        {
            bool bSeen = false;
            for (const std::string& Existing : Tokens)
            {
                if (Existing == Current)
                {
                    bSeen = true;
                    break;
                }
            }
            if (!bSeen)
            {
                Tokens.push_back(Current);
            }
        }
        Current.clear();
    };

    for (unsigned char Char : Text)
    {
        if (std::isalnum(Char))
        {
            Current.push_back(static_cast<char>(std::tolower(Char)));
        }
        else
        {
            Flush();
        }
    }
    Flush();
    return Tokens;
}

std::string JoinTokens(const std::vector<std::string>& Tokens)
{
    std::string Joined;
    for (std::size_t Index = 0; Index < Tokens.size(); ++Index)
    {
        if (Index != 0)
        {
            Joined.push_back(' ');
        }
        Joined += Tokens[Index];
    }
    return Joined;
}

} // namespace WanaMemory
