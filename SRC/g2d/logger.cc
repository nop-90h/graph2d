#include "logger.h"

_G2D_NAMESPACE_BEGIN_

#ifdef TARGET_EMSCRIPTEN
EM_JS(void, console_log, (const char* lpccLogText), {
    const log_str = UTF8ToString(lpccLogText);
    console.log(log_str);
});

EM_JS(void, alert_log, (const char* lpccLogText), {
    const log_str = UTF8ToString(lpccLogText);
    alert(log_str);
});
#endif //TARGET_EMSCRIPTEN

CLogger* CLogger::_instance = NULL;

CLogger::CLogger()
{

}
CLogger* CLogger::getInstance()
{
    if (CLogger::_instance == NULL)
        CLogger::_instance = new CLogger();
    return CLogger::_instance;
}

void CLogger::trace(const char* lpccTraceStr)
{
#ifdef TARGET_EMSCRIPTEN
    std::string sLog = lpccTraceStr;
#elif defined TARGET_WIN
    auto const time  = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    std::string sLog = std::format("[{0:%H:%M:%S}] {1}", std::chrono::floor<std::chrono::seconds>(time), lpccTraceStr);
#endif

#ifdef TARGET_EMSCRIPTEN
    console_log(sLog.c_str());
#elif defined TARGET_WIN
    //static RAIIFile f;
    //static bool bFileOpened = false;
    //if (!bFileOpened)
    //{
    //    bFileOpened = true;
    //    int i = 0;
    //    std::string sLogFname;
    //    do
    //    {
    //        i++;
    //        sLogFname = std::format("{:05}_hero_log.log",i);
    //    }
    //    while (std::filesystem::exists(std::format("EMBED/{}", sLogFname)));
    //    f.open(sLogFname.c_str(), "w+");
    //    assert(f.isOpened());
    //}
    //auto sToFile = std::format("{}\n", sLog);

    //f.writeFromString(sToFile);
    std::cout << sLog << std::endl;
#endif //TARGET_EMSCRIPTEN

}

void CLogger::alert(const char* lpccTraceStr)
{
#ifdef TARGET_EMSCRIPTEN
    alert_log(lpccTraceStr);
#else
    //SDL_ShowSimpleMessageBox(0, "Alert", lpccTraceStr, nullptr);
#endif //TARGET_EMSCRIPTEN
}

_G2D_NAMESPACE_END_