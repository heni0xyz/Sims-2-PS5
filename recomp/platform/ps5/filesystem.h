#pragma once

namespace sims2::ps5 {
// Supported title pathname/name limits; errors retain errno semantics.
long PathConf(const char* path,int query);
}
