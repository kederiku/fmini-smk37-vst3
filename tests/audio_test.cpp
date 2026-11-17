#include "Audio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
static void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main() {
    try {
        for(double rate:{44100.,48000.,88200.,96000.}) {
            Engine e1,e2; Audio a,b; a.prepare(rate);b.prepare(rate);
            e1.midi(0x90,60,100); e2.midi(0x90,60,100);
            unsigned n=static_cast<unsigned>(rate);
            std::vector<float> l(n),r(n),l2(n),r2(n);
            a.render(e1,l.data(),r.data(),n);
            const unsigned sizes[]={1,31,128,511,2048,4096};
            for(unsigned p=0,j=0;p<n;j++) { auto k=std::min(sizes[j%6],n-p);b.render(e2,l2.data()+p,r2.data()+p,k);p+=k; }
            double peak=0,err=0;
            for(unsigned i=0;i<n;i++) { require(std::isfinite(l[i])&&std::isfinite(r[i]),"Non-finite samples"); peak=std::max(peak,double(std::abs(l[i])));err=std::max(err,double(std::abs(l[i]-l2[i]))); }
            require(peak>.001,"FAIL: resampler must produce sound"); require(err<1e-5,"FAIL: output depends on host block size");
            e1.midi(0x80,60,0); a.render(e1,l.data(),r.data(),n);
            for(int i=0;i<6;i++)a.render(e1,l.data(),r.data(),n);
            double tail=0;for(auto x:l)tail=std::max(tail,double(std::abs(x)));
            require(tail<.002,"FAIL: note release stuck");
            std::printf("PASS rate=%.0f variable_blocks error=%g peak=%g released_tail=%g\n",rate,err,peak,tail);
        }
        return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
