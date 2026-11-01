// Harness-only host environment adapters; real Windows kernel APIs stay intact.
// OpenAI Codex for szampan.
#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdlib>
inline int setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && std::getenv(name)) return 0;
    return _putenv_s(name,value);
}
inline int Sims2HeadlessMessageBoxW(HWND,const wchar_t* text,const wchar_t* title,UINT) {
    std::fprintf(stderr,"[HEADLESS][NATIVE_MESSAGE] %ls: %ls\n",title,text);
    return 0;
}
#define MessageBoxW Sims2HeadlessMessageBoxW
