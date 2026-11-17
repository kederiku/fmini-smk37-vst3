#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace bytes {
// Explicit little-endian encoding avoids depending on native struct layout.
inline void u32(std::vector<uint8_t>& out,uint32_t x) { for(int i=0;i<4;i++) out.push_back(static_cast<uint8_t>(x>>(8*i))); }
// FNV-1a detects accidental corruption, not cryptographic authenticity.
inline uint32_t hash(const uint8_t* p,size_t n) { uint32_t h=2166136261u; for(size_t i=0;i<n;i++) h=(h^p[i])*16777619u; return h; }
// Every read checks remaining input before advancing, including nested payloads.
struct Reader {
    const uint8_t* p; size_t left;
    void read(void* out,size_t n) { if(n>left) throw std::runtime_error("Truncated state"); std::memcpy(out,p,n); p+=n; left-=n; }
    uint32_t u32() { uint8_t b[4]; read(b,4); return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24); }
};
}
