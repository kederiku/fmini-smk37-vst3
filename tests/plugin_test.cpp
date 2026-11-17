#define NOMINMAX
#include <windows.h>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/gui/iplugview.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
using namespace Steinberg;using namespace Steinberg::Vst;
static void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
class Events:public IEventList {
public:
 std::vector<Event> list;
 tresult PLUGIN_API queryInterface(const TUID id,void** p)override{*p=nullptr;if(!std::memcmp(id,IEventList_iid,16)||!std::memcmp(id,FUnknown_iid,16)){*p=this;return kResultOk;}return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return 1;}uint32 PLUGIN_API release()override{return 1;}
 int32 PLUGIN_API getEventCount()override{return static_cast<int32>(list.size());}
 tresult PLUGIN_API getEvent(int32 i,Event& e)override{if(i<0||size_t(i)>=list.size())return kInvalidArgument;e=list[i];return kResultOk;}
 tresult PLUGIN_API addEvent(Event& e)override{list.push_back(e);return kResultOk;}
 void note(bool on,int pitch,int offset,int channel=0){Event e{};e.type=on?Event::kNoteOnEvent:Event::kNoteOffEvent;e.sampleOffset=offset;e.busIndex=0;if(on)e.noteOn={int16(channel),int16(pitch),0,.8f,-1,-1};else e.noteOff={int16(channel),int16(pitch),0,-1,0};list.push_back(e);}
};
class Stream:public IBStream {
public:
 std::vector<uint8_t> data;size_t pos=0;
 tresult PLUGIN_API queryInterface(const TUID id,void** p)override{*p=nullptr;if(!std::memcmp(id,IBStream_iid,16)||!std::memcmp(id,FUnknown_iid,16)){*p=this;return kResultOk;}return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return 1;}uint32 PLUGIN_API release()override{return 1;}
 tresult PLUGIN_API read(void* p,int32 n,int32* got)override{if(n<0)return kInvalidArgument;auto k=std::min(size_t(n),data.size()-pos);std::memcpy(p,data.data()+pos,k);pos+=k;if(got)*got=int32(k);return k==size_t(n)?kResultOk:kResultFalse;}
 tresult PLUGIN_API write(void* p,int32 n,int32* wrote)override{if(n<0)return kInvalidArgument;if(pos+size_t(n)>data.size())data.resize(pos+n);std::memcpy(data.data()+pos,p,n);pos+=n;if(wrote)*wrote=n;return kResultOk;}
 tresult PLUGIN_API seek(int64 n,int32 mode,int64* result)override{auto v=n+(mode==kIBSeekCur?int64(pos):mode==kIBSeekEnd?int64(data.size()):0);if(v<0||uint64(v)>data.size())return kInvalidArgument;pos=size_t(v);if(result)*result=v;return kResultOk;}
 tresult PLUGIN_API tell(int64* p)override{if(!p)return kInvalidArgument;*p=pos;return kResultOk;}
};
class ParamQueue:public IParamValueQueue {
public:
 ParamID id=0;std::vector<std::pair<int32,double>> points;
 tresult PLUGIN_API queryInterface(const TUID,void** out)override{*out=nullptr;return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return 1;}uint32 PLUGIN_API release()override{return 1;}
 ParamID PLUGIN_API getParameterId()override{return id;}
 int32 PLUGIN_API getPointCount()override{return int32(points.size());}
 tresult PLUGIN_API getPoint(int32 i,int32& offset,ParamValue& value)override{if(i<0||size_t(i)>=points.size())return kInvalidArgument;offset=points[i].first;value=points[i].second;return kResultOk;}
 tresult PLUGIN_API addPoint(int32 o,ParamValue v,int32& i)override{i=int32(points.size());points.emplace_back(o,v);return kResultOk;}
};
class Changes:public IParameterChanges {
public:
 ParamQueue q;
 tresult PLUGIN_API queryInterface(const TUID,void** out)override{*out=nullptr;return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return 1;}uint32 PLUGIN_API release()override{return 1;}
 int32 PLUGIN_API getParameterCount()override{return 1;}
 IParamValueQueue* PLUGIN_API getParameterData(int32 i)override{return i==0?&q:nullptr;}
 IParamValueQueue* PLUGIN_API addParameterData(const ParamID&,int32&)override{return nullptr;}
};
struct Instance {
 IComponent* c=nullptr;IAudioProcessor* p=nullptr;IEditController* ctl=nullptr;
 Instance(IPluginFactory* f,const TUID cid){check(f->createInstance(cid,IComponent_iid,reinterpret_cast<void**>(&c))==kResultOk,"component creation");check(c->initialize(nullptr)==kResultOk,"initialize");check(c->queryInterface(IAudioProcessor_iid,reinterpret_cast<void**>(&p))==kResultOk,"processor query");check(c->queryInterface(IEditController_iid,reinterpret_cast<void**>(&ctl))==kResultOk,"controller query");}
 void start(double rate){ProcessSetup s{kRealtime,kSample32,4096,rate};check(p->setupProcessing(s)==kResultOk,"setup");check(c->activateBus(kAudio,kOutput,0,true)==kResultOk,"audio bus");check(c->activateBus(kEvent,kInput,0,true)==kResultOk,"event bus");check(c->setActive(true)==kResultOk,"active");check(p->setProcessing(true)==kResultOk,"processing");}
 void stop(){p->setProcessing(false);c->setActive(false);}
 ~Instance(){if(c){stop();ctl->release();p->release();c->terminate();c->release();}}
 void render(float* l,float* r,int n,Events* events=nullptr,IParameterChanges* changes=nullptr){float* channels[]={l,r};AudioBusBuffers out{};out.numChannels=2;out.channelBuffers32=channels;ProcessData d{};d.processMode=kRealtime;d.symbolicSampleSize=kSample32;d.numSamples=n;d.numOutputs=1;d.outputs=&out;d.inputEvents=events;d.inputParameterChanges=changes;check(p->process(d)==kResultOk,"process");}
};
int main() {
 try {
    HMODULE dll=LoadLibraryW(L"dist\\FMini SMK-37.vst3\\Contents\\x86_64-win\\FMini SMK-37.vst3");
    check(dll!=nullptr,"FAIL: VST3 binary cannot be loaded");
    auto init=reinterpret_cast<bool(*)()>(GetProcAddress(dll,"InitDll"));if(init)check(init(),"InitDll");
    auto get=reinterpret_cast<IPluginFactory*(*)()>(GetProcAddress(dll,"GetPluginFactory"));check(get!=nullptr,"Factory export");
    IPluginFactory* factory=get();check(factory&&factory->countClasses()==1,"One instrument class");
    PClassInfo info{};check(factory->getClassInfo(0,&info)==kResultOk,"Class info");
    std::printf("Plugin: %s\n",info.name);
    {
      Instance a(factory,info.cid),b(factory,info.cid);
      check(a.c->getBusCount(kAudio,kInput)==0&&a.c->getBusCount(kAudio,kOutput)==1,"bus configuration");
      check(a.p->canProcessSampleSize(kSample64)!=kResultOk,"Double precision must be explicitly rejected");
      IMidiMapping* mapping=nullptr;check(a.ctl->queryInterface(IMidiMapping_iid,reinterpret_cast<void**>(&mapping))==kResultOk,"MIDI mapping");ParamID id=0;check(mapping->getMidiControllerAssignment(0,0,129,id)==kResultOk,"Pitch bend mapping");mapping->release();
      IPlugView* view=a.ctl->createView("editor");check(view!=nullptr,"Native editor available");ViewRect rect{};check(view->getSize(&rect)==kResultOk&&rect.getWidth()>=900,"Editor dimensions");view->release();
      for(double rate:{44100.,48000.,88200.,96000.}) {
        a.start(rate);b.start(rate);
        std::vector<float> l(4096),r(4096),q(4096),qr(4096);
        Events notes;notes.note(true,60,100);notes.note(false,60,3000);a.render(l.data(),r.data(),4096,&notes);b.render(q.data(),qr.data(),4096);
        double peak=0,quiet=0;for(int i=0;i<4096;i++){check(std::isfinite(l[i])&&std::isfinite(r[i]),"Finite plugin output");peak=std::max(peak,double(std::abs(l[i])));quiet=std::max(quiet,double(std::abs(q[i])));}
        check(peak>.001,"VST3 MIDI note produces audio");check(quiet<1e-6,"VST3 instance isolation");
        std::printf("PASS plugin rate=%.0f peak=%g independent=%g\n",rate,peak,quiet);
        a.stop();b.stop();
      }
      check(a.ctl->setParamNormalized(0,.31)==kResultOk,"Set output gain");
      Stream state;check(a.c->getState(&state)==kResultOk,"Immediate state save");state.pos=0;check(b.c->setState(&state)==kResultOk,"New instance state restore");check(std::abs(b.ctl->getParamNormalized(0)-.31)<1e-5,"Host parameter recall");
      auto bad=state;bad.pos=0;bad.data.resize(30);check(b.c->setState(&bad)!=kResultOk,"Truncated state rejected");check(std::abs(b.ctl->getParamNormalized(0)-.31)<1e-5,"Invalid state leaves parameter intact");
      std::printf("PASS plugin state (%zu bytes), parameters, editor interface\n",state.data.size());
      {
       Instance ramp(factory,info.cid),reference(factory,info.cid);ramp.start(44100);reference.start(44100);
       Events on;on.note(true,60,0);Changes changes;changes.q.points.emplace_back(4095,0.);
       std::vector<float> l(4096),r(4096),baseL(4096),baseR(4096);
       ramp.render(l.data(),r.data(),4096,&on,&changes);reference.render(baseL.data(),baseR.data(),4096,&on);
       double error=0;for(int i=0;i<4096;i++)error=std::max(error,double(std::abs(l[i]-baseL[i]*(1.f-(i+1)/4096.f))));
       check(error<1e-5,"FAIL: VST3 continuous automation is not interpolated");
       IMidiMapping* mapping=nullptr;ramp.ctl->queryInterface(IMidiMapping_iid,reinterpret_cast<void**>(&mapping));ParamID first,second;
       check(mapping->getMidiControllerAssignment(0,0,129,first)==kResultOk&&mapping->getMidiControllerAssignment(0,1,129,second)==kResultOk&&first!=second,"FAIL: MIDI channels lack independent pitch routing");mapping->release();
       check(ramp.ctl->normalizedParamToPlain(second,1.)==16383.,"Pitch bend plain-value range");
       std::puts("PASS continuous gain automation and MIDI channel mappings");
      }
      {
       Instance sustain(factory,info.cid);sustain.start(48000);
       ProcessSetup setup{kOffline,kSample32,4096,48000};check(sustain.p->setupProcessing(setup)==kResultOk,"Offline setup");
       Changes pedal;pedal.q.id=100+130+64;pedal.q.points.emplace_back(0,1.);
       Events on;on.note(true,60,0,1);ProcessData flush{};flush.processMode=kOffline;flush.symbolicSampleSize=kSample32;flush.inputEvents=&on;flush.inputParameterChanges=&pedal;
       check(sustain.p->process(flush)==kResultOk,"Zero-frame event/parameter flush without outputs");
       std::vector<float> l(4096),r(4096);float* channels[]={l.data(),r.data()};AudioBusBuffers output{};output.numChannels=2;output.channelBuffers32=channels;
       ProcessData offline{};offline.processMode=kOffline;offline.symbolicSampleSize=kSample32;offline.numSamples=4096;offline.numOutputs=1;offline.outputs=&output;
       check(sustain.p->process(offline)==kResultOk,"Offline render");
       Events off;off.note(false,60,0,1);sustain.render(l.data(),r.data(),4096,&off);
       for(int i=0;i<84;i++)sustain.render(l.data(),r.data(),4096);
       auto peakOf=[&]{float result=0;for(float x:l)result=std::max(result,std::abs(x));return result;};
       check(peakOf()>.005f,"Channel 2 sustain must retain its released note");
       pedal.q.id=100+64;pedal.q.points[0].second=0.;sustain.render(l.data(),r.data(),4096,nullptr,&pedal);
       check(peakOf()>.005f,"Channel 1 pedal release must not release channel 2");
       pedal.q.id=100+130+64;sustain.render(l.data(),r.data(),4096,nullptr,&pedal);
       for(int i=0;i<84;i++)sustain.render(l.data(),r.data(),4096);
       check(peakOf()<.002f,"Channel 2 pedal-up must release its note");
       Events chord;for(int note:{48,52,55,60,64,67})chord.note(true,note,0);
       sustain.render(l.data(),r.data(),4096,&chord);
       auto begin=std::chrono::steady_clock::now();for(int i=0;i<256;i++)sustain.render(l.data(),r.data(),4096);
       double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
       std::printf("PASS offline/zero-frame flush/channel sustain; benchmark %.3f s for %.3f s audio (%.2fx realtime)\n",seconds,256.*4096/48000,(256.*4096/48000)/seconds);
      }
      {
       Instance concurrent(factory,info.cid);concurrent.start(44100);
       std::vector<float> l(256),r(256);Events on;on.note(true,60,0);concurrent.render(l.data(),r.data(),256,&on);
       for(int i=0;i<40;i++)concurrent.render(l.data(),r.data(),256);
       std::atomic<bool> started{false},done{false};bool saved=false;
       std::thread saver([&]{Stream s;started=true;saved=concurrent.c->getState(&s)==kResultOk;done=true;});
       while(!started)std::this_thread::yield();Sleep(10);
       Events off;off.note(false,60,0);concurrent.render(l.data(),r.data(),256,&off);
       unsigned silentBlocks=std::all_of(l.begin(),l.end(),[](float x){return x==0;})?1:0;
       for(int i=0;i<25;i++){concurrent.render(l.data(),r.data(),256);if(std::all_of(l.begin(),l.end(),[](float x){return x==0;}))silentBlocks++;}
       while(!done){concurrent.render(l.data(),r.data(),256);std::this_thread::yield();}saver.join();
       for(int i=0;i<1500;i++)concurrent.render(l.data(),r.data(),256);
       double tail=0;for(float x:l)tail=std::max(tail,double(std::abs(x)));
       check(saved,"Concurrent save succeeds");check(tail<.002,"FAIL: save discarded note-off and left a held note");check(silentBlocks==0,"FAIL: save interrupted audio with silent blocks");
       std::puts("PASS concurrent save / note-off / uninterrupted processing");
      }
    }
    factory->release();auto fini=reinterpret_cast<bool(*)()>(GetProcAddress(dll,"ExitDll"));if(fini)fini();FreeLibrary(dll);
    std::puts("PASS VST3 host integration");return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
