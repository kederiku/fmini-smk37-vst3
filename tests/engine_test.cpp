#include "Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>
static void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
int main() {
    try {
        std::ifstream file("build/reference.f32",std::ios::binary);
        require(bool(file),"Reference file missing");
        std::vector<float> ref(88200*2), l(88200),r(88200),quietL(44100),quietR(44100);
        file.read(reinterpret_cast<char*>(ref.data()),ref.size()*sizeof(float));
        require(file.gcount()==static_cast<std::streamsize>(ref.size()*sizeof(float)),"Reference truncated");
        Engine a, b;
        a.midi(0x90,60,100); a.render(l.data(),r.data(),44100);
        a.midi(0x80,60,0); a.render(l.data()+44100,r.data()+44100,44100);
        b.render(quietL.data(),quietR.data(),44100);
        double peak=0,error=0,silent=0;
        for(size_t i=0;i<l.size();i++) {
            require(std::isfinite(l[i])&&std::isfinite(r[i]),"Non-finite audio");
            peak=std::max(peak,static_cast<double>(std::abs(l[i])));
            error=std::max({error,static_cast<double>(std::abs(l[i]-ref[2*i])),static_cast<double>(std::abs(r[i]-ref[2*i+1]))});
        }
        for(auto x:quietL) silent=std::max(silent,static_cast<double>(std::abs(x)));
        std::printf("peak=%.9f max_reference_error=%.9g independent_peak=%.9g\n",peak,error,silent);
        require(peak>0.001,"FAIL: MIDI note must produce audible sound");
        require(error<=1e-6,"FAIL: native audio differs from reference");
        require(silent<1e-6,"FAIL: instances share notes");
        std::puts("PASS engine/reference/instance isolation");
        a.control({ControlKind::Encoder,4,9});
        a.midi(0x90,64,99); a.render(l.data(),r.data(),1024);
        const auto state=a.save();
        require(!state.empty(),"FAIL: live state must be serializable");
        Engine restored;
        require(restored.load(state.data(),state.size()),"FAIL: fresh instance cannot restore live state");
        a.render(l.data(),r.data(),44100);
        restored.render(quietL.data(),quietR.data(),44100);
        require(std::equal(quietL.begin(),quietL.end(),l.begin())&&std::equal(quietR.begin(),quietR.end(),r.begin()),"FAIL: state recall changed subsequent audio");
        auto bad=state; bad.back()^=1;
        auto before=restored.save();
        require(!restored.load(bad.data(),bad.size()),"FAIL: corrupt state accepted");
        require(!restored.load(state.data(),state.size()-1),"FAIL: truncated state accepted");
        require(before==restored.save(),"FAIL: invalid state mutated the engine");
        std::printf("PASS live state / corrupt-state rejection (%zu bytes)\n",state.size());
        Engine btnA,btnB;
        for(int i=0;i<4;i++){btnA.render(l.data(),r.data(),44100);btnB.render(quietL.data(),quietR.data(),44100);}
        btnA.control({ControlKind::Buttons,1<<5});btnA.control({ControlKind::Buttons,0});
        auto pending=btnA.save();require(btnB.load(pending.data(),pending.size()),"Pending gesture state load");
        btnA.render(l.data(),r.data(),44100);btnB.render(quietL.data(),quietR.data(),44100);
        btnA.control({ControlKind::Key,0,100});btnB.control({ControlKind::Key,0,100});
        btnA.render(l.data(),r.data(),44100);btnB.render(quietL.data(),quietR.data(),44100);
        require(std::equal(quietL.begin(),quietL.end(),l.begin()),"FAIL: immediate save lost pending OCT+ gesture");
        std::puts("PASS pending button gesture recall");
        return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"%s\n",e.what()); return 1; }
}
