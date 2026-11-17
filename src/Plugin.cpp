// SPDX-License-Identifier: GPL-3.0-only
#include "Plugin.h"
#include "Editor.h"
#include "Bytes.h"
#include "Version.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <chrono>
#include <thread>
// Internal queue marker, separate from firmware-facing ControlKind values.
static constexpr ControlKind parameterCommand=static_cast<ControlKind>(100);
Plugin::Plugin(){for(auto& v:values)v=0.;values[0]=.5;for(int ch=0;ch<16;ch++)values[5+ch*130+129]=.5;}
Plugin::~Plugin(){if(handler)handler->release();}
tresult Plugin::queryInterface(const TUID id,void** out){
 if(!out)return kInvalidArgument;*out=nullptr;
 if(sameId(id,FUnknown_iid)||sameId(id,IPluginBase_iid)||sameId(id,IComponent_iid))*out=static_cast<IComponent*>(this);
 else if(sameId(id,IAudioProcessor_iid))*out=static_cast<IAudioProcessor*>(this);
 else if(sameId(id,IEditController_iid))*out=static_cast<IEditController*>(this);
 else if(sameId(id,IMidiMapping_iid))*out=static_cast<IMidiMapping*>(this);
 else if(sameId(id,IProcessContextRequirements_iid))*out=static_cast<IProcessContextRequirements*>(this);
 if(*out){addRef();return kResultOk;}return kNoInterface;
}
tresult Plugin::initialize(FUnknown*){if(engine)return kResultFalse;try{engine=std::make_unique<Engine>();audio.prepare(44100);engine->display(screen);return kResultOk;}catch(...){return kInternalError;}}
tresult Plugin::terminate(){std::lock_guard<std::mutex> lock(engineMutex);engine.reset();return kResultOk;}
int32 Plugin::getBusCount(MediaType t,BusDirection d){return (t==kAudio&&d==kOutput)||(t==kEvent&&d==kInput)?1:0;}
tresult Plugin::getBusInfo(MediaType t,BusDirection d,int32 i,BusInfo& b){if(i!=0||!getBusCount(t,d))return kInvalidArgument;b={};b.mediaType=t;b.direction=d;b.channelCount=t==kAudio?2:16;b.busType=kMain;b.flags=BusInfo::kDefaultActive;text16(b.name,t==kAudio?"Stereo output":"MIDI input");return kResultOk;}
tresult Plugin::activateBus(MediaType t,BusDirection d,int32 i,TBool active){if(i!=0||!getBusCount(t,d))return kInvalidArgument;if(t==kAudio)audioActive=active;else eventActive=active;return kResultOk;}
tresult Plugin::setActive(TBool state){if(!engine)return kNotInitialized;if(!state)panic=true;return kResultOk;}
tresult Plugin::setBusArrangements(SpeakerArrangement*,int32 ni,SpeakerArrangement* out,int32 no){return ni==0&&no==1&&out&&out[0]==SpeakerArr::kStereo?kResultOk:kResultFalse;}
tresult Plugin::getBusArrangement(BusDirection d,int32 i,SpeakerArrangement& out){if(d!=kOutput||i!=0)return kInvalidArgument;out=SpeakerArr::kStereo;return kResultOk;}
tresult Plugin::setupProcessing(ProcessSetup& s){if(s.symbolicSampleSize!=kSample32||s.maxSamplesPerBlock<1)return kInvalidArgument;try{audio.prepare(s.sampleRate);sampleRate=s.sampleRate;return kResultOk;}catch(...){return kInvalidArgument;}}
tresult Plugin::setProcessing(TBool active){if(!active)panic=true;return kResultOk;}
void Plugin::send(Control c){std::lock_guard<std::mutex> lock(producerMutex);auto w=qw.load(std::memory_order_relaxed);if(w-qr.load(std::memory_order_acquire)>=queue.size()){panic=true;return;}queue[w%queue.size()]=c;qw.store(w+1,std::memory_order_release);}
void Plugin::drain(){auto r=qr.load(std::memory_order_relaxed),w=qw.load(std::memory_order_acquire);for(;r!=w;r++){auto c=queue[r%queue.size()];if(c.kind==parameterCommand)applyParameter(c.a,double(c.b)/1000000.);else engine->control(c);}qr.store(r,std::memory_order_release);if(panic.exchange(false))engine->control({ControlKind::Panic});}
void Plugin::applyParameter(ParamID id,double v){auto i=parameterIndex(id);if(i<0||!std::isfinite(v))return;v=std::clamp(v,0.,1.);values[i]=v;if(id>=1&&id<=4)engine->control({ControlKind::Fader,int(id)-1,int(std::lround(v*127))});else if(id>=100){auto cc=(id-100)%130;auto channel=(id-100)/130;if(cc==129){int bend=int(std::lround(v*16383));engine->midi(0xe0|channel,bend&127,bend>>7);}else if(cc==128)engine->midi(0xd0|channel,unsigned(std::lround(v*127)),0);else engine->midi(0xb0|channel,cc,unsigned(std::lround(v*127)));}}
void Plugin::render(float* l,float* r,unsigned n){
 // VST3 continuous parameters describe a ramp from the previous block's last
 // sample (offset -1) to this block's first automation point.
 audio.render(*engine,l,r,n);float p=0;
 for(unsigned i=0;i<n;i++,renderPosition++){
  double value=values[0].load();
  if(gainCount){
   while(gainIndex<gainCount&&gainPoints[gainIndex].offset<=renderPosition){gainPreviousOffset=gainPoints[gainIndex].offset;gainPreviousValue=gainPoints[gainIndex++].value;}
   value=gainPreviousValue;
   if(gainIndex<gainCount){auto next=gainPoints[gainIndex];value+=(next.value-gainPreviousValue)*double(renderPosition-gainPreviousOffset)/double(next.offset-gainPreviousOffset);}
  }
  float gain=float(value*2);l[i]*=gain;r[i]*=gain;p=std::max({p,std::abs(l[i]),std::abs(r[i])});
 }
 peak.store(p);
}
tresult Plugin::process(ProcessData& d){
 if(d.numSamples<0||d.symbolicSampleSize!=kSample32)return kInvalidArgument;
 auto silence=[&]{if(d.outputs)for(int b=0;b<d.numOutputs;b++){auto& out=d.outputs[b];if(out.channelBuffers32)for(int ch=0;ch<out.numChannels;ch++)if(out.channelBuffers32[ch])std::fill_n(out.channelBuffers32[ch],d.numSamples,0.f);out.silenceFlags=~uint64(0);}};
 // Never wait for preset replacement on the audio thread. Deferred host events
 // preserve note-offs even when this callback has to output silence.
 std::unique_lock<std::mutex> lock(engineMutex,std::try_to_lock);
 if(!lock.owns_lock()){deferHostEvents(d);silence();return kResultOk;}
 if(!engine||failed){silence();return kResultOk;}
 try{
  drain();
  for(unsigned i=0;i<deferredCount;i++){auto c=deferred[i];if(c.kind==parameterCommand)applyParameter(c.a,double(c.b)/1000000);else engine->control(c);}deferredCount=0;
  if(d.processContext){bool playing=(d.processContext->state&ProcessContext::kPlaying)!=0;if(wasPlaying&&!playing)engine->control({ControlKind::Panic});wasPlaying=playing;}
  size_t count=0;bool overflow=false;
  gainCount=gainIndex=0;gainPreviousOffset=-1;renderPosition=0;gainPreviousValue=values[0].load();
  auto add=[&](int offset,Control c){if(count==timed.size()){overflow=true;return;}timed[count]={std::clamp(offset,0,d.numSamples),unsigned(count),c};count++;};
  if(eventActive&&d.inputEvents)for(int i=0;i<d.inputEvents->getEventCount();i++){
   Event e{};if(d.inputEvents->getEvent(i,e)!=kResultOk||e.busIndex!=0)continue;
   if(e.type==Event::kNoteOnEvent&&e.noteOn.pitch>=0&&e.noteOn.pitch<128&&std::isfinite(e.noteOn.velocity))add(e.sampleOffset,{ControlKind::Midi,0x90|(e.noteOn.channel&15),e.noteOn.pitch,int(std::lround(std::clamp(e.noteOn.velocity,0.f,1.f)*127))});
   else if(e.type==Event::kNoteOffEvent&&e.noteOff.pitch>=0&&e.noteOff.pitch<128)add(e.sampleOffset,{ControlKind::Midi,0x80|(e.noteOff.channel&15),e.noteOff.pitch,0});
   else if(e.type==Event::kPolyPressureEvent&&std::isfinite(e.polyPressure.pressure))add(e.sampleOffset,{ControlKind::Midi,0xa0|(e.polyPressure.channel&15),e.polyPressure.pitch,int(std::lround(std::clamp(e.polyPressure.pressure,0.f,1.f)*127))});
  }
  if(d.inputParameterChanges)for(int i=0;i<d.inputParameterChanges->getParameterCount();i++){
   auto* q=d.inputParameterChanges->getParameterData(i);if(!q||parameterIndex(q->getParameterId())<0)continue;
   for(int j=0;j<q->getPointCount();j++){
    int32 offset=0;double value=0;
    if(q->getPoint(j,offset,value)!=kResultOk||!std::isfinite(value))continue;
    value=std::clamp(value,0.,1.);
    if(q->getParameterId()==0){if(gainCount<gainPoints.size())gainPoints[gainCount++]={std::clamp(offset,0,d.numSamples),value};else overflow=true;}
    else add(offset,{parameterCommand,int(q->getParameterId()),int(std::lround(value*1000000)),0});
   }
  }
  if(overflow){count=0;gainCount=0;engine->control({ControlKind::Panic});}
  std::sort(gainPoints.begin(),gainPoints.begin()+gainCount,[](auto a,auto b){return a.offset<b.offset;});
  // Preserve insertion order for simultaneous events; render only up to each
  // event's timestamp before applying it to the firmware.
  std::sort(timed.begin(),timed.begin()+count,[](const Timed& a,const Timed& b){return a.offset<b.offset||(a.offset==b.offset&&a.order<b.order);});
  float scratchL[256],scratchR[256];int done=0;
  auto segment=[&](int end){while(done<end){unsigned n=unsigned(std::min(256,end-done));float *l=scratchL,*r=scratchR;bool output=audioActive&&d.numOutputs>0&&d.outputs&&d.outputs[0].numChannels>=2&&d.outputs[0].channelBuffers32&&d.outputs[0].channelBuffers32[0]&&d.outputs[0].channelBuffers32[1];if(output){l=d.outputs[0].channelBuffers32[0]+done;r=d.outputs[0].channelBuffers32[1]+done;}render(l,r,n);done+=int(n);}};
  silence();
  for(size_t i=0;i<count;i++){segment(timed[i].offset);auto c=timed[i].command;if(c.kind==parameterCommand)applyParameter(c.a,double(c.b)/1000000);else engine->control(c);}
  segment(d.numSamples);
  if(gainCount)values[0]=gainPoints[gainCount-1].value;
  if(audioActive&&d.numOutputs>0&&d.outputs)d.outputs[0].silenceFlags=0;
  screenCounter+=unsigned(d.numSamples);if(screenCounter>=sampleRate/30){screenCounter=0;std::unique_lock<std::mutex> screenLock(displayMutex,std::try_to_lock);if(screenLock.owns_lock())engine->display(screen);}
  // Snapshot compression/writing happens on the requesting thread. Only the
  // preallocated memory copy occurs here, at a coherent block boundary.
  if(auto* request=snapshotRequest.exchange(nullptr,std::memory_order_acq_rel))capture(*request);
  return kResultOk;
 }catch(...){failed=true;silence();return kResultOk;}
}
void Plugin::deferHostEvents(ProcessData& d){
 auto push=[&](Control c){if(deferredCount<deferred.size())deferred[deferredCount++]=c;else panic=true;};
 if(d.inputEvents)for(int i=0;i<d.inputEvents->getEventCount();i++){Event e{};if(d.inputEvents->getEvent(i,e)!=kResultOk||e.busIndex!=0)continue;if(e.type==Event::kNoteOnEvent&&std::isfinite(e.noteOn.velocity))push({ControlKind::Midi,0x90|(e.noteOn.channel&15),e.noteOn.pitch,int(std::lround(std::clamp(e.noteOn.velocity,0.f,1.f)*127))});else if(e.type==Event::kNoteOffEvent)push({ControlKind::Midi,0x80|(e.noteOff.channel&15),e.noteOff.pitch,0});}
 if(d.inputParameterChanges)for(int i=0;i<d.inputParameterChanges->getParameterCount();i++){auto* q=d.inputParameterChanges->getParameterData(i);if(!q||parameterIndex(q->getParameterId())<0)continue;for(int j=0;j<q->getPointCount();j++){int32 offset=0;double v=0;if(q->getPoint(j,offset,v)==kResultOk&&std::isfinite(v))push({parameterCommand,int(q->getParameterId()),int(std::lround(std::clamp(v,0.,1.)*1000000)),0});}}
}
void Plugin::capture(Snapshot& request){engine->copyTo(*request.target);for(size_t i=0;i<values.size();i++)request.params[i]=values[i].load();request.theme=finish.load();request.done.store(true,std::memory_order_release);}
void Plugin::copyDisplay(Display& d){std::lock_guard<std::mutex> lock(displayMutex);d=screen;}
tresult Plugin::getParameterInfo(int32 i,ParameterInfo& p){
 if(i<0||i>=parameterCount)return kInvalidArgument;
 p={};p.id=i<5?ParamID(i):ParamID(i+95);p.unitId=0;
 const bool bend=i>=5&&(i-5)%130==129;
 p.stepCount=i==0?0:bend?16383:127;p.defaultNormalizedValue=(i==0||bend)?.5:0.;
 p.flags=i<5?ParameterInfo::kCanAutomate:ParameterInfo::kIsHidden;
 char name[64];
 if(i==0)std::strcpy(name,"Output");else if(i<5)std::snprintf(name,64,"Fader %d",i);
 else {int channel=(i-5)/130+1,cc=(i-5)%130;if(cc==129)std::snprintf(name,64,"Ch %d Pitch bend",channel);else if(cc==128)std::snprintf(name,64,"Ch %d Pressure",channel);else std::snprintf(name,64,"Ch %d MIDI CC %d",channel,cc);}
 text16(p.title,name);text16(p.shortTitle,name);if(i==0)text16(p.units,"%");return kResultOk;
}
tresult Plugin::getParamStringByValue(ParamID id,ParamValue v,String128 out){if(parameterIndex(id)<0||!std::isfinite(v))return kInvalidArgument;char s[64];std::snprintf(s,64,"%.1f",normalizedParamToPlain(id,v));text16(out,s);return kResultOk;}
tresult Plugin::getParamValueByString(ParamID id,TChar* text,ParamValue& v){if(parameterIndex(id)<0||!text)return kInvalidArgument;char s[128]{};for(int i=0;i<127&&text[i];i++)s[i]=char(text[i]);char* end=nullptr;double x=std::strtod(s,&end);if(end==s||!std::isfinite(x))return kInvalidArgument;v=std::clamp(plainParamToNormalized(id,x),0.,1.);return kResultOk;}
ParamValue Plugin::getParamNormalized(ParamID id){int i=parameterIndex(id);return i<0?0:values[i].load();}
tresult Plugin::setParamNormalized(ParamID id,ParamValue v){int i=parameterIndex(id);if(i<0||!std::isfinite(v))return kInvalidArgument;v=std::clamp(v,0.,1.);values[i]=v;send({parameterCommand,int(id),int(std::lround(v*1000000)),0});return kResultOk;}
tresult Plugin::setComponentHandler(IComponentHandler* h){if(h)h->addRef();if(handler)handler->release();handler=h;return kResultOk;}
void Plugin::edit(ParamID id,double v){setParamNormalized(id,v);if(handler){handler->beginEdit(id);handler->performEdit(id,v);handler->endEdit(id);}}
tresult Plugin::getMidiControllerAssignment(int32 bus,int16 channel,CtrlNumber cc,ParamID& id){if(bus||channel<0||channel>=16||cc<0||cc>=130)return kResultFalse;id=100+channel*130+cc;return kResultOk;}
IPlugView* Plugin::createView(FIDString name){return name&&!std::strcmp(name,"editor")?new Editor(*this):nullptr;}
static bool readExact(IBStream* s,void* p,size_t n){if(!s||n>INT32_MAX)return false;int32 got=0;return s->read(p,int32(n),&got)==kResultOk&&got==int32(n);}
static bool writeExact(IBStream* s,const void* p,size_t n){if(!s||n>INT32_MAX)return false;int32 wrote=0;return s->write(const_cast<void*>(p),int32(n),&wrote)==kResultOk&&wrote==int32(n);}
tresult Plugin::getState(IBStream* s){try{
 // Serialize save requests so each stack-owned request has exactly one consumer.
 std::lock_guard<std::mutex> stateLock(snapshotMutex);
 auto snapshot=std::make_unique<Engine>();Snapshot request{snapshot.get()};
 snapshotRequest.store(&request,std::memory_order_release);
 auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(200);
 while(!request.done.load(std::memory_order_acquire)&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
 if(!request.done.load(std::memory_order_acquire)){
  // If audio is idle, withdraw the request before taking the engine lock. If
  // audio already claimed it, keep the request alive until its copy completes.
  auto* expected=&request;
  if(snapshotRequest.compare_exchange_strong(expected,nullptr)){std::lock_guard<std::mutex> lock(engineMutex);if(!engine)return kNotInitialized;drain();capture(request);}
  else while(!request.done.load(std::memory_order_acquire))std::this_thread::yield();
 }
 std::vector<uint8_t> out;bytes::u32(out,0x31545356);bytes::u32(out,2);bytes::u32(out,unsigned(request.theme));for(auto v:request.params)bytes::u32(out,unsigned(std::lround(v*1000000)));auto raw=snapshot->save();bytes::u32(out,unsigned(raw.size()));out.insert(out.end(),raw.begin(),raw.end());
 return writeExact(s,out.data(),out.size())?kResultOk:kResultFalse;
 }catch(...){return kInternalError;}}
tresult Plugin::setState(IBStream* s){try{
 // Validate and construct off the audio thread; swap only after full success.
 std::array<uint8_t,12> prefix{};if(!readExact(s,prefix.data(),prefix.size()))return kResultFalse;
 bytes::Reader head{prefix.data(),prefix.size()};if(head.u32()!=0x31545356)return kResultFalse;auto version=head.u32();if(version<1||version>2)return kResultFalse;auto color=head.u32();if(color>5)return kResultFalse;
 const unsigned count=version==1?135:parameterCount;std::vector<uint8_t> header(count*4+4);if(!readExact(s,header.data(),header.size()))return kResultFalse;bytes::Reader in{header.data(),header.size()};
 std::array<double,parameterCount> params{};for(int ch=0;ch<16;ch++)params[5+ch*130+129]=.5;
 for(unsigned i=0;i<count;i++){auto x=in.u32();if(x>1000000)return kResultFalse;params[i]=x/1000000.;}auto size=in.u32();if(size>40*1024*1024||size<88)return kResultFalse;
 std::vector<uint8_t> raw(size);if(!readExact(s,raw.data(),raw.size()))return kResultFalse;
 auto replacement=std::make_unique<Engine>();if(!replacement->load(raw.data(),raw.size()))return kResultFalse;
 replacement->control({ControlKind::ReleaseNotes});
 {std::lock_guard<std::mutex> lock(engineMutex);engine.swap(replacement);audio.reset();for(size_t i=0;i<values.size();i++)values[i]=params[i];finish=int(color);qr=qw.load();failed=false;std::lock_guard<std::mutex> screenLock(displayMutex);engine->display(screen);}
 return kResultOk;
 }catch(...){return kInternalError;}}

// Stable host identity: changing this ID would break saved project references.
static const TUID classId=INLINE_UID(0x53F84EB2,0x19A84C33,0xADB0E17C,0x37F12601);
class Factory final:public IPluginFactory2 {
 std::atomic<uint32> refs{1};
public:
 tresult PLUGIN_API queryInterface(const TUID id,void** out)override{if(!out)return kInvalidArgument;*out=nullptr;if(sameId(id,FUnknown_iid)||sameId(id,IPluginFactory_iid)||sameId(id,IPluginFactory2_iid)){*out=static_cast<IPluginFactory2*>(this);addRef();return kResultOk;}return kNoInterface;}
 uint32 PLUGIN_API addRef()override{return ++refs;}uint32 PLUGIN_API release()override{auto r=--refs;if(!r)delete this;return r;}
 tresult PLUGIN_API getFactoryInfo(PFactoryInfo* p)override{if(!p)return kInvalidArgument;*p={};std::strcpy(p->vendor,"FMini SMK-37 Community Port");std::strcpy(p->url,"https://github.com/ruppert/fmini-1");p->flags=PFactoryInfo::kUnicode;return kResultOk;}
 int32 PLUGIN_API countClasses()override{return 1;}
 tresult PLUGIN_API getClassInfo(int32 i,PClassInfo* p)override{if(i||!p)return kInvalidArgument;*p={};std::memcpy(p->cid,classId,16);p->cardinality=PClassInfo::kManyInstances;std::strcpy(p->category,kVstAudioEffectClass);std::strcpy(p->name,"FMini SMK-37");return kResultOk;}
 tresult PLUGIN_API getClassInfo2(int32 i,PClassInfo2* p)override{if(i||!p)return kInvalidArgument;*p={};std::memcpy(p->cid,classId,16);p->cardinality=PClassInfo::kManyInstances;std::strcpy(p->category,kVstAudioEffectClass);std::strcpy(p->name,"FMini SMK-37");std::strcpy(p->subCategories,"Instrument|Synth");std::strcpy(p->vendor,"FMini SMK-37 Community Port");std::strcpy(p->version,FMINI_VERSION);std::strcpy(p->sdkVersion,"VST 3.8.0");p->classFlags=kSimpleModeSupported;return kResultOk;}
 tresult PLUGIN_API createInstance(FIDString cid,FIDString iid,void** out)override{if(!out||!cid||!iid)return kInvalidArgument;*out=nullptr;if(!sameId(cid,classId))return kInvalidArgument;try{auto* p=new Plugin;auto result=p->queryInterface(iid,out);p->release();return result;}catch(...){return kOutOfMemory;}}
};
extern "C" __declspec(dllexport) IPluginFactory* GetPluginFactory(){return new Factory;}
extern "C" __declspec(dllexport) bool InitDll(){return true;}
extern "C" __declspec(dllexport) bool ExitDll(){return true;}
