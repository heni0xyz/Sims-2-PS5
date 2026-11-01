#include "core/log.h"
#include "services.h"
#include <cstdio>
#include <cstring>

namespace logx {
namespace {std::string path;}
// The template routes stdout/stderr into klog. Avoid a second transport or
// high-frequency writes to the console filesystem; the host collector keeps it.
void Init(const std::string&,const std::string&,bool) {}
const std::string& Path() {return path;}
void Flush() {std::fflush(stderr);}
void Write(Level level,const char* format,...) {
    bool performance=false;
#if defined(SIMS2_PERFORMANCE_LOG)
    performance=level==Level::Info && (!std::strncmp(format,"perf:",5) ||
                                      !std::strncmp(format,"vulkan: present ",16));
#endif
    if (!kVerbose && level<Level::Warn && !performance) return;
    char message[4096];
    va_list args;va_start(args,format);
    std::vsnprintf(message,sizeof(message),format,args);va_end(args);
    const char* names[]={"DEBUG","INFO","WARN","ERROR","INFO"};
    const char* subsystem="XDK";
    if (!std::strncmp(message,"perf:",5)) subsystem="PERF";
    else if (!std::strncmp(message,"vulkan:",7)) subsystem="VULKAN";
    else if (!std::strncmp(message,"memory:",7)) subsystem="MEM";
    else if (!std::strncmp(message,"thread",6)) subsystem="THREAD";
    else if (!std::strncmp(message,"boot:",5)) subsystem="BOOT";
    sims2::ps5::Log(subsystem,names[int(level)],message);
}
}
