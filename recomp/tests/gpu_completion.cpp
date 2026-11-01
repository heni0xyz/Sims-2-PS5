// Asset-free contract test of the actual threaded backend completion queue.
#include "gpu/backend.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "core/log.h"
#include <array>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <vector>
namespace {std::array<uint8_t,4*1024*1024> arena{};}
namespace kernel {uint8_t* g_base=arena.data();}
namespace mem {
uint32_t AllocSystem(uint32_t size,uint32_t align) {static uint32_t next=4096;next=(next+align-1)&~(align-1);const auto result=next;next+=size;if(next>arena.size())throw std::bad_alloc();return result;}
uint32_t VirtualFromPhysical(uint32_t address) {return address;}
}
namespace logx {void Write(Level,const char*,...) {}}
struct Gate {std::mutex mutex;std::condition_variable ready;bool entered=false,open=false;unsigned completed=0;std::vector<unsigned> order;};
struct Fake final:gpu::Backend {
    Gate& gate;bool fail=false;explicit Fake(Gate& g):gate(g){}
    bool Init(uint32_t,uint32_t) override{return true;}
    void Draw(const gpu::State&,uint32_t,uint32_t,uint32_t,const uint8_t*) override{}
    void Clear(const gpu::State&,uint32_t,const uint8_t*,uint32_t,uint32_t,float,uint32_t) override{}
    void Copy(const gpu::Surface&,const gpu::Surface&,uint32_t,uint32_t,uint32_t) override{}
    void Present(const gpu::Surface&) override{}
    void Sync(uint32_t,uint32_t) override{}
    void SyncLate(uint32_t,uint32_t) override{}
    void SetGamma(const uint8_t*) override{}
    void Flush() override {
        if(fail)throw std::runtime_error("synthetic failed GPU completion");
        std::unique_lock lock(gate.mutex);gate.entered=true;gate.ready.notify_all();
        gate.ready.wait(lock,[&]{return gate.open;});gate.completed++;
    }
};
void Require(bool value,const char* reason){if(!value){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
int main() {
    Gate gate;
    auto recording=gpu::CreateThreadedBackend(std::make_unique<Fake>(gate));
    auto callback=[&](unsigned index){
        return [&,index]{std::lock_guard lock(gate.mutex);Require(gate.completed>=index,"GPU completes before callback");gate.order.push_back(index);gate.ready.notify_all();};
    };
    recording->QueueCompletion(callback(1));
    {
        std::unique_lock lock(gate.mutex);
        Require(gate.ready.wait_for(lock,std::chrono::seconds(2),[&]{return gate.entered;}),"worker reaches GPU wait");
        Require(gate.order.empty(),"pending GPU cannot deliver callback");
    }
    // The producer remains free to record while completion 1 waits on the GPU.
    recording->QueueCompletion(callback(2));
    {std::lock_guard lock(gate.mutex);gate.open=true;gate.ready.notify_all();}
    recording->Flush();
    {
        std::lock_guard lock(gate.mutex);
        Require(gate.order==std::vector<unsigned>({1,2}),"ordered delivery across queued batches");
    }
    for(unsigned i=3;i<=34;i++)recording->QueueCompletion(callback(i));
    recording->Flush();
    {
        std::lock_guard lock(gate.mutex);
        Require(gate.order.size()==34,"all callbacks delivered exactly once after batch reuse");
        for(unsigned i=0;i<gate.order.size();i++)Require(gate.order[i]==i+1,"FIFO order survives batch reuse");
    }
    Fake failed(gate);failed.fail=true;bool called=false,threw=false;
    try{failed.QueueCompletion([&]{called=true;});}catch(const std::runtime_error&){threw=true;}
    Require(threw && !called,"failed completion never dispatches callback");
    recording.reset();
    std::puts("PASS_GPU_COMPLETION: nonblocking recording, actual worker FIFO, GPU gate, exactly-once batch reuse, fail-closed default");
}
