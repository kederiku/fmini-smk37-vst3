#include "Engine.h"
#include "Bytes.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <stdexcept>
extern "C" {
#include "fmini.h"
#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"
}
// wasm2c keeps linear memory and mutable globals inside the instance structure.
// The immutable generated function table is rebuilt on instantiation, not saved.
struct Engine::Impl {
    w2c_fmini module{};
    std::array<uint32_t,256> buttons{};
    uint32_t read=0,write=0,nextButton=0;
    bool ready=false;
    ~Impl() { if(ready) wasm2c_fmini_free(&module); }
};
static void trapCheck(int result) { if(result) throw std::runtime_error("FMini firmware trap"); }
Engine::Engine():impl(new Impl) {
    static std::once_flag init;
    std::call_once(init,[]{ wasm_rt_init(); });
    trapCheck(wasm_rt_impl_try());
    auto* m=&impl->module;
    wasm2c_fmini_instantiate(m); impl->ready=true;
    w2c_fmini_0x5Finitialize(m);
    w2c_fmini_web_nor_erase(m);
    w2c_fmini_web_smk_wheel(m,0,535);
    w2c_fmini_web_smk_wheel(m,1,615);
    for(unsigned i=0;i<4;i++) w2c_fmini_web_smk_fader(m,i,0);
    w2c_fmini_web_boot(m);
    float l[128],r[128];
    // Skip the firmware's 3030 ms splash before accepting virtual keys.
    for(unsigned p=0;p<136710;p+=128) render(l,r,std::min(128u,136710-p));
}
Engine::~Engine()=default;
void Engine::render(float* l,float* r,unsigned n) {
    trapCheck(wasm_rt_impl_try());
    auto* m=&impl->module;
    for(unsigned p=0;p<n;p+=128) {
        const unsigned count=std::min(128u,n-p);
        const auto now=w2c_fmini_web_now_ms(m);
        // Space transitions in device time so firmware debouncing sees a short
        // desktop click as a press followed by a release, rather than neither.
        if(impl->read!=impl->write && static_cast<int32_t>(now-impl->nextButton)>=0) {
            w2c_fmini_web_smk_buttons(m,impl->buttons[impl->read++%256]);
            impl->nextButton=now+24;
        }
        w2c_fmini_web_render(m,count);
        std::memcpy(l+p,m->w2c_memory.data+w2c_fmini_web_out_l(m),count*sizeof(float));
        std::memcpy(r+p,m->w2c_memory.data+w2c_fmini_web_out_r(m),count*sizeof(float));
    }
}
void Engine::midi(unsigned status,unsigned d1,unsigned d2) {
    trapCheck(wasm_rt_impl_try());
    w2c_fmini_web_midi(&impl->module,status,d1,d2);
}
void Engine::control(Control c) {
    trapCheck(wasm_rt_impl_try());
    auto* m=&impl->module;
    switch(c.kind) {
    case ControlKind::Key: w2c_fmini_web_smk_key(m,c.a,c.b); break;
    case ControlKind::Pad: w2c_fmini_web_smk_pad(m,c.a,c.b); break;
    case ControlKind::Buttons:
        if(impl->write-impl->read>=256) { impl->read=impl->write; w2c_fmini_web_smk_buttons(m,0); }
        impl->buttons[impl->write++%256]=static_cast<uint32_t>(c.a); break;
    case ControlKind::Encoder: w2c_fmini_web_smk_enc(m,c.a,c.b); break;
    case ControlKind::Fader: w2c_fmini_web_smk_fader(m,c.a,c.b); break;
    case ControlKind::Wheel: w2c_fmini_web_smk_wheel(m,c.a,c.b); break;
    case ControlKind::Midi: w2c_fmini_web_midi(m,c.a,c.b,c.c); break;
    case ControlKind::Panic:
    case ControlKind::ReleaseNotes:
        // Recall keeps pending patch/menu gestures; panic discards them as well.
        if(c.kind==ControlKind::Panic)impl->read=impl->write;
        for(unsigned i=0;i<37;i++) w2c_fmini_web_smk_key(m,i,0);
        for(unsigned i=0;i<16;i++) w2c_fmini_web_smk_pad(m,i,0);
        if(c.kind==ControlKind::Panic)w2c_fmini_web_smk_buttons(m,0);
        else if(impl->write-impl->read<256)impl->buttons[impl->write++%256]=0;
        w2c_fmini_web_smk_wheel(m,0,535);
        for(unsigned ch=0;ch<16;ch++) { w2c_fmini_web_midi(m,0xb0|ch,64,0); w2c_fmini_web_midi(m,0xb0|ch,123,0); }
        break;
    }
}
void Engine::display(Display& out) {
    trapCheck(wasm_rt_impl_try());
    auto* m=&impl->module;
    std::memcpy(out.pixels.data(),m->w2c_memory.data+w2c_fmini_web_screen(m),sizeof(out.pixels));
    std::memcpy(out.pads.data(),m->w2c_memory.data+w2c_fmini_web_smk_pad_rgb(m),sizeof(out.pads));
    out.buttons=w2c_fmini_web_smk_leds(m); out.master=w2c_fmini_web_smk_master(m);
}
// A raw firmware snapshot is only meaningful for this exact memory layout.
static constexpr char engineHash[]="dcbfa908bd9213c454238a7769160f00634a22c1efa8628d7958688c7f76862b";
std::vector<uint8_t> Engine::save() {
    auto& m=impl->module;
    std::vector<uint8_t> out;
    out.reserve(2*1024*1024);
    bytes::u32(out,0x314b4d53); bytes::u32(out,2);
    out.insert(out.end(),engineHash,engineHash+64);
    const auto n=static_cast<uint32_t>(m.w2c_memory.size);
    bytes::u32(out,n); bytes::u32(out,m.w2c_0x5F_stack_pointer);
    bytes::u32(out,m.w2c_GOT0x2Edata0x2Einternal0x2E_0x5Fmemory_base);
    bytes::u32(out,bytes::hash(m.w2c_memory.data,n));
    bytes::u32(out,impl->nextButton);bytes::u32(out,impl->write-impl->read);
    for(auto i=impl->read;i!=impl->write;i++)bytes::u32(out,impl->buttons[i%256]);
    const uint8_t* data=m.w2c_memory.data;
    uint32_t p=0,start=0;
    // Zero runs of 16+ bytes; all other bytes are literal. No native pointers.
    while(p<n) {
        if(data[p]!=0) { p++; continue; }
        uint32_t end=p;
        while(end<n&&data[end]==0) end++;
        if(end-p>=16) {
            if(p>start) { bytes::u32(out,p-start); out.insert(out.end(),data+start,data+p); }
            bytes::u32(out,0x80000000u|(end-p)); start=end;
        }
        p=end;
    }
    if(start<n) { bytes::u32(out,n-start); out.insert(out.end(),data+start,data+n); }
    return out;
}
bool Engine::load(const uint8_t* data,size_t size) {
    if(!data||size>40*1024*1024) return false;
    try {
        bytes::Reader in{data,size};
        if(in.u32()!=0x314b4d53) return false;
        const auto version=in.u32();if(version<1||version>2)return false;
        char hash[64]; in.read(hash,64);
        if(std::memcmp(hash,engineHash,64)) return false;
        const auto n=in.u32(), stack=in.u32(), base=in.u32(), checksum=in.u32();
        uint32_t next=0,queued=0;std::array<uint32_t,256> buttons{};
        if(version==2){next=in.u32();queued=in.u32();if(queued>256)return false;for(unsigned i=0;i<queued;i++){buttons[i]=in.u32();if(buttons[i]>=(1u<<18))return false;}}
        auto& m=impl->module;
        if(n!=m.w2c_memory.size||stack!=m.w2c_0x5F_stack_pointer||base!=m.w2c_GOT0x2Edata0x2Einternal0x2E_0x5Fmemory_base) return false;
        // Decode separately so malformed presets cannot partially replace live state.
        std::vector<uint8_t> raw(n,0);
        uint32_t pos=0;
        while(pos<n) {
            auto tag=in.u32(); const auto len=tag&0x7fffffffu;
            if(!len||len>n-pos) return false;
            if(!(tag&0x80000000u)) in.read(raw.data()+pos,len);
            pos+=len;
        }
        if(in.left||bytes::hash(raw.data(),raw.size())!=checksum) return false;
        std::memcpy(m.w2c_memory.data,raw.data(),n);
        impl->read=0;impl->write=queued;impl->buttons=buttons;impl->nextButton=version==2?next:w2c_fmini_web_now_ms(&m);
        return true;
    } catch(...) { return false; }
}
void Engine::copyTo(Engine& destination) const {
    // Destination retains its own native allocation and generated function table.
    auto& from=*impl;auto& to=*destination.impl;
    std::memcpy(to.module.w2c_memory.data,from.module.w2c_memory.data,static_cast<size_t>(from.module.w2c_memory.size));
    to.module.w2c_0x5F_stack_pointer=from.module.w2c_0x5F_stack_pointer;
    to.module.w2c_GOT0x2Edata0x2Einternal0x2E_0x5Fmemory_base=from.module.w2c_GOT0x2Edata0x2Einternal0x2E_0x5Fmemory_base;
    to.buttons=from.buttons;to.read=from.read;to.write=from.write;to.nextButton=from.nextButton;
}
