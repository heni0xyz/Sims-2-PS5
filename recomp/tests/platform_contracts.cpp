// Asset-free compatibility checks, OpenAI Codex for szampan.
#include "contracts.h"
#include "cpu/guest_address.h"
#include "kernel/kernel.h"
#include <cstdio>
#include <limits>
#include <stdexcept>

static unsigned checks = 0;
static void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}

int main() {
    using namespace sims2::ps5;
    try {
        constexpr uintptr_t base = 0x200000000ull;
        uint32_t guest = 0;
        uintptr_t native = 0;
        Check(ToGuest(base,base,guest) && guest==0,"guest zero offset");
        Check(ToGuest(base,base+0xFFFFFFFFull,guest) && guest==0xFFFFFFFFu,"guest high pointer");
        Check(!ToGuest(base,base-1,guest),"reject native below reservation");
        Check(!ToGuest(base,base+0x100000000ull,guest),"reject truncating 64-bit pointer");
        Check(ToNative(base,0xFFFFFFFFu,1,native) && native==base+0xFFFFFFFFull,"last guest byte");
        Check(!ToNative(base,0xFFFFFFFFu,2,native),"reject guest range wrap");
        Check(!ToNative(UINTPTR_MAX-3,8,1,native),"reject native pointer wrap");
        Check(ToNative(base,0x10000,4096,native) && native==base+0x10000,"low guest allocation span");
        Check(x86_memory_address(0xF0000000)==0x80000000,"write-combined RAM alias");
        Check(x86_memory_address(0xF3FFFFFF)==0x83FFFFFF,"last RAM alias byte");
        Check(x86_memory_address(0xF4000000)==0xF4000000,"do not normalize outside RAM");
        Check(x86_memory_address(0xD0001000)==0xD0001000,"guest stack address retained");

        std::string path;
        Check(TranslatePath("D:\\DATA\\..\\movies\\intro.xmv","/assets","/hdd",path) && path=="/assets/movies/intro.xmv","disc path normalization");
        Check(TranslatePath("u:\\save.dat","/assets","/hdd",path) && path=="/hdd/U/save.dat","save mount separation");
        Check(!TranslatePath("T:\\..\\save.dat","/assets","/hdd",path),"reject save traversal");
        Check(!TranslatePath("D:\\..\\file","/assets","/hdd",path),"reject disc traversal");
        Check(!TranslatePath("C:\\file","/assets","/hdd",path),"reject unsupported drive");
        Check(!TranslatePath(std::string("D:\\a\0b",6),"/assets","/hdd",path),"reject embedded NUL");

        PadSample sample{};
        Check(!MapPad(sample).connected,"disconnected pad");
        sample.connected=true;sample.held=0x4000|0x2000|0x8000|0x1000|0x10|0x8;
        sample.lx=-2;sample.ly=-1;sample.rx=2;sample.ry=1;sample.l2=.5f;sample.r2=2;
        auto pad=MapPad(sample);
        Check(pad.connected && pad.buttons==0x11,"DualSense dpad and options");
        Check(pad.analog[0]==255 && pad.analog[1]==255 && pad.analog[2]==255 && pad.analog[3]==255,"DualSense positional face mapping");
        Check(pad.lx==-32768 && pad.ly==32767 && pad.rx==32767 && pad.ry==-32768,"stick orientation and clamping");
        Check(pad.analog[6]==128 && pad.analog[7]==255,"trigger scaling");
        sample.lx=std::numeric_limits<float>::quiet_NaN();sample.l2=sample.lx;
        pad=MapPad(sample);Check(pad.lx==0 && pad.analog[6]==0,"invalid pad sample neutralized");
        sample.held=0x80000000u;Check(!MapPad(sample).connected,"intercepted sample excluded");

        X86Context context{};
        context.esp=0xD0001000;context.ebx=0x12345678;
        kernel::Return(context,0x80070002,3);
        Check(context.esp==0xD0001010 && context.eax==0x80070002 && context.ebx==0x12345678,"stdcall return and three-argument cleanup");
        Check(alignof(X86Context)==64,"native guest CPU context alignment");
        std::printf("{\"status\":\"PASS\",\"checks\":%u,\"scope\":\"host contracts; no PS5 execution\"}\n",checks);
        return 0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"FAIL after %u checks: %s\n",checks,error.what());
        return 1;
    }
}
