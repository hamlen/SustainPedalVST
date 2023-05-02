#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "SustainPedal.h"
#include "SustainPedalController.h"

SustainPedal::SustainPedal(void)
{
	LOG("SustainPedal constructor called.\n");
	setControllerClass(FUID(SustainPedalControllerUID));
	processSetup.maxSamplesPerBlock = 8192;
	LOG("SustainPedal constructor exited.\n");
}

SustainPedal::~SustainPedal(void)
{
	LOG("SustainPedal destructor called and exited.\n");
}

tresult PLUGIN_API SustainPedal::initialize(FUnknown* context)
{
	LOG("SustainPedal::initialize called.\n");
	tresult result = AudioEffect::initialize(context);

	if (result != kResultOk)
	{
		LOG("SustainPedal::initialize failed with code %d.\n", result);
		return result;
	}

	addEventInput(STR16("Event In"));
	addEventOutput(STR16("Event Out"));

	LOG("SustainPedal::initialize exited normally.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::terminate()
{
	LOG("SustainPedal::terminate called.\n");
	tresult result = AudioEffect::terminate();
	LOG("SustainPedal::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedal::setActive(TBool state)
{
	LOG("SustainPedal::setActive called.\n");
	memset(SustainPedal::state, 0, sizeof(SustainPedal::state));
	tresult result = AudioEffect::setActive(state);
	LOG("SustainPedal::setActive exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedal::setIoMode(IoMode mode)
{
	LOG("SustainPedal::setIoMode called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::setProcessing(TBool state)
{
	LOG("SustainPedal::setProcessing called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::setState(IBStream* state)
{
	LOG("SustainPedal::setState called.\n");

	IBStreamer streamer(state, kLittleEndian);
	float rt;
	if (streamer.readFloat(rt) == false)
	{
		LOG("SustainPedal::setState failed due to streamer error.\n");
		return kResultFalse;
	}
	retrigger = (rt >= 0.5);

	LOG("SustainPedal::setState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::getState(IBStream* state)
{
	LOG("SustainPedal::getState called.\n");

	IBStreamer streamer(state, kLittleEndian);
	if (streamer.writeFloat(retrigger ? 1. : 0.) == false)
	{
		LOG("SustainPedal::getState failed due to streamer error.\n");
		return kResultFalse;
	}

	LOG("SustainPedal::getState exited successfully.\n");
	return kResultOk;
}

// Say ok to all bus arrangements, since this plug-in doesn't process any audio.
tresult PLUGIN_API SustainPedal::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	LOG("SustainPedal::setBusArrangements called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::setupProcessing(ProcessSetup& newSetup)
{
	LOG("SustainPedal::setupProcessing called.\n");
	processContextRequirements.flags = kNeedProjectTimeMusic | kNeedTempo;
	tresult result = AudioEffect::setupProcessing(newSetup);
	LOG("SustainPedal::setupProcessing exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedal::canProcessSampleSize(int32 symbolicSampleSize)
{
	LOG("SustainPedal::canProcessSapmleSize called with arg %d and exited.\n", symbolicSampleSize);
	return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API SustainPedal::getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo)
{
	LOG("SustainPedal::getRoutingInfo called.\n");
	if (inInfo.mediaType == kEvent && inInfo.busIndex == 0)
	{
		outInfo = inInfo;
		LOG("SustainPedal::getRoutingInfo exited with success.\n");
		return kResultOk;
	}
	else
	{
		LOG("SustainPedal::getRoutingInfo exited with failure.\n");
		return kResultFalse;
	}
}

void SustainPedal::addParamChange(ParamID id, IParameterChanges* params_out, int32 sampleOffset, bool value)
{
	if (params_out)
	{
		int32 numChangedParams = params_out->getParameterCount();
		IParamValueQueue* queue = nullptr;
		int32 dummy;
		for (int32 i = 0; i < numChangedParams; ++i)
		{
			IParamValueQueue* q = params_out->getParameterData(i);
			if (q->getParameterId() == id)
			{
				queue = q;
				break;
			}
		}
		if (!queue)
		{
			queue = params_out->addParameterData(id, dummy);
		}
		queue->addPoint(sampleOffset, value ? 0. : 1., dummy);
		queue->addPoint(sampleOffset + 1, value ? 1. : 0., dummy);
	}
}

void SustainPedal::pedal_on(int32 channel, IParameterChanges* params_out, int32 sampleOffset)
{
	if (!state[channel].pedal_on)
	{
		state[channel].pedal_on = true;
		state[channel].note_sus[0] = state[channel].note_on[0];
		state[channel].note_sus[1] = state[channel].note_on[1];
		addParamChange(channel + 1, params_out, sampleOffset, true);
	}
}

void SustainPedal::pedal_off(int32 channel, IEventList* events_out, TQuarterNotes pos, IParameterChanges* params_out, int32 sampleOffset)
{
	if (state[channel].pedal_on)
	{
		state[channel].pedal_on = false;
		for (uint16 i = 0; i < 128; ++i)
		{
			uint64 phi = i / 64;
			uint64 pmask = 1ULL << (i % 64);
			if (state[channel].note_sus[phi] & ~state[channel].note_on[phi] & pmask)
			{
				state[channel].last_event[i].sampleOffset = sampleOffset;
				state[channel].last_event[i].ppqPosition = pos;
				if (events_out)
					events_out->addEvent(state[channel].last_event[i]);
			}
		}
		state[channel].note_sus[0] = 0;
		state[channel].note_sus[1] = 0;
		addParamChange(channel + 1, params_out, sampleOffset, false);
	}
}

tresult PLUGIN_API SustainPedal::process(ProcessData& data)
{
	// We shouldn't be asked for audio output, but process it anyway (emit silence) to accommodate uncompliant hosts.
	bool is32bit = (data.symbolicSampleSize == kSample32);
	if (is32bit || (data.symbolicSampleSize == kSample64))
	{
		for (int32 i = 0; i < data.numOutputs; ++i)
		{
			for (int32 j = 0; j < data.outputs[i].numChannels; ++j)
			{
				void* buffer = is32bit ? (void*)data.outputs[i].channelBuffers32[j] : (void*)data.outputs[i].channelBuffers64[j];
				if (buffer)
					memset(buffer, 0, data.numSamples * (is32bit ? sizeof(*data.outputs[i].channelBuffers32[j]) : sizeof(*data.outputs[i].channelBuffers64[j])));
			}
		}
	}
	for (int32 i = 0; i < data.numOutputs; ++i)
		data.outputs[i].silenceFlags = (1ULL << data.outputs[i].numChannels) - 1;

	IParameterChanges* params_in = data.inputParameterChanges;
	IParameterChanges* params_out = data.outputParameterChanges;
	IEventList* events_in = data.inputEvents;
	IEventList* events_out = data.outputEvents;

	int32 numEvents = events_in ? events_in->getEventCount() : 0;
	int32 numParamChanges[kNumParams] = {};
	IParamValueQueue* paramQueue[kNumParams] = {};
	if (params_in)
	{
		int32 numParamsChanged = params_in->getParameterCount();

		for (int32 i = 0; i < numParamsChanged; ++i)
		{
			IParamValueQueue* q = params_in->getParameterData(i);
			ParamID id = q->getParameterId();
			if (id < kNumParams)
			{
				paramQueue[id] = q;
				numParamChanges[id] = q->getPointCount();
			}
		}
	}

	int32 pindex[kNumParams] = {};
	int32 eindex = 0;
	for (;;)
	{
		int32 nextSampleOffset = kMaxInt32;
		int32 nextId = -1;
		int32 nextUnusedSampleOffset[16] = {};

		Event evt;
		if (eindex < numEvents)
		{
			if (events_in->getEvent(eindex, evt) == kResultOk)
			{
				nextSampleOffset = evt.sampleOffset;
				nextId = kNumParams + 1;
			}
		}

		ParamValue value;
		for (int32 i = 0; i < kNumParams; ++i)
		{
			if (pindex[i] < numParamChanges[i])
			{
				ParamValue v;
				int32 o;
				if (paramQueue[i]->getPoint(pindex[i], o, v) == kResultOk)
				{
					if (o < nextSampleOffset)
					{
						nextSampleOffset = o;
						value = v;
						nextId = i;
					}
				}
			}
		}

		if (nextId < 0)
		{
			// no more events
			break;
		}
		else if (nextId < kNumParams)
		{
			if (nextId == kRetrigger)
			{
				// retrigger param change
				bool new_retrigger = (value >= 0.5);
				if (new_retrigger != retrigger)
					addParamChange(kRetrigger, params_out, nextSampleOffset, new_retrigger);
				retrigger = new_retrigger;
			}
			else if (value > 0.)
			{
				// pedal on
				pedal_on(nextId - 1, params_out, nextSampleOffset);
			}
			else
			{
				// pedal off
				TQuarterNotes pos = 0.;
				ProcessContext* ctx = data.processContext;
				constexpr uint32 flags = ProcessContext::kProjectTimeMusicValid | ProcessContext::kTempoValid;
				if (ctx && ctx->sampleRate > 0. && (data.processContext->state & flags) == flags)
					pos = ctx->projectTimeMusic + nextSampleOffset / (ctx->sampleRate * 60.) * ctx->tempo;
				const int32 c = nextId - 1;
				if (nextSampleOffset > nextUnusedSampleOffset[c])
					nextUnusedSampleOffset[c] = nextSampleOffset;
				pedal_off(c, events_out, pos, params_out, nextUnusedSampleOffset[c]);
				++nextUnusedSampleOffset[c];
			}
			++pindex[nextId];
		}
		else
		{
			// non-pedal event
			switch (evt.type)
			{
			case Event::kNoteOnEvent:
			{
				uint16 c = evt.noteOn.channel % 16;
				const uint16 p = evt.noteOn.pitch % 128;
#ifdef NOTE0_IS_PEDAL
				if (p)
				{
#endif
					const uint16 phi = p / 64;
					const uint64 pmask = 1ULL << (p % 64);
					const uint64 sounding = (state[c].note_on[phi] | state[c].note_sus[phi]) & pmask;
					const int32 noteOn_sampleOffset = (evt.sampleOffset <= nextUnusedSampleOffset[c]) ? (nextUnusedSampleOffset[c] + 1) : evt.sampleOffset;
					if (sounding && retrigger)
					{
						state[c].note_on[phi] &= ~pmask;
						if (!state[c].pedal_on)
							state[c].note_sus[phi] &= ~pmask;
						if (events_out)
						{
							Event e_off = evt;
							e_off.type = Event::kNoteOffEvent;
							e_off.sampleOffset = noteOn_sampleOffset - 1;
							e_off.noteOff.channel = c;
							e_off.noteOff.pitch = p;
							e_off.noteOff.velocity = 127;
							e_off.noteOff.noteId = state[c].last_event[p].noteOn.noteId;
							e_off.noteOff.tuning = state[c].last_event[p].noteOn.tuning;
							events_out->addEvent(e_off);
						}
					}

					if (!sounding || retrigger)
					{
						if (events_out)
						{
							Event e_on = evt;
							e_on.sampleOffset = noteOn_sampleOffset;
							if (events_out)
								events_out->addEvent(e_on);
						}
						nextUnusedSampleOffset[c] = noteOn_sampleOffset + 1;
						state[c].note_on[phi] |= pmask;
						state[c].last_event[p] = evt;
						if (state[c].pedal_on)
							state[c].note_sus[phi] |= pmask;
					}
#ifdef NOTE0_IS_PEDAL
				}
				else
				{
					// note 0 on acts like pedal on
					pedal_on(c, params_out, nextSampleOffset);
				}
#endif
				break;
			}
			case Event::kNoteOffEvent:
			{
				uint16 c = evt.noteOff.channel % 16;
				uint16 p = evt.noteOff.pitch % 128;
#ifdef NOTE0_IS_PEDAL
				if (p)
				{
#endif
					uint16 phi = p / 64;
					uint64 pmask = 1ULL << (p % 64);
					if (state[c].note_on[phi] & pmask)
					{
						state[c].last_event[p] = evt;
						state[c].note_on[phi] &= ~pmask;
					}
					if (!state[c].pedal_on)
					{
						state[c].note_sus[phi] &= ~pmask;
						if (evt.sampleOffset < nextUnusedSampleOffset[c])
							evt.sampleOffset = nextUnusedSampleOffset[c];
						if (events_out)
							events_out->addEvent(evt);
						nextUnusedSampleOffset[c] = evt.sampleOffset + 1;
					}
#ifdef NOTE0_IS_PEDAL
				}
				else
				{
					// note 0 off acts like pedal off
					if (nextSampleOffset > nextUnusedSampleOffset[c])
						nextUnusedSampleOffset[c] = nextSampleOffset;
					pedal_off(c, events_out, evt.ppqPosition, params_out, nextUnusedSampleOffset[c]);
					++nextUnusedSampleOffset[c];
				}
#endif
				break;
			}
			case Event::kPolyPressureEvent:
			case Event::kNoteExpressionValueEvent:
			case Event::kNoteExpressionTextEvent:
				if (events_out)
					events_out->addEvent(evt);
				break;
			}
			++eindex;
		}
	}

	return kResultOk;
}
