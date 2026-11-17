#include "Panel.h"
std::vector<Hit> panelLayout(){
 std::vector<Hit> out;
 auto add=[&](Part p,int id,float x,float y,float w,float h,bool black=false){out.push_back({p,id,{x,y,w,h},black});};
 for(int i=0;i<8;i++)add(Part::Encoder,i,400+i*56,101,44,44);
 for(int i=0;i<16;i++)add(Part::Pad,i,399+(i%8)*56,190+(i/8)*59,49,52);
 for(int i=0;i<4;i++)add(Part::Fader,i,899+i*66,103,38,132);
 for(int i=0;i<2;i++)add(Part::Wheel,i,39+i*47,102,35,102);
 for(int i=0;i<6;i++)add(Part::Button,i,38+(i%2)*47,215+(i/2)*29,43,24);
 for(int i=6;i<18;i++)add(Part::Button,i,884+((i-6)%6)*45,254+((i-6)/6)*31,41,26);
 constexpr float step=1124.f/22;int white=0;
 for(int i=0;i<37;i++){
  int n=i%12;bool black=n==1||n==3||n==6||n==8||n==10;
  if(black)add(Part::Key,i,38+(white-.31f)*step,350,step*.62f,94,true);
  else{add(Part::Key,i,38+white*step,350,step,154);white++;}
 }
 add(Part::Finish,0,1085,12,85,30);add(Part::Output,0,900,15,144,24);
 return out;
}
Hit panelHit(const std::vector<Hit>& all,float x,float y){
 // Black keys overlap white-key rectangles and must win the hit test.
 for(auto h:all)if(h.black&&h.box.contains(x,y))return h;
 for(auto h:all)if(!h.black&&h.box.contains(x,y))return h;
 return {};
}
