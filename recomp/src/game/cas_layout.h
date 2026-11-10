// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// USA native CAS target metadata and authored presentation policy only.
#pragma once
#include "gpu/draw_viewport.h"
#include <cstdint>
#include <string_view>
#include <atomic>
namespace sims2::ui {
inline std::atomic<bool> cas_edge_anchors{true};
inline int CasModalChange(std::string_view operation,std::string_view payload) {
    if(operation=="RemoveScreen" && payload=="0G2D")return -1;
    if(operation=="LoadWidget" && payload.size()<=65536 && payload.starts_with("<scrn>")) {
        const auto id=payload.find("<scrnid>"),end=id==payload.npos?payload.npos:payload.find("</scrnid>",id+8);
        if(end!=payload.npos && payload.substr(id+8,end-id-8)=="0G2D")return 1;
    }
    return 0;
}
inline bool CasEditorScreen(uint32_t screen) {
    // Genetics (3), keyboard (23), family selection (24), loading and unknown
    // states use their own centered layouts. Select the observed editor pages.
    switch(screen) {
        case 4:case 5:case 6:case 8:case 9:case 10:case 11:case 12:case 13:
        case 14:case 16:case 17:case 20:case 21:return true;
        default:return false;
    }
}
inline int CasDrawAnchor(const gpu::DrawBounds &b) {
    if(!std::isfinite(b.left) || !std::isfinite(b.right) || !std::isfinite(b.top) || !std::isfinite(b.bottom) ||
       b.right<=b.left || b.bottom<=b.top)return 0;
    if(b.left>=-64 && b.right<=280 && b.right-b.left<=280)return -1;
    if(b.left>=440 && b.right<=704 && b.right-b.left<=200)return 1;
    return 0;
}
// The original target owns its lifetime and state; renderer threads receive
// only a read-only, validated snapshot. Never tag a guest thread as the UI.
bool CasEditorActive();
void NoteCasModal(std::string_view operation,std::string_view payload);
}
