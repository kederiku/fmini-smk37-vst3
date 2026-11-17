#include "Audio.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
// Normalized Blackman-windowed sinc phases. prepare() warms this table before
// processing, avoiding lazy initialization in the audio callback.
static const std::array<std::array<float,16>,1024>& coefficients() {
    static const auto table=[] {
        std::array<std::array<float,16>,1024> t{};
        constexpr double pi=3.14159265358979323846;
        for(unsigned p=0;p<1024;p++) {
            double sum=0;
            for(unsigned j=0;j<16;j++) {
                const double x=double(j)+double(p)/1024.-8.;
                const double sinc=std::abs(x)<1e-12?1:std::sin(pi*x)/(pi*x);
                const double window=std::abs(x)>=8?0:.42+.5*std::cos(pi*x/8)+.08*std::cos(2*pi*x/8);
                t[p][j]=static_cast<float>(sinc*window);sum+=t[p][j];
            }
            for(auto& v:t[p])v=static_cast<float>(v/sum);
        }
        return t;
    }();
    return table;
}
void Audio::prepare(double r) {
    if(!std::isfinite(r)||r<44100||r>192000) throw std::runtime_error("Sample rate must be 44100..192000 Hz");
    rate=r; (void)coefficients();reset();
}
void Audio::reset() { phase=1; head=0; histL.fill(0); histR.fill(0); }
void Audio::render(Engine& e,float* l,float* r,unsigned n) {
    // Preserve firmware samples exactly when no rate conversion is needed.
    if(rate==44100) { e.render(l,r,n);return; }
    const double ratio=44100./rate;
    for(unsigned offset=0;offset<n;offset+=128) {
        const unsigned count=std::min(128u,n-offset);
        // Count using the exact same arithmetic as consumption, independent of block splitting.
        double cursor=phase; unsigned needed=0;
        for(unsigned i=0;i<count;i++){while(cursor>=1){needed++;cursor-=1;}cursor+=ratio;}
        float srcL[129],srcR[129];e.render(srcL,srcR,needed);
        unsigned index=0;
        for(unsigned i=0;i<count;i++) {
            while(phase>=1) { histL[head%32]=srcL[index];histR[head%32]=srcR[index];head++;index++;phase-=1; }
            const auto& coeff=coefficients()[std::min(1023u,static_cast<unsigned>(phase*1024))];
            float left=0,right=0;
            for(unsigned j=0;j<16;j++){left+=histL[(head-1-j)%32]*coeff[j];right+=histR[(head-1-j)%32]*coeff[j];}
            l[offset+i]=left;r[offset+i]=right;phase+=ratio;
        }
    }
}
// Convert the causal filter's eight source-sample delay to host samples.
unsigned Audio::latency() const { return rate==44100?0:static_cast<unsigned>(std::ceil(8*rate/44100)); }
