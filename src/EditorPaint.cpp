// SPDX-License-Identifier: GPL-3.0-only
// Layout and styling derived from Chance Roth's Felucca SMK panel,
// restyled by Guilherme Ruppert for the FMini-1 web emulator.
#include "Editor.h"
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
using namespace Gdiplus;
static constexpr unsigned finishes[]={0xb92832,0xe8e7e0,0x687481,0x16191b,0xe5a2b4,0x52575b};
static const wchar_t* buttonNames[]={L"KNOB\nBANK",L"FADER\nBANK",L"\x25c0",L"\x25b6",L"OCT-",L"OCT+",L"ARP",L"REPEAT",L"SCALE",L"CHORD",L"GLOBE",L"BT",L"PATCH",L"PARA",L"FX",L"SEQ",L"PLAY",L"REC"};
static const wchar_t* knobNames[]={L"MASTER",L"PRESETS",L"SELECT",L"ALGORITHM",L"KNOB 1",L"KNOB 2",L"KNOB 3",L"KNOB 4"};
static Color rgb(unsigned v){return Color(255,BYTE(v>>16),BYTE(v>>8),BYTE(v));}
static Color blend(Color a,Color b,float k){return Color(255,BYTE(a.GetR()*(1-k)+b.GetR()*k),BYTE(a.GetG()*(1-k)+b.GetG()*k),BYTE(a.GetB()*(1-k)+b.GetB()*k));}
static void rounded(GraphicsPath& p,Box b,float r){float d=r*2;p.AddArc(b.x,b.y,d,d,180,90);p.AddArc(b.x+b.w-d,b.y,d,d,270,90);p.AddArc(b.x+b.w-d,b.y+b.h-d,d,d,0,90);p.AddArc(b.x,b.y+b.h-d,d,d,90,90);p.CloseFigure();}
static void box(Graphics& g,Box b,Color top,Color bottom,float radius=4,Color edge=rgb(0x141719)){
 GraphicsPath p;rounded(p,b,radius);LinearGradientBrush fill(PointF(b.x,b.y),PointF(b.x,b.y+b.h),top,bottom);g.FillPath(&fill,&p);Pen pen(edge,1);g.DrawPath(&pen,&p);
}
static void label(Graphics& g,const wchar_t* s,Box b,float size,Color color,bool bold=false,bool left=false){
 FontFamily family(L"Arial");Font font(&family,size,bold?FontStyleBold:FontStyleRegular,UnitPixel);SolidBrush brush(color);StringFormat fmt;fmt.SetAlignment(left?StringAlignmentNear:StringAlignmentCenter);fmt.SetLineAlignment(StringAlignmentCenter);g.DrawString(s,-1,&font,RectF(b.x,b.y,b.w,b.h),&fmt,&brush);
}
void Editor::paint(HDC dc){
 // Draw from the latest display snapshot; never call into firmware while painting.
 Bitmap back(width,height,PixelFormat32bppPARGB);Graphics g(&back);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);g.ScaleTransform(width/1200.f,height/560.f);g.Clear(rgb(0x0d0c10));
 int theme=std::clamp(plugin.finish.load(),0,5);Color shell=rgb(finishes[theme]),ink=rgb(theme==1?0x303436:theme==4?0x392630:0xf2f0e6);
 label(g,L"SMK-37  \x00b7  FMini-1",{24,9,330,32},16,rgb(0xf4f0ea),true,true);
 label(g,L"OUTPUT",{833,13,63,26},9,rgb(0xa29cab),true);
 box(g,{900,23,144,7},rgb(0x0b0c0d),rgb(0x25232a),3,rgb(0x393540));
 float gain=float(plugin.getParamNormalized(0));box(g,{900+gain*136,17,9,19},rgb(0xff9a4d),rgb(0xdd6414),3,rgb(0x23180e));
 box(g,{1085,12,85,30},rgb(0x25232a),rgb(0x17151b),6,rgb(0x34303b));SolidBrush swatch(shell);g.FillEllipse(&swatch,1096.f,17.f,20.f,20.f);label(g,L"\x2304",{1138,12,23,30},15,rgb(0xa29cab));
 box(g,{14,56,1172,472},blend(shell,Color::White,.25f),blend(shell,Color::Black,.25f),20,rgb(0x0a0a0b));
 box(g,{16,56,1168,463},blend(shell,Color::White,.11f),shell,18,blend(shell,Color::White,.35f));
 for(float xx:{29.f,1162.f}){LinearGradientBrush b(PointF(xx,67),PointF(xx+8,77),rgb(0xe4e4e4),rgb(0x66686b));g.FillEllipse(&b,xx,66.f,10.f,10.f);Pen p(rgb(0x3c3e41));g.DrawEllipse(&p,xx,66.f,10.f,10.f);}
 box(g,{141,80,239,239},rgb(0x323436),rgb(0x08090a),18,rgb(0x111111));
 for(size_t i=0;i<pixels.size();i++){uint16_t v=display.pixels[i];uint16_t p=uint16_t((v>>8)|(v<<8));pixels[i]=0xff000000u|(((p>>11)&31)*255/31<<16)|(((p>>5)&63)*255/63<<8)|((p&31)*255/31);}
 Bitmap lcd(240,240,240*4,PixelFormat32bppARGB,reinterpret_cast<BYTE*>(pixels.data()));g.SetInterpolationMode(InterpolationModeNearestNeighbor);g.DrawImage(&lcd,Rect(154,93,213,213));g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
 box(g,{389,179,458,130},blend(shell,Color::Black,.22f),blend(shell,Color::Black,.14f),10,blend(shell,Color::Black,.3f));
 box(g,{33,341,1134,169},rgb(0x121315),rgb(0x08090a),5,rgb(0x08090a));
 auto drawItem=[&](Hit h){auto b=h.box;wchar_t text[32]{};
 switch(h.part){
 case Part::Encoder:{
  std::swprintf(text,32,L"K%d",h.id+1);label(g,text,{b.x,b.y-24,b.w,18},10,ink,true);
  SolidBrush shadow(Color(110,0,0,0));g.FillEllipse(&shadow,b.x+1,b.y+4,b.w,b.h);LinearGradientBrush rim(PointF(b.x,b.y),PointF(b.x,b.y+b.h),rgb(0x424548),rgb(0x17191b));g.FillEllipse(&rim,b.x,b.y,b.w,b.h);
  Pen grooves(rgb(0x111213),1);for(int j=0;j<30;j++){float a=j*6.2831853f/30;float cx=b.x+b.w/2,cy=b.y+b.h/2;g.DrawLine(&grooves,cx+std::cos(a)*19,cy+std::sin(a)*19,cx+std::cos(a)*22,cy+std::sin(a)*22);}
  LinearGradientBrush face(PointF(b.x,b.y+4),PointF(b.x+10,b.y+b.h),rgb(0x67696a),rgb(0x202224));g.FillEllipse(&face,b.x+5,b.y+5,b.w-10,b.h-10);Pen edge(rgb(0x111111),1);g.DrawEllipse(&edge,b.x,b.y,b.w,b.h);
  float a=(angles[h.id]-90)*3.14159265f/180;Pen marker(rgb(0xe8e8e4),2);g.DrawLine(&marker,b.x+22+std::cos(a)*8,b.y+22+std::sin(a)*8,b.x+22+std::cos(a)*17,b.y+22+std::sin(a)*17);
  label(g,knobNames[h.id],{b.x-7,b.y+49,b.w+14,14},7.5f,ink);break;}
 case Part::Pad:{unsigned color=display.pads[h.id];Color base=theme==2?rgb(0xd7d5c4):rgb(0x3c4145);int maxc=std::max({int((color>>16)&255),int((color>>8)&255),int(color&255)});if(maxc){float mix=.15f+.85f*std::sqrt(maxc/255.f);base=blend(base,Color(255,BYTE(((color>>16)&255)*255/maxc),BYTE(((color>>8)&255)*255/maxc),BYTE((color&255)*255/maxc)),mix);}
  if(sounding.part==Part::Pad&&sounding.id==h.id)base=blend(base,Color::Black,.3f);box(g,b,blend(base,Color::White,.1f),base,5,rgb(0x1b2023));std::swprintf(text,32,L"%d",h.id+1);label(g,text,b,10,maxc>160?rgb(0x222222):rgb(0xd6d9dd));break;}
 case Part::Fader:{std::swprintf(text,32,L"F%d",h.id+1);label(g,text,{b.x,b.y-25,b.w,18},10,ink,true);Pen ticks(blend(shell,ink,.35f),1);for(int j=0;j<10;j++)g.DrawLine(&ticks,b.x-3,b.y+7+j*12,b.x+4,b.y+7+j*12);box(g,{b.x+16,b.y,6,b.h},rgb(0x0a0c0e),rgb(0x171a1c),3,blend(shell,ink,.25f));float v=float(plugin.getParamNormalized(h.id+1));float yy=b.y+(1-v)*(b.h-20);box(g,{b.x,yy,b.w,21},rgb(0x74797c),rgb(0x202427),3,rgb(0x141719));SolidBrush line(rgb(0xd7d9d6));g.FillRectangle(&line,b.x+2,yy+9,b.w-4,3.f);break;}
 case Part::Wheel:{label(g,h.id?L"MOD":L"PITCH",{b.x-3,b.y-24,b.w+6,18},9,ink,true);box(g,b,rgb(0x020406),rgb(0x0b0e10),10,rgb(0x090b0e));LinearGradientBrush wheel(PointF(b.x,b.y),PointF(b.x+b.w,b.y),rgb(0x101315),rgb(0x424b50));g.FillRectangle(&wheel,b.x+6,b.y+7,b.w-12,b.h-14);Pen tread(rgb(0x0c1012),2);for(int j=0;j<24;j++)g.DrawLine(&tread,b.x+6,b.y+8+j*3.7f,b.x+b.w-6,b.y+8+j*3.7f);float v=h.id?mod:(pitch+1)*.5f;Pen marker(rgb(0xe0e4df),2);g.DrawLine(&marker,b.x+9,b.y+12+(1-v)*76,b.x+b.w-9,b.y+12+(1-v)*76);break;}
 case Part::Button:{bool lit=(display.buttons&(1u<<h.id))!=0,pressed=(buttonMask&(1u<<h.id))!=0;Color top=theme==2?rgb(0xf78a4f):rgb(0x474b4e),bottom=theme==2?rgb(0xcd4b16):rgb(0x272b2e);if(pressed)top=bottom=rgb(0x171b1e);box(g,b,top,bottom,4,rgb(0x141719));if(lit){SolidBrush led(rgb(0xff7a1a));g.FillRectangle(&led,b.x+2,b.y+b.h-3,b.w-4,3.f);}label(g,buttonNames[h.id],b,h.id<2?6.8f:8,lit?rgb(0xffd8b0):theme==2?rgb(0x311f17):rgb(0xe3e5e6),true);break;}
 case Part::Key:{bool held=typed[h.id]||(sounding.part==Part::Key&&sounding.id==h.id);Color top,bottom;if(held){top=rgb(0xe0661a);bottom=rgb(0xff9a4d);}else if(h.black){top=theme==2?rgb(0xed733d):rgb(0x373c3f);bottom=theme==2?rgb(0xac3914):rgb(0x07090a);}else{top=theme==3?rgb(0x383d40):rgb(0xfffffa);bottom=theme==3?rgb(0x1b2023):rgb(0xdfdfda);}box(g,b,top,bottom,2,h.black?rgb(0x050607):rgb(0x777a78));SolidBrush end(held?rgb(0xc86421):h.black?rgb(0x080a0b):theme==3?rgb(0x121416):rgb(0xbfc1bc));g.FillRectangle(&end,b.x+1,b.y+b.h-5,b.w-2,4.f);if(!h.black){const wchar_t* names[]={L"C",L"",L"D",L"",L"E",L"F",L"",L"G",L"",L"A",L"",L"B"};std::swprintf(text,32,L"%ls%d",names[h.id%12],3+h.id/12);label(g,text,{b.x,b.y+b.h-21,b.w,13},8,theme==3?rgb(0xaaaaaa):rgb(0x777777));}break;}
 default:break;
 }};
 for(auto h:layout)if(!h.black)drawItem(h);for(auto h:layout)if(h.black)drawItem(h);
 label(g,plugin.failed?L"Erreur du moteur : recharger l'instrument":L"A W S E D F T G Y H U J K O L P ;   \x00b7   Z / X OCTAVE   \x00b7   Maj + clic : maintenir un bouton   \x00b7   Echap : stop",{22,532,920,21},9,plugin.failed?rgb(0xff7a1a):rgb(0x87818e),false,true);
 label(g,L"FMini-1 2.6.1  /  VST3",{1015,532,166,21},9,rgb(0x87818e));
 Graphics output(dc);output.DrawImage(&back,0,0);
}
