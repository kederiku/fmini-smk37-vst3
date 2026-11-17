// SPDX-License-Identifier: GPL-3.0-only
#include "Editor.h"
#include <windowsx.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
static const wchar_t* finishNames[]={L"Pro Red",L"Pro White",L"Elite",L"Pro Black",L"Pro Pink",L"Pro Charcoal"};
Editor::Editor(Plugin& p):plugin(p),layout(panelLayout()){plugin.addRef();Gdiplus::GdiplusStartupInput input;Gdiplus::GdiplusStartup(&graphicsToken,&input,nullptr);plugin.copyDisplay(display);}
Editor::~Editor(){removed();if(frame)frame->release();if(graphicsToken)Gdiplus::GdiplusShutdown(graphicsToken);plugin.release();}
tresult Editor::attached(void* parent,FIDString type){
 if(!parent||window||isPlatformTypeSupported(type)!=kResultOk)return kResultFalse;
 HINSTANCE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&windowProc),&module);
 WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=module;wc.lpszClassName=L"FMiniSMK37EditorV1";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
 window=CreateWindowExW(0,wc.lpszClassName,L"FMini SMK-37",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_TABSTOP,0,0,width,height,static_cast<HWND>(parent),nullptr,module,this);
 if(!window)return kResultFalse;SetTimer(window,1,33,nullptr);return kResultOk;
}
tresult Editor::removed(){if(window){releaseAll();KillTimer(window,1);DestroyWindow(window);window=nullptr;}return kResultOk;}
tresult Editor::onSize(ViewRect* r){if(!r||r->getWidth()<1||r->getHeight()<1)return kInvalidArgument;width=r->getWidth();height=r->getHeight();if(window)SetWindowPos(window,nullptr,0,0,width,height,SWP_NOZORDER|SWP_NOMOVE);return kResultOk;}
tresult Editor::setFrame(IPlugFrame* f){if(f)f->addRef();if(frame)frame->release();frame=f;return kResultOk;}
tresult Editor::checkSizeConstraint(ViewRect* r){if(!r)return kInvalidArgument;r->right=r->left+std::clamp(r->getWidth(),900,1800);r->bottom=r->top+r->getWidth()*560/1200;return kResultOk;}
tresult Editor::onKeyDown(char16 key,int16,int16 modifiers){if(modifiers&6)return kResultFalse;return typing(unsigned(key),true)?kResultOk:kResultFalse;}
tresult Editor::onKeyUp(char16 key,int16,int16){return typing(unsigned(key),false)?kResultOk:kResultFalse;}
LRESULT CALLBACK Editor::windowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto* self=reinterpret_cast<Editor*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(msg==WM_NCCREATE){self=static_cast<Editor*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));self->window=w;}return self?self->message(msg,wp,lp):DefWindowProcW(w,msg,wp,lp);}
LRESULT Editor::message(UINT msg,WPARAM wp,LPARAM lp){
 // Hit testing and painting share a 1200x560 logical coordinate system.
 float x=GET_X_LPARAM(lp)*1200.f/width,y=GET_Y_LPARAM(lp)*560.f/height;
 switch(msg){
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);paint(dc);EndPaint(window,&ps);return 0;}
 case WM_ERASEBKGND:return 1;
 case WM_TIMER:plugin.copyDisplay(display);InvalidateRect(window,nullptr,FALSE);return 0;
 case WM_LBUTTONDOWN:SetFocus(window);pointer(x,y,true);return 0;
 case WM_LBUTTONUP:releasePointer();if(GetCapture()==window)ReleaseCapture();return 0;
 case WM_MOUSEMOVE:if(GetCapture()==window)move(x,y);return 0;
 case WM_CAPTURECHANGED:releasePointer();return 0;
 case WM_KILLFOCUS:releaseAll();return 0;
 case WM_KEYDOWN:case WM_SYSKEYDOWN:if(!(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)){if(wp==VK_ESCAPE){releaseAll();plugin.send({ControlKind::Panic});return 0;}if(typing(unsigned(wp),true))return 0;}break;
 case WM_KEYUP:if(typing(unsigned(wp),false))return 0;break;
 case WM_MOUSEWHEEL:{POINT pt{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(window,&pt);auto h=panelHit(layout,pt.x*1200.f/width,pt.y*560.f/height);if(h.part==Part::Encoder){int steps=GET_WHEEL_DELTA_WPARAM(wp)>0?1:-1;angles[h.id]+=steps*12;plugin.send({ControlKind::Encoder,h.id,steps});return 0;}break;}
 case WM_GETDLGCODE:return DLGC_WANTALLKEYS;
 }
 return DefWindowProcW(window,msg,wp,lp);
}
void Editor::play(Hit h,int velocity){
 // Mouse and typing may hold the same key. One input source must not release
 // the other source's note when moving across the keyboard.
 if(sounding.part==h.part&&sounding.id==h.id)return;
 if(sounding.part==Part::Key&&!typed[sounding.id])plugin.send({ControlKind::Key,sounding.id,0});
 if(sounding.part==Part::Pad)plugin.send({ControlKind::Pad,sounding.id,0});
 sounding={};
 if(h.part==Part::Key){if(!typed[h.id])plugin.send({ControlKind::Key,h.id,velocity});sounding=h;}
 else if(h.part==Part::Pad){plugin.send({ControlKind::Pad,h.id,velocity});sounding=h;}
}
void Editor::updateSlider(Hit h,float x,float y){
 auto b=h.box;float value=std::clamp(1-(y-b.y)/std::max(1.f,b.h),0.f,1.f);
 if(h.part==Part::Fader)plugin.edit(h.id+1,value);
 else if(h.part==Part::Wheel){if(h.id){mod=value;plugin.send({ControlKind::Wheel,1,int(std::lround(615-value*130))});}else{pitch=value*2-1;plugin.send({ControlKind::Wheel,0,535+int(std::lround(pitch*95))});}}
 else if(h.part==Part::Output)plugin.edit(0,std::clamp(double((x-b.x)/b.w),0.,1.));
}
void Editor::pointer(float x,float y,bool){
 // Mouse capture guarantees release delivery after dragging outside the view.
 auto h=panelHit(layout,x,y);
 if(h.part==Part::Finish){HMENU menu=CreatePopupMenu();for(int i=0;i<6;i++)AppendMenuW(menu,MF_STRING|(plugin.finish==i?MF_CHECKED:0),i+1,finishNames[i]);POINT p{int(x*width/1200),int((y+20)*height/560)};ClientToScreen(window,&p);int choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,window,nullptr);DestroyMenu(menu);if(choice)plugin.finish=choice-1;return;}
 drag=h;previousY=y;SetCapture(window);
 if(h.part==Part::Key||h.part==Part::Pad)play(h,std::clamp(int(40+87*(y-h.box.y)/h.box.h),20,127));
 else if(h.part==Part::Button){uint32_t bit=1u<<h.id;if(GetKeyState(VK_SHIFT)&0x8000){latchedMask^=bit;buttonMask=(buttonMask&~bit)|(latchedMask&bit);drag={};}else buttonMask|=bit;plugin.send({ControlKind::Buttons,int(buttonMask)});}
 else updateSlider(h,x,y);
 InvalidateRect(window,nullptr,FALSE);
}
void Editor::move(float x,float y){
 if(drag.part==Part::Key||drag.part==Part::Pad){auto h=panelHit(layout,x,y);play(h,std::clamp(int(40+87*(y-h.box.y)/std::max(1.f,h.box.h)),20,127));}
 else if(drag.part==Part::Encoder){float step=(GetKeyState(VK_SHIFT)&0x8000)?24.f:7.f;int n=int((previousY-y)/step);if(n){n=std::clamp(n,-32,32);previousY-=n*step;angles[drag.id]+=n*12;plugin.send({ControlKind::Encoder,drag.id,n});}}
 else updateSlider(drag,x,y);
 InvalidateRect(window,nullptr,FALSE);
}
void Editor::releasePointer(){play({},0);if(drag.part==Part::Button){buttonMask=(buttonMask&~(1u<<drag.id))|latchedMask;plugin.send({ControlKind::Buttons,int(buttonMask)});}if(drag.part==Part::Wheel&&!drag.id){pitch=0;plugin.send({ControlKind::Wheel,0,535});}drag={};}
void Editor::releaseAll(){releasePointer();for(int i=0;i<37;i++)if(typed[i]){typed[i]=false;plugin.send({ControlKind::Key,i,0});}latchedMask=buttonMask=0;plugin.send({ControlKind::Buttons,0});pitch=0;plugin.send({ControlKind::Wheel,0,535});}
bool Editor::typing(unsigned key,bool down){
 // Key state suppresses operating-system autorepeat; octave keys use the same
 // firmware button masks as mouse input rather than changing MIDI pitches here.
 if(key>='a'&&key<='z')key-=32;
 const unsigned keys[]={'A','W','S','E','D','F','T','G','Y','H','U','J','K','O','L','P',VK_OEM_1};
 for(int i=0;i<17;i++)if(key==keys[i]||(i==16&&key==';')){int note=12+i;if(typed[note]!=down){typed[note]=down;if(!(sounding.part==Part::Key&&sounding.id==note))plugin.send({ControlKind::Key,note,down?100:0});}return true;}
 if(key=='Z'||key=='X'){uint32_t bit=1u<<(key=='Z'?4:5);auto old=buttonMask;if(down)buttonMask|=bit;else buttonMask=(buttonMask&~bit)|latchedMask;if(old!=buttonMask)plugin.send({ControlKind::Buttons,int(buttonMask)});return true;}return false;
}
