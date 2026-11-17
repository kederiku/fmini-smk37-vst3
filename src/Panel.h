#pragma once
#include <vector>
// Geometry is expressed in the editor's logical canvas, independent of HWND size.
enum class Part {None,Key,Pad,Encoder,Fader,Wheel,Button,Finish,Output};
struct Box {float x,y,w,h;bool contains(float px,float py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}};
struct Hit {Part part=Part::None;int id=-1;Box box{};bool black=false;};
std::vector<Hit> panelLayout();
Hit panelHit(const std::vector<Hit>&,float x,float y);
