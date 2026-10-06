#include "WanaMemoryRoundtrip.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWanaMemoryRoundtripAutomation,
    "WanaWorks.Memory.Roundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWanaMemoryRoundtripAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    std::string Report;
    const int Failed = WanaMemory::RunMemoryRoundtripScenario(Report);
    AddInfo(UTF8_TO_TCHAR(Report.c_str()));
    TestEqual(TEXT("WanaWorks memory roundtrip failures"), Failed, 0);
    return Failed == 0;
}

#endif
