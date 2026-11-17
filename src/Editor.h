#pragma once
#include "Plugin.h"
#include "Panel.h"
// The view owns no engine. Input is queued through Plugin; painting reads a
// copied display so GDI+ work never runs on the audio thread.
class Editor final:public IPlugView {
public:
 explicit Editor(Plugin&);
 ~Editor();
 tresult PLUGIN_API queryInterface(const TUID id,void** out)override{if(!out)return kInvalidArgument;*out=nullptr;if(sameId(id,FUnknown_iid)||sameId(id,IPlugView_iid)){*out=this;addRef();return kResultOk;}return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return ++refs;}
 uint32 PLUGIN_API release()override{auto r=--refs;if(!r)delete this;return r;}
 tresult PLUGIN_API isPlatformTypeSupported(FIDString type)override{return type&&!std::strcmp(type,kPlatformTypeHWND)?kResultOk:kResultFalse;}
 tresult PLUGIN_API attached(void*,FIDString)override;
 tresult PLUGIN_API removed()override;
 tresult PLUGIN_API onWheel(float)override{return kResultFalse;}
 tresult PLUGIN_API onKeyDown(char16,int16,int16)override;
 tresult PLUGIN_API onKeyUp(char16,int16,int16)override;
 tresult PLUGIN_API getSize(ViewRect* r)override{if(!r)return kInvalidArgument;*r={0,0,width,height};return kResultOk;}
 tresult PLUGIN_API onSize(ViewRect*)override;
 tresult PLUGIN_API onFocus(TBool)override{return kResultOk;}
 tresult PLUGIN_API setFrame(IPlugFrame*)override;
 tresult PLUGIN_API canResize()override{return kResultTrue;}
 tresult PLUGIN_API checkSizeConstraint(ViewRect*)override;
private:
 static LRESULT CALLBACK windowProc(HWND,UINT,WPARAM,LPARAM);
 LRESULT message(UINT,WPARAM,LPARAM);
 void paint(HDC);
 void pointer(float x,float y,bool down);
 void move(float x,float y);
 void releasePointer();
 void releaseAll();
 void play(Hit,int velocity);
 bool typing(unsigned,bool);
 void updateSlider(Hit,float x,float y);
 Plugin& plugin;
 std::atomic<uint32> refs{1};
 IPlugFrame* frame=nullptr;
 HWND window=nullptr;
 int width=1200,height=560;
 ULONG_PTR graphicsToken=0;
 std::vector<Hit> layout;
 Display display;
 std::array<uint32_t,240*240> pixels{};
 std::array<float,8> angles{};
 std::array<bool,37> typed{};
 Hit drag{},sounding{};
 float previousY=0,mod=0,pitch=0;
 uint32_t buttonMask=0,latchedMask=0;
};
