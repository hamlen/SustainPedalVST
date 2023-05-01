#pragma once

#undef LOGGING

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/funknown.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

// Parameter enumeration
enum SustainPedalParams : Steinberg::Vst::ParamID
{
	kRetrigger = 0,
	kPedal1 = 1,
	kPedal2 = 2,
	kPedal3 = 3,
	kPedal4 = 4,
	kPedal5 = 5,
	kPedal6 = 6,
	kPedal7 = 7,
	kPedal8 = 8,
	kPedal9 = 9,
	kPedal10 = 10,
	kPedal11 = 11,
	kPedal12 = 12,
	kPedal13 = 13,
	kPedal14 = 14,
	kPedal15 = 15,
	kPedal16 = 16,
	kNumParams = 17
};

// Plugin processor GUID - must be unique
static const FUID SustainPedalProcessorUID(0x152C7B8D, 0x71604051, 0x8FD3A939, 0x17EB5368);

typedef struct {
	uint64 note_on[2], note_sus[2];
	Event last_event[128];
	bool pedal_on;
} channel_state;

class SustainPedal : public AudioEffect
{
public:
	SustainPedal(void);

	static FUnknown* createInstance(void* context)
	{
		return (IAudioProcessor*) new SustainPedal();
	}

	tresult PLUGIN_API initialize(FUnknown* context);
	tresult PLUGIN_API terminate();
	tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup);
	tresult PLUGIN_API setActive(TBool state);
	tresult PLUGIN_API setProcessing(TBool state);
	tresult PLUGIN_API process(ProcessData& data);
	tresult PLUGIN_API getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo);
	tresult PLUGIN_API setIoMode(IoMode mode);
	tresult PLUGIN_API setState(IBStream* state);
	tresult PLUGIN_API getState(IBStream* state);
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
	~SustainPedal(void);

protected:
	void addParamChange(ParamID id, IParameterChanges* params_out, int32 sampleOffset, bool value);
	void pedal_on(int32 channel, IParameterChanges* params_out, int32 sampleOffset);
	void pedal_off(int32 channel, IEventList* events_out, TQuarterNotes pos, IParameterChanges* params_out, int32 sampleOffset);
	channel_state state[16] = {};
	bool retrigger = true;
};

#ifdef LOGGING
	void log(const char* format, ...);
#	define LOG(format, ...) log((format), __VA_ARGS__)
#else
#	define LOG(format, ...) 0
#endif
