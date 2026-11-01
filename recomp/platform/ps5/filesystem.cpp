#include "filesystem.h"
#include <cerrno>
#include <climits>
#include <sys/stat.h>
#include <unistd.h>

namespace sims2::ps5 {
long PathConf(const char* path,int query) {
    // libc++ 18's filesystem::current_path asks for _PC_PATH_MAX before
    // getcwd. A title cannot import pathconf from PosixForWebKit. These two
    // kernel pathname limits come from the target SDK's sys/syslimits.h;
    // do not report success for a missing path or an unsupported query.
    if (!path) {errno=EFAULT;return -1;}
    if (query!=_PC_PATH_MAX && query!=_PC_NAME_MAX) {errno=ENOTSUP;return -1;}
    struct stat information{};
    if (stat(path,&information)!=0) return -1;
    return query==_PC_PATH_MAX?PATH_MAX:NAME_MAX;
}
}

#if defined(__PROSPERO__)
// Local definition, not an exported override of a console system module.
extern "C" __attribute__((visibility("hidden"))) long pathconf(const char* path,int query) {
    return sims2::ps5::PathConf(path,query);
}
#endif
