// Logical HUD groups, derived from the owner's USA widget bridge observations.
// OpenAI Codex for szampan. No game XML/artwork or generated code is embedded.
#pragma once
#include <string>
#include <string_view>
#include <atomic>
namespace sims2::ui {
inline bool edge_anchors=true;
inline std::atomic<bool> single_player_hud{false};
// Only the observed one-player groups are eligible. Unknown/centered screens
// and 4:3 pass through unchanged. Returns whether a coordinate was changed.
bool PatchLayout(std::string& xml, std::string_view operation, bool wide,bool single_player=false);
// Restart the existing dialog a0 initializer only inside its ready geometry
// publication. No hard-coded height, asset rewrite or world state change.
bool RefreshContinue(std::string& xml,std::string_view operation,bool geometry_ready);
}
