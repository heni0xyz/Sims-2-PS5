#include "platform/title.h"
#include "services.h"
#include "app/app.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <atomic>
#include <exception>
#include <memory>
#include <pthread.h>
#if defined(__PROSPERO__)
#include <ps5platform/libc.h>
#else
// Host regression supplies this exact public SDK interface.
extern "C" [[noreturn]] void ps5_pthread_exit(void*);
#endif

// These are the same exported calls and declarations as the template's
// ps5/src/platform.c. The template still owns the CRT's catchReturnFromMain.
extern "C" int sceSystemServiceLoadExec(const char*, const char* const*);
extern "C" int sceKernelUsleep(uint32_t);

namespace {
[[noreturn]] void WaitForShell() {
    for (;;) sceKernelUsleep(100000);
}
[[noreturn]] void ExitThroughShell() {
    std::fflush(nullptr);
    const int result=sceSystemServiceLoadExec("exit",nullptr);
    if (result<0) sims2::ps5::Log("CRASH","ERROR","Shell exit request failed");
    WaitForShell();
}
}
namespace title_platform {
[[noreturn]] void ExitThread() {ps5_pthread_exit(nullptr);}
void StartThread(std::function<void()> routine) {
    auto owned=std::make_unique<std::function<void()>>(std::move(routine));
    pthread_attr_t attributes;
    int error=pthread_attr_init(&attributes);
    if (error) Fatal("pthread_attr_init failed");
    error=pthread_attr_setstacksize(&attributes,64u<<20);
    if (error) {pthread_attr_destroy(&attributes);Fatal("64 MiB native stack request failed");}
    pthread_t thread;
    // The established RADV recipe wraps pthread_create with direct-memory stacks.
    error=pthread_create(&thread,&attributes,[](void* argument)->void* {
        std::unique_ptr<std::function<void()>> function(static_cast<std::function<void()>*>(argument));
        try {(*function)();}
        catch (const std::exception& exception) {Fatal(exception.what());}
        // pthread_exit/cancellation may use a foreign forced unwind; let it pass.
        return nullptr;
    },owned.get());
    pthread_attr_destroy(&attributes);
    if (error) {
        char message[128];std::snprintf(message,sizeof(message),"pthread_create failed with code %d",error);
        Fatal(message);
    }
    owned.release();
    if (pthread_detach(thread)) Fatal("pthread_detach failed");
}
void GuestStarting(uint32_t address) {
    static std::atomic<bool> reported{false};
    if (!reported.exchange(true))
        sims2::ps5::Log("GAME","INFO","Calling recompiled guest entry","GUEST_ENTRY",address);
}
[[noreturn]] void Shutdown(int code) {
    sims2::ps5::StopAudio();
    sims2::ps5::Log("SHUTDOWN",code==0?"INFO":"ERROR","Guest requested title shutdown",
                   code==0?"SHUTDOWN":"");
    ExitThroughShell();
}
[[noreturn]] void Fatal(const char* message,uint32_t guest,uintptr_t native) {
    sims2::ps5::Log("CRASH","ERROR",message?message:"Fatal runtime failure","",guest,native);
    ExitThroughShell();
}
[[noreturn]] void Relaunch(const char* executable) {
    if (!executable || !*executable) Fatal("Missing native relaunch executable");
    bool known=false;
    for (const auto& image:app::kGame.images)
        known |= std::strcmp(executable,image.second)==0;
    if (!known) Fatal("Relaunch target is not in the configured image mapping");
    for (const char* p=executable;*p;p++)
        if (!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='_'||*p=='-'))
            Fatal("Invalid native relaunch executable name");
    const std::string path=app::kGame.images.size()==1?"/app0/eboot.bin":std::string("/app0/")+executable+".bin";
    const char* arguments[]={"--relaunched",nullptr};
    sims2::ps5::Log("BOOT","INFO","Replacing title through LoadExec");
    std::fflush(nullptr);
    if (sceSystemServiceLoadExec(path.c_str(),arguments)<0) Fatal("Native LoadExec relaunch failed");
    WaitForShell();
}
void ThreadName(const char* name) {
    // The SDK has no verified POSIX thread-renaming import for a title. Keep
    // the diagnostic identity without introducing a possibly-null import.
    sims2::ps5::Log("THREAD","INFO",name?name:"unnamed thread");
}
}
