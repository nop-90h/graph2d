#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

#ifdef NDEBUG

#define NO_LOG

#endif //NDEBUG

#define NO_LOG

class CLogger
{
private:
    static CLogger* _instance;
public:
                        CLogger         (void);
    static CLogger*     getInstance     (void);
    void                trace           (const char*        lpccTraceStr);
    void                alert           (const char*        lpccTraceStr);
};

#ifdef NO_LOG

#define LOG_TRACE(s)                 ((void)0)
#define LOG_TRACE_FMT(s,...)         ((void)0)
#define LOG_ALERT(s)                 ((void)0)
#define LOG_ALERT_FMT(s,...)         ((void)0)

#define LOG_TRACE_CH(ch, s) CLogger::getInstance()->trace(ch, s)
#define LOG_TRACE_CH_FMT(ch, s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->trace(ch, lt); };

//#define LOG_TRACE_CH(ch, s)          ((void)0)
//#define LOG_TRACE_CH_FMT(ch, s,...)  ((void)0)
#define LOG_TRACE_ENUM(eVal, s)      ((void)0)
#define UB_LOG_INFO(...)             ((void)0)
#define UB_LOG_WARNING(...)          ((void)0)

#define LOG_TRACE_RND(ch, s) CLogger::getInstance()->trace(s);
#define LOG_TRACE_RND_FMT(s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->trace(lt); };
#else

#define LOG_TRACE(s) CLogger::getInstance()->trace(s)
#define LOG_TRACE_FMT(s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->trace(lt); };
#define LOG_ALERT(s) CLogger::getInstance()->alert(s)
#define LOG_ALERT_FMT(s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->alert(lt); };

#define LOG_TRACE_CH(ch, s) CLogger::getInstance()->trace(ch, s)
#define LOG_TRACE_CH_FMT(ch, s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->trace(ch, lt); };
#define LOG_TRACE_ENUM(eVal, s) CLogger::getInstance()->traceEnum<decltype(eVal)>(eVal, s)
#define LOG_TRACE_RND(ch, s) CLogger::getInstance()->trace(s);
#define LOG_TRACE_RND_FMT(s,...) { char lt[1024] = {0}; sprintf(lt, s, __VA_ARGS__); CLogger::getInstance()->trace(lt); };
#endif //DEBUG

_G2D_NAMESPACE_END_