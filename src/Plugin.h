#pragma once
#define NOMINMAX
#include <windows.h>
#include "Audio.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/gui/iplugview.h"
#include <atomic>
#include <mutex>
using namespace Steinberg;
using namespace Steinberg::Vst;
inline bool sameId(const TUID a,const TUID b){return std::memcmp(a,b,16)==0;}
inline void text16(TChar* out,const char* in,size_t n=128){size_t i=0;for(;i+1<n&&in[i];i++)out[i]=static_cast<TChar>(static_cast<unsigned char>(in[i]));out[i]=0;}

// Component and controller share one object. Hosts query the controller interface
// rather than creating a second object with another engine.
class Plugin final:public IComponent,public IAudioProcessor,public IEditController,public IMidiMapping,public IProcessContextRequirements {
public:
    // Five visible parameters; CC0..127, pressure and bend on each MIDI channel.
    static constexpr int parameterCount=5+16*130;
    Plugin();~Plugin();
    tresult PLUGIN_API queryInterface(const TUID,void**) override;
    uint32 PLUGIN_API addRef() override{return ++refs;}
    uint32 PLUGIN_API release() override{auto r=--refs;if(!r)delete this;return r;}
    tresult PLUGIN_API initialize(FUnknown*) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API getControllerClassId(TUID) override{return kNotImplemented;}
    tresult PLUGIN_API setIoMode(IoMode) override{return kResultOk;}
    int32 PLUGIN_API getBusCount(MediaType,BusDirection) override;
    tresult PLUGIN_API getBusInfo(MediaType,BusDirection,int32,BusInfo&) override;
    tresult PLUGIN_API getRoutingInfo(RoutingInfo&,RoutingInfo&) override{return kNotImplemented;}
    tresult PLUGIN_API activateBus(MediaType,BusDirection,int32,TBool) override;
    tresult PLUGIN_API setActive(TBool) override;
    tresult PLUGIN_API setState(IBStream*) override;
    tresult PLUGIN_API getState(IBStream*) override;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement*,int32,SpeakerArrangement*,int32) override;
    tresult PLUGIN_API getBusArrangement(BusDirection,int32,SpeakerArrangement&) override;
    tresult PLUGIN_API canProcessSampleSize(int32 size) override{return size==kSample32?kResultOk:kResultFalse;}
    uint32 PLUGIN_API getLatencySamples() override{return audio.latency();}
    tresult PLUGIN_API setupProcessing(ProcessSetup&) override;
    tresult PLUGIN_API setProcessing(TBool) override;
    tresult PLUGIN_API process(ProcessData&) override;
    uint32 PLUGIN_API getTailSamples() override{return kInfiniteTail;}
    uint32 PLUGIN_API getProcessContextRequirements() override{return IProcessContextRequirements::kNeedTransportState;}
    tresult PLUGIN_API setComponentState(IBStream* s) override{return setState(s);}
    int32 PLUGIN_API getParameterCount() override{return parameterCount;}
    tresult PLUGIN_API getParameterInfo(int32,ParameterInfo&) override;
    tresult PLUGIN_API getParamStringByValue(ParamID,ParamValue,String128) override;
    tresult PLUGIN_API getParamValueByString(ParamID,TChar*,ParamValue&) override;
    ParamValue PLUGIN_API normalizedParamToPlain(ParamID id,ParamValue v) override{return v*parameterScale(id);}
    ParamValue PLUGIN_API plainParamToNormalized(ParamID id,ParamValue v) override{return v/parameterScale(id);}
    ParamValue PLUGIN_API getParamNormalized(ParamID) override;
    tresult PLUGIN_API setParamNormalized(ParamID,ParamValue) override;
    tresult PLUGIN_API setComponentHandler(IComponentHandler*) override;
    IPlugView* PLUGIN_API createView(FIDString) override;
    tresult PLUGIN_API getMidiControllerAssignment(int32,int16,CtrlNumber,ParamID&) override;
    void send(Control);
    void edit(ParamID,double);
    void copyDisplay(Display&);
    std::atomic<int> finish{5};
    std::atomic<float> peak{0};
    std::atomic<bool> failed{false};
private:
    static double parameterScale(ParamID id){return id==0?200:id>=100&&(id-100)%130==129?16383:127;}
    static int parameterIndex(ParamID id){return id<5?int(id):id>=100&&id<100+16*130?int(id)-95:-1;}
    void applyParameter(ParamID,double);
    void drain();
    void deferHostEvents(ProcessData&);
    void render(float*,float*,unsigned);
    std::atomic<uint32> refs{1};
    std::unique_ptr<Engine> engine;
    Audio audio;
    // Audio tries engine/display locks without waiting and never takes producerMutex.
    std::mutex engineMutex,displayMutex,producerMutex;
    Display screen;
    // Acquire/release publication protects a bounded queue with monotonic counters.
    std::array<Control,4096> queue{};
    std::atomic<uint32_t> qr{0},qw{0};
    std::atomic<bool> panic{false};
    std::array<std::atomic<double>,parameterCount> values;
    IComponentHandler* handler=nullptr;
    double sampleRate=44100;
    unsigned screenCounter=0;
    bool audioActive=false,eventActive=false,wasPlaying=false;
    struct Timed{int offset;unsigned order;Control command;};
    std::array<Timed,8192> timed{};
    // Retain note-offs when a state swap prevents one callback from rendering.
    std::array<Control,8192> deferred{};
    unsigned deferredCount=0;
    // getState owns the request until the consumer publishes done=true.
    struct Snapshot{Engine* target;std::array<double,parameterCount> params{};int theme=5;std::atomic<bool> done{false};};
    std::atomic<Snapshot*> snapshotRequest{nullptr};
    std::mutex snapshotMutex;
    void capture(Snapshot&);
    struct GainPoint{int offset;double value;};
    std::array<GainPoint,8192> gainPoints{};
    unsigned gainCount=0,gainIndex=0;
    int gainPreviousOffset=-1,renderPosition=0;
    double gainPreviousValue=.5;
};
