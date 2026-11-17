#include "Panel.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{auto layout=panelLayout();auto count=[&](Part p){return std::count_if(layout.begin(),layout.end(),[&](auto h){return h.part==p;});};
check(count(Part::Key)==37,"FAIL: 37 playable keys required");check(count(Part::Pad)==16,"16 pads");check(count(Part::Encoder)==8,"8 encoders");check(count(Part::Fader)==4,"4 faders");check(count(Part::Button)==18,"18 buttons");check(count(Part::Wheel)==2,"2 wheels");
auto h=panelHit(layout,80,370);check(h.part==Part::Key&&h.id==1&&h.black,"Black key priority over overlapping white key");h=panelHit(layout,48,480);check(h.part==Part::Key&&h.id==0,"C3 reachable");
for(auto item:layout)if(item.part!=Part::Key){auto hit=panelHit(layout,item.box.x+item.box.w/2,item.box.y+item.box.h/2);check(hit.part==item.part&&hit.id==item.id,"Control covered or unreachable");}
check(panelHit(layout,-10,-10).part==Part::None,"Outside panel");std::puts("PASS panel geometry/control coverage/black-key precedence");return 0;}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
