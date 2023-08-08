#pragma once

#undef LOGGING

/* Mechanical sostenuto pedals lock all dampers in raised positions (even for unpressed keys)
 * if pressed while the sustain pedal is already down, making the sostenuto pedal temporarily
 * act like a sustain pedal.  Since this is usually undesired, pianists usually avoid pressing
 * the sostenuto pedal while the sustain pedal is down.  In this plug-in, I have chosen NOT to
 * implement that undesired behavior.  Pressing the sostenuto pedal while the sustain pedal is
 * down instead locks only the dampers for the currently pressed keys, just like when the
 * sustain pedal is not down.  If you really want the mechanical behavior, change the #undef
 * in the following line to a #define. */
#undef SOS_WITH_SUS_SUSTAINS_ALL

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/funknown.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID SustainPedalProcessorUID(0x152C7B8D, 0x71604051, 0x8FD3A939, 0x17EB5368);

enum SustainPedalParams : Steinberg::Vst::ParamID
{
	kRetrigger = 0,
	kSustain1 = 1,
	kSustain2 = 2,
	kSustain3 = 3,
	kSustain4 = 4,
	kSustain5 = 5,
	kSustain6 = 6,
	kSustain7 = 7,
	kSustain8 = 8,
	kSustain9 = 9,
	kSustain10 = 10,
	kSustain11 = 11,
	kSustain12 = 12,
	kSustain13 = 13,
	kSustain14 = 14,
	kSustain15 = 15,
	kSustain16 = 16,
	kSostenuto1 = 17,
	kSostenuto2 = 18,
	kSostenuto3 = 19,
	kSostenuto4 = 20,
	kSostenuto5 = 21,
	kSostenuto6 = 22,
	kSostenuto7 = 23,
	kSostenuto8 = 24,
	kSostenuto9 = 25,
	kSostenuto10 = 26,
	kSostenuto11 = 27,
	kSostenuto12 = 28,
	kSostenuto13 = 29,
	kSostenuto14 = 30,
	kSostenuto15 = 31,
	kSostenuto16 = 32,
	kBypass = 33,
	kNumParams = 34
};

typedef struct {
	uint64 note_on[2];			// bitfield: 1 = key is currently pressed
	uint64 note_sustain[2];		// bitfield: 1 = key was last pressed with damper pedal down
	uint64 note_sostenuto[2];	// bitfield: 1 = sostenuto was pressed while key down
	uint64 release_pending[2];	// bitfield: 1 = send note-off when next possible
	Event last_event[128];
	bool sustain_on, sostenuto_on;
#ifdef SOS_WITH_SUS_SUSTAINS_ALL
	bool sostenuto_all;			// sostenuto was last pressed while damper pedal down
#endif
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
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
	~SustainPedal(void);

protected:
	void send_note_offs(int32 channel, IEventList* events_out, const uint64 release_mask[2], TQuarterNotes pos, int32 sampleOffset);
	void sustain_on(int32 channel, int32 sampleOffset);
	void sustain_off(int32 channel, IEventList* events_out, TQuarterNotes pos, int32 sampleOffset);
	void sostenuto_on(int32 channel, int32 sampleOffset);
	void sostenuto_off(int32 channel, IEventList* events_out, TQuarterNotes pos, int32 sampleOffset);
	void bypass_on(IEventList* const events_out, ProcessContext* const ctx, int32 sampleOffset);

	channel_state state[16] = {};
	bool retrigger = true;
	bool bypass = false;
};

#ifdef LOGGING
	void log(const char* format, ...);
#	define LOG(format, ...) log((format), __VA_ARGS__)
#else
#	define LOG(format, ...) 0
#endif
