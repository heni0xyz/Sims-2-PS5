#include "kernel/wake_stats.h"
#include "core/deep_profile.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>
#include <queue>
#include <span>

#include "kernel/kernel.h"
#include "kernel/handle_cache.h"

namespace kernel {

namespace {

std::mutex g_dispatch;
std::condition_variable g_cv;
// State/predicates and registration stay under g_dispatch. Each sleeping wait
// owns a stack registration and uses its host thread's one reusable channel.
// List writes hold BOTH g_dispatch and g_wait_registry; object notifications
// read under g_dispatch, generic/APC broadcasts read under g_wait_registry.
// Generic notifications never acquire g_dispatch: QueueApc may hold apc_lock.
size_t g_sleeping_waiters = 0;
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
struct WaitRegistration;
std::mutex g_wait_registry;
WaitRegistration* g_waiters=nullptr;
thread_local std::condition_variable g_wait_channel;
bool SelectiveWaitsEnabled() noexcept {
    static const bool enabled=[] {
        const char* value=std::getenv("XBE_OBJECT_WAIT_CHANNELS");
        return !value || std::strcmp(value,"0")!=0;
    }();
    return enabled;
}
struct WaitRegistration {
    std::span<const ObjectRef> objects;
    std::condition_variable* channel;
    WaitRegistration* previous=nullptr;
    WaitRegistration* next=nullptr;
};
#elif defined(XBE_WAIT_STATS) && XBE_WAIT_STATS
constexpr bool SelectiveWaitsEnabled() noexcept {return false;}
#endif
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
std::atomic<uint64_t> g_registered{0},g_object_signals{0},g_object_notifies{0},g_avoided{0},
    g_broadcast_signals{0},g_broadcast_notifies{0};
void CountBroadcast(uint64_t count) {
    g_broadcast_signals.fetch_add(1,std::memory_order_relaxed);
    g_broadcast_notifies.fetch_add(count,std::memory_order_relaxed);
}
#else
void CountBroadcast(uint64_t) {}
#endif
struct SleepingWaiter {
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
    WaitRegistration registration;
#endif
    explicit SleepingWaiter(std::span<const ObjectRef> objects)
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
        :registration{objects,nullptr,nullptr,nullptr}
#endif
    {
        (void)objects;
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
        if(SelectiveWaitsEnabled()) {
            registration.channel=&g_wait_channel;
            std::lock_guard lock(g_wait_registry);
            registration.next=g_waiters;
            if(g_waiters)g_waiters->previous=&registration;
            g_waiters=&registration;
        }
#endif
        ++g_sleeping_waiters;
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
        g_registered.fetch_add(1,std::memory_order_relaxed);
#endif
    }
    ~SleepingWaiter() {
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
        if(registration.channel) {
            std::lock_guard lock(g_wait_registry);
            if(registration.previous)registration.previous->next=registration.next;
            else g_waiters=registration.next;
            if(registration.next)registration.next->previous=registration.previous;
        }
#endif
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
        g_registered.fetch_sub(1,std::memory_order_relaxed);
#endif
        --g_sleeping_waiters;
    }
    std::condition_variable& Channel() {
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
        if(registration.channel)return *registration.channel;
#endif
        return g_cv;
    }
    SleepingWaiter(const SleepingWaiter&) = delete;
    SleepingWaiter& operator=(const SleepingWaiter&) = delete;
};
void NotifyObject(Object& object) {
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
    if(SelectiveWaitsEnabled()) {
        uint64_t notified=0,total=0;
        // Caller holds g_dispatch: registrations cannot be changed here.
        // Notify every matching registration, preserving wait-any/all choices
        // and multiple consumers, rather than choosing an arbitrary waiter.
        for(auto* waiter=g_waiters;waiter;waiter=waiter->next) {
            ++total;
            for(const auto& ref:waiter->objects)if(ref.get()==&object) {
                waiter->channel->notify_one();++notified;break;
            }
        }
        g_object_signals.fetch_add(1,std::memory_order_relaxed);
        g_object_notifies.fetch_add(notified,std::memory_order_relaxed);
        g_avoided.fetch_add(total-notified,std::memory_order_relaxed);
        return;
    }
#endif
    (void)object;SignalChanged();
}


std::mutex g_handles_lock;
std::unordered_map<uint32_t, ObjectRef> g_handles;
uint32_t g_next_handle = 0x1000;
#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
std::atomic<uint64_t> g_handle_generation{1},g_handle_hits{0},g_handle_misses{0};
bool FastHandlesEnabled() noexcept {
    static const bool enabled=[] {
        const char* value=std::getenv("XBE_FAST_HANDLES");
        return !value || std::strcmp(value,"0")!=0;
    }();
    return enabled;
}
struct HandleCacheEntry {uint32_t handle=0;std::weak_ptr<Object> object;};
struct HandleCache {
    uint64_t generation=0;
    size_t next=0;
    std::array<HandleCacheEntry,8> entries{};
    void Reset(uint64_t value) {
        for(auto& entry:entries) {entry.object.reset();entry.handle=0;}
        generation=value;next=0;
    }
};
thread_local HandleCache g_handle_cache;
#endif


std::mutex g_ptr_lock;
struct PtrEntry {
    ObjectRef obj;
    uint32_t serial;
};
std::unordered_map<uint32_t, PtrEntry> g_by_ptr;
uint32_t g_serial = 1;
constexpr uint32_t kStamp = 0x4A424F4Bu;  // written into WaitListHead.Blink of objects the host tracks

std::mutex g_names_lock;
std::unordered_map<std::string, std::weak_ptr<Object>> g_names;

}  // namespace

std::mutex& DispatchLock() { return g_dispatch; }
void SignalChanged() {
#if defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS
    if(SelectiveWaitsEnabled()) {
        uint64_t count=0;
        std::lock_guard lock(g_wait_registry);
        for(auto* waiter=g_waiters;waiter;waiter=waiter->next) {waiter->channel->notify_one();++count;}
        CountBroadcast(count);return;
    }
#endif
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
    CountBroadcast(g_registered.load(std::memory_order_relaxed));
#else
    CountBroadcast(0);
#endif
    g_cv.notify_all();
}
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
DispatcherWakeStats GetDispatcherWakeStats() noexcept {
    return {SelectiveWaitsEnabled(),g_registered.load(),g_object_signals.load(),g_object_notifies.load(),
        g_avoided.load(),g_broadcast_signals.load(),g_broadcast_notifies.load()};
}
void ReportDispatcherWakePerf() {
    const auto value=GetDispatcherWakeStats();
    LOGI("perf: dispatcher-wakes lifetime selective=%u sleepers=%llu object_signals=%llu object_notifies=%llu avoided=%llu broadcast_signals=%llu broadcast_notifies=%llu",
        unsigned(value.selective),(unsigned long long)value.sleepers,(unsigned long long)value.object_signals,
        (unsigned long long)value.object_notifies,(unsigned long long)value.avoided,
        (unsigned long long)value.broadcast_signals,(unsigned long long)value.broadcast_notifies);
}
#endif

uint32_t NewHandle(ObjectRef obj) {
    std::unique_lock<std::mutex> lock(g_handles_lock,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::HandleLock);lock.lock();}
    uint32_t h = g_next_handle;
    g_next_handle += 4;
    g_handles[h] = std::move(obj);
#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
    // Publication under the map lock also covers a numeric handle being reused.
    g_handle_generation.fetch_add(1,std::memory_order_release);
#endif
    return h;
}

ObjectRef FromHandle(uint32_t handle) {
    if (handle == 0xFFFFFFFE) return CurrentRef();  // NtCurrentThread pseudo handle
#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
    if (FastHandlesEnabled()) {
        HandleCache& cache=g_handle_cache;
        const uint64_t generation=g_handle_generation.load(std::memory_order_acquire);
        if (cache.generation!=generation) cache.Reset(generation);
        for (const auto& entry:cache.entries) {
            if (entry.handle!=handle) continue;
            ObjectRef object=entry.object.lock();
            if (object && g_handle_generation.load(std::memory_order_acquire)==generation) {
                g_handle_hits.fetch_add(1,std::memory_order_relaxed);
                return object;
            }
            break;
        }
        g_handle_misses.fetch_add(1,std::memory_order_relaxed);
        ObjectRef object;
        uint64_t observed;
        {
            std::unique_lock<std::mutex> lock(g_handles_lock,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::HandleLock);lock.lock();}
            auto it=g_handles.find(handle);
            if (it!=g_handles.end()) object=it->second;
            observed=g_handle_generation.load(std::memory_order_relaxed);
        }
        // Store only weak ownership, outside the map lock. CloseHandle still
        // destroys an otherwise unreferenced object immediately.
        if (cache.generation!=observed) cache.Reset(observed);
        if (object) {
            auto& entry=cache.entries[cache.next++%cache.entries.size()];
            entry.handle=handle;entry.object=object;
        }
        return object;
    }
#endif
    std::unique_lock<std::mutex> lock(g_handles_lock,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::HandleLock);lock.lock();}
    auto it = g_handles.find(handle);
    return it == g_handles.end() ? nullptr : it->second;
}

bool CloseHandle(uint32_t handle) {
    std::unique_lock<std::mutex> lock(g_handles_lock,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::HandleLock);lock.lock();}
    const bool closed=g_handles.erase(handle)!=0;
#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
    if (closed) g_handle_generation.fetch_add(1,std::memory_order_release);
#endif
    return closed;
}

#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
HandleCacheStats GetHandleCacheStats() noexcept {
    return {FastHandlesEnabled(),g_handle_hits.load(std::memory_order_relaxed),
        g_handle_misses.load(std::memory_order_relaxed),g_handle_generation.load(std::memory_order_acquire)};
}
void ReportHandleCachePerf() {
    const auto stats=GetHandleCacheStats();
    LOGI("perf: handle-cache lifetime enabled=%u hits=%llu misses=%llu generation=%llu",
        unsigned(stats.enabled),(unsigned long long)stats.hits,(unsigned long long)stats.misses,
        (unsigned long long)stats.generation);
}
#endif

uint32_t DuplicateHandle(uint32_t handle) {
    ObjectRef o = FromHandle(handle);
    return o ? NewHandle(o) : 0;
}

// serial in WaitListHead.Flink, stamp in Blink
static void Stamp(uint32_t guest, uint32_t serial) {
    W32(guest + 8, serial);
    W32(guest + 12, kStamp);
}

void Bind(ObjectRef obj, uint32_t guest) {
    obj->guest = guest;
    std::lock_guard<std::mutex> lock(g_ptr_lock);
    uint32_t serial = g_serial++;
    g_by_ptr[guest] = {obj, serial};
    Stamp(guest, serial);
}

ObjectRef FromPointer(uint32_t guest) {
    if (!guest) return nullptr;
    std::lock_guard<std::mutex> lock(g_ptr_lock);
    auto it = g_by_ptr.find(guest);
    if (it != g_by_ptr.end() && R32(guest + 12) == kStamp && R32(guest + 8) == it->second.serial) return it->second.obj;
    // first sight, or the guest reinitialised the structure: build from the header
    uint8_t type = G<uint8_t>(guest);
    int32_t state = int32_t(R32(guest + 4));
    ObjectRef o;
    switch (type) {
        case 0:
        case 1:
            o = std::make_shared<Object>(ObjType::Event);
            o->notification = type == 0;
            o->signal = state;
            break;
        case 2:
            o = std::make_shared<Object>(ObjType::Mutant);
            o->signal = state;
            break;
        case 4: {  // QueueObject: R12, the packet-carrying port type so NtRemoveIoCompletion/PostIoCompletion accept it
            // packets live host-side, and the port's SignalState must equal its packet count (a Wait on a
            // signalled port with no packet would never block), so it starts empty; entries the guest
            // linked into its own KQUEUE list cannot be carried over
            if (state) LOG_FIRST(8, "objects: guest queue at %08X built with %d entries, starting empty", guest, state);
            o = NewIoCompletion(R32(guest + 0x1C));
            break;
        }
        case 5:
            o = std::make_shared<Object>(ObjType::Semaphore);
            o->signal = state;
            o->limit = int32_t(R32(guest + 0x10));
            break;
        case 8:
        case 9:
            o = std::make_shared<Object>(ObjType::Timer);
            o->notification = type == 8;
            o->signal = state;
            break;
        case 6:
            if (it != g_by_ptr.end()) return it->second.obj;  // threads keep their object
            o = std::make_shared<Object>(ObjType::Other);
            break;
        default:
            LOGW("objects: unknown dispatcher type %u at %08X", type, guest);
            if (Current()) LogGuestStack(*Current()->ctx, 64);
            o = std::make_shared<Object>(ObjType::Event);
            o->notification = true;
            o->signal = state;
            break;
    }
    o->guest = guest;
    uint32_t serial = g_serial++;
    g_by_ptr[guest] = {o, serial};
    Stamp(guest, serial);
    return o;
}
ObjectRef FindNamed(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_names_lock);
    auto it = g_names.find(name);
    if (it != g_names.end())
        if (ObjectRef found = it->second.lock()) return found;
    // the object namespace is case-insensitive (OBJ_CASE_INSENSITIVE): fall back to a folded match
    auto fold = [](std::string s) {
        for (char& c : s) c = char(std::toupper(uint8_t(c)));
        return s;
    };
    std::string want = fold(name);
    for (auto& [key, weak] : g_names)
        if (fold(key) == want)
            if (ObjectRef found = weak.lock()) return found;
    return nullptr;
}

void AddNamed(const std::string& name, ObjectRef obj) {
    std::lock_guard<std::mutex> lock(g_names_lock);
    obj->name = name;
    g_names[name] = obj;
}

void SetSignal(Object& o, int32_t v) {
    o.signal = v;
    if (o.guest) W32(o.guest + 4, uint32_t(v));
}

static uint32_t Me() {
    Thread* t = Current();
    return t ? t->ethread : 1;
}

static bool Satisfied(Object& o, uint32_t me) {
    if (o.type == ObjType::Mutant) return o.signal > 0 || o.owner == me;
    return o.signal > 0;
}

static void Consume(Object& o, uint32_t me) {
    switch (o.type) {
        case ObjType::Event:
        case ObjType::Timer:
            if (!o.notification) SetSignal(o, 0);
            break;
        case ObjType::Semaphore:
            SetSignal(o, o.signal - 1);
            break;
        case ObjType::Mutant:
            if (o.owner == me) {
                o.recursion++;
            } else {
                o.owner = me;
                o.recursion = 1;
                SetSignal(o, 0);
            }
            break;
        default:
            break;
    }
}

int64_t Now100ns() {
    using namespace std::chrono;
    // 1601 to 1970 in 100 ns units
    auto t = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    return t / 100 + 116444736000000000ll;
}

// D13: a span multiplied unchecked wraps a far-future wait (due == INT64_MIN, or anything beyond ~292
// years) into the past and times out at once; saturate the deadline at INT64_MAX instead
static int64_t AddSpan100(int64_t now, int64_t span_100ns) {
    if (span_100ns <= 0) return now;
    if (span_100ns > (INT64_MAX - now) / 100) return INT64_MAX;
    return now + span_100ns * 100;
}

int64_t ToDeadline(int64_t due) {
    using namespace std::chrono;
    int64_t now = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    // negative due is relative, positive is absolute system time
    if (due < 0) return AddSpan100(now, due == INT64_MIN ? INT64_MAX : -due);
    return AddSpan100(now, due - Now100ns());
}

static uint32_t WaitFor(std::span<const ObjectRef> objs, bool wait_all, int64_t timeout, bool alertable) {
    using namespace std::chrono;
    Thread* self = Current();
    uint32_t me = Me();
    bool infinite = timeout == INT64_MAX;
    const bool poll = timeout == 0;
    int64_t deadline = infinite || poll ? 0 : ToDeadline(timeout);
    std::unique_lock<std::mutex> lock(g_dispatch,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::WaitLock);lock.lock();}
    for (;;) {
        if (wait_all) {
            bool all = std::all_of(objs.begin(), objs.end(), [&](const ObjectRef& o) { return o && Satisfied(*o, me); });
            if (all) {
                for (auto& o : objs) Consume(*o, me);
                return kWait0;
            }
        } else {
            for (size_t i = 0; i < objs.size(); i++)
                if (objs[i] && Satisfied(*objs[i], me)) {
                    Consume(*objs[i], me);
                    return kWait0 + uint32_t(i);
                }
        }
        if (alertable && self) {
            bool pending;
            {
                std::lock_guard<std::mutex> al(self->apc_lock);
                pending = !self->apcs.empty();
            }
            if (pending) {
                lock.unlock();
                RunApcs(*self);
                return kUserApc;
            }
        }
        if (!infinite) {
            // Keep ready-object and APC priority above. A zero-timeout poll
            // otherwise always times out, without converting/querying clocks.
            if (poll) return kTimeout;
            int64_t now = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
            if (now >= deadline) return kTimeout;
            // bounded so alertable waits notice new APCs
            SleepingWaiter waiting(objs);
            deep_profile::Scope measured(deep_profile::Metric::WaitCvRelock,objs.empty() || !objs[0]?0:objs[0]->guest);
            waiting.Channel().wait_for(lock, nanoseconds(std::min<int64_t>(deadline - now, 50000000)));
        } else {
            SleepingWaiter waiting(objs);
            deep_profile::Scope measured(deep_profile::Metric::WaitCvRelock,objs.empty() || !objs[0]?0:objs[0]->guest);
            waiting.Channel().wait_for(lock, milliseconds(50));
        }
    }
}

static uint32_t WaitRange(std::span<const ObjectRef> objs, bool wait_all, int64_t timeout, bool alertable) {
    SuspendPoint();
    uint32_t r = WaitFor(objs, wait_all, timeout, alertable);
    SuspendPoint();
    return r;
}

uint32_t Wait(const std::vector<ObjectRef>& objs, bool wait_all, int64_t timeout, bool alertable) {
    return WaitRange(objs, wait_all, timeout, alertable);
}
uint32_t Wait(std::initializer_list<ObjectRef> objs, bool wait_all, int64_t timeout, bool alertable) {
    return WaitRange(std::span<const ObjectRef>(objs.begin(), objs.size()), wait_all, timeout, alertable);
}

int32_t SetEvent(Object& o) {
    std::lock_guard<std::mutex> lock(g_dispatch);
    int32_t old = o.signal;
    SetSignal(o, 1);
    NotifyObject(o);
    return old;
}

int32_t ResetEvent(Object& o) {
    std::lock_guard<std::mutex> lock(g_dispatch);
    int32_t old = o.signal;
    SetSignal(o, 0);
    return old;
}

int32_t PulseEvent(Object& o) {
    // waiters already blocked cannot be told apart here, so a pulse behaves like set for one pass
    std::lock_guard<std::mutex> lock(g_dispatch);
    int32_t old = o.signal;
    SetSignal(o, 1);
    NotifyObject(o);
    return old;
}

int32_t ReleaseSemaphore(Object& o, int32_t count, uint32_t& status) {
    std::unique_lock<std::mutex> lock(g_dispatch,std::defer_lock);
    {deep_profile::Scope measured(deep_profile::Metric::ReleaseLock,o.guest);lock.lock();}
    int32_t old = o.signal;
    if (o.limit && old + count > o.limit) {
        status = kSemaphoreLimitExceeded;
        return old;
    }
    SetSignal(o, old + count);
    status = kSuccess;
    // Nothing can be asleep when the count is zero. Preserve the signal and
    // all semaphore checks, avoiding a host broadcast with no recipient.
    if (g_sleeping_waiters) NotifyObject(o);
    return old;
}

int32_t ReleaseMutant(Object& o, uint32_t& status) {
    std::lock_guard<std::mutex> lock(g_dispatch);
    uint32_t me = Me();
    if (o.owner != me) {
        status = kMutantNotOwned;
        return o.signal;
    }
    int32_t old = o.signal;
    if (--o.recursion == 0) {
        o.owner = 0;
        SetSignal(o, 1);
        NotifyObject(o);
    }
    status = kSuccess;
    return old;
}

// timers and DPCs

namespace {

struct TimerEntry {
    int64_t deadline;
    uint64_t seq;
    ObjectRef timer;
    int32_t period_ms;
    uint32_t dpc;
    std::weak_ptr<Thread> apc_thread;
    uint32_t apc_routine, apc_context;
    bool operator>(const TimerEntry& o) const { return deadline > o.deadline; }
};

std::mutex g_timer_lock;
std::condition_variable g_timer_cv;
std::priority_queue<TimerEntry, std::vector<TimerEntry>, std::greater<TimerEntry>> g_timers;
std::unordered_map<Object*, uint64_t> g_timer_seq;  // latest arm per timer, older entries are stale
uint64_t g_seq = 1;
bool g_timer_thread = false;

std::mutex g_dpc_lock;
std::condition_variable g_dpc_cv;
std::deque<std::array<uint32_t, 3>> g_dpcs;
bool g_dpc_thread = false;

}  // namespace

void RunDpcs(X86Context& ctx) {
    for (;;) {
        std::array<uint32_t, 3> d;
        {
            std::unique_lock<std::mutex> lock(g_dpc_lock);
            g_dpc_cv.wait(lock, [] { return !g_dpcs.empty(); });
            d = g_dpcs.front();
            g_dpcs.pop_front();
        }
        uint32_t dpc = d[0];
        G<uint8_t>(dpc + 2) = 0;  // Inserted
        uint32_t routine = R32(dpc + 0x0C), context = R32(dpc + 0x10);  // KDPC.DeferredRoutine and DeferredContext
        if (routine) CallGuest(ctx, routine, {dpc, context, d[1], d[2]});
    }
}

void QueueDpc(uint32_t dpc, uint32_t arg1, uint32_t arg2) {
    std::lock_guard<std::mutex> lock(g_dpc_lock);
    W32(dpc + 0x14, arg1);  // KDPC.SystemArgument1 and 2
    W32(dpc + 0x18, arg2);
    g_dpcs.push_back({dpc, arg1, arg2});
    g_dpc_cv.notify_one();
    if (!g_dpc_thread) {
        g_dpc_thread = true;
        StartHostThread("dpc", [](X86Context& ctx) { RunDpcs(ctx); });
    }
}

static void TimerLoop() {
    using namespace std::chrono;
    std::unique_lock<std::mutex> lock(g_timer_lock);
    for (;;) {
        if (g_timers.empty()) {
            g_timer_cv.wait(lock);
            continue;
        }
        int64_t now = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
        TimerEntry e = g_timers.top();
        if (e.deadline > now) {
            // D13: a saturated deadline would overflow wait_for's own now + span; the cv is notified on every new timer
            g_timer_cv.wait_for(lock, nanoseconds(std::min<int64_t>(e.deadline - now, 3600ll * 1000000000ll)));
            continue;
        }
        g_timers.pop();
        auto it = g_timer_seq.find(e.timer.get());
        if (it == g_timer_seq.end() || it->second != e.seq) continue;
        {
            std::lock_guard<std::mutex> dl(g_dispatch);
            SetSignal(*e.timer, 1);
            NotifyObject(*e.timer);
        }
        if (e.dpc) QueueDpc(e.dpc, uint32_t(Now100ns()), uint32_t(Now100ns() >> 32));
        if (auto t = e.apc_thread.lock(); t && e.apc_routine)
            QueueApc(*t, e.apc_routine, e.apc_context, uint32_t(Now100ns()), uint32_t(Now100ns() >> 32));
        if (e.period_ms > 0) {
            e.deadline += int64_t(e.period_ms) * 1000000;
            g_timers.push(e);
        } else {
            g_timer_seq.erase(e.timer.get());
        }
    }
}

void SetTimer(ObjectRef timer, int64_t due, int32_t period_ms, uint32_t dpc, std::shared_ptr<Thread> apc_thread,
              uint32_t apc_routine, uint32_t apc_context) {
    std::lock_guard<std::mutex> lock(g_timer_lock);
    if (!g_timer_thread) {
        g_timer_thread = true;
        std::thread(TimerLoop).detach();
    }
    {
        std::lock_guard<std::mutex> dl(g_dispatch);
        SetSignal(*timer, 0);
    }
    uint64_t seq = g_seq++;
    g_timer_seq[timer.get()] = seq;
    g_timers.push({ToDeadline(due), seq, timer, period_ms, dpc, apc_thread, apc_routine, apc_context});
    g_timer_cv.notify_all();
}

bool CancelTimer(Object& timer) {
    std::lock_guard<std::mutex> lock(g_timer_lock);
    return g_timer_seq.erase(&timer) != 0;
}

}  // namespace kernel
