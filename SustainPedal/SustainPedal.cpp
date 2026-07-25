#include "SustainPedal.h"
#include "SustainPedalController.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

SustainPedal::SustainPedal(void)
{
	LOG("SustainPedal constructor called.\n");
	setControllerClass(FUID(SustainPedalControllerUID));
	processSetup.maxSamplesPerBlock = kMaxInt32;
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

tresult PLUGIN_API SustainPedal::setState(IBStream* s)
{
	LOG("SustainPedal::setState called.\n");

	IBStreamer streamer(s, kLittleEndian);
	bool val;

	if (!streamer.readBool(val))
	{
		LOG("SustainPedal::setState failed due to streamer error.\n");
		return kResultFalse;
	}
	retrigger = val;

	if (streamer.readBool(val))
		release_allup = val;

	LOG("SustainPedal::setState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedal::getState(IBStream* s)
{
	LOG("SustainPedal::getState called.\n");

	IBStreamer streamer(s, kLittleEndian);
	if (!streamer.writeBool(retrigger) || !streamer.writeBool(release_allup))
	{
		LOG("SustainPedal::getState failed due to streamer error.\n");
		return kResultFalse;
	}

	LOG("SustainPedal::getState exited successfully.\n");
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

void SustainPedal::send_note_offs(int32 channel, IEventList* events_out, const Bits128& release_mask, TQuarterNotes pos, int32 sampleOffset)
{
	Event e_off = {};
	e_off.type = Event::kNoteOffEvent;
	e_off.sampleOffset = sampleOffset;
	e_off.ppqPosition = pos;
	e_off.noteOff.channel = channel;
	e_off.noteOff.velocity = 64;

	channel_state& s = state[channel];
	Bits128 rm = release_mask;
	while (rm)
	{
		uint16 pitch = rm.hibit() - 1;
		const Bits128 pmask = Bits128(pitch);
		rm ^= pmask;
		e_off.noteOff.pitch = pitch;
		e_off.flags = (s.last_event_live & pmask) ? Event::kIsLive : 0;
		e_off.noteOff.noteId = s.last_event[pitch].noteId;
		e_off.noteOff.tuning = s.last_event[pitch].tuning;
		if (events_out)
			events_out->addEvent(e_off);
	}

	if (events_out)
		s.release_pending &= ~release_mask;
	else
		s.release_pending |= release_mask;
}

void SustainPedal::sustain_on(int32 channel, int32 sampleOffset)
{
	if (!state[channel].sustain_on)
	{
		state[channel].sustain_on = true;
		if (!bypass)
			state[channel].note_sustain = state[channel].note_on | state[channel].note_sostenuto;
	}
}

void SustainPedal::sustain_off(int32 channel, IEventList* events_out, TQuarterNotes pos, int32 sampleOffset)
{
	if (state[channel].sustain_on)
	{
		state[channel].sustain_on = false;
#ifdef SOS_WITH_SUS_SUSTAINS_ALL
		if (!state[channel].sostenuto_all)
#endif
		{
			const Bits128 release_mask = state[channel].note_sustain & ~state[channel].note_on & ~state[channel].note_sostenuto;
			send_note_offs(channel, events_out, release_mask, pos, sampleOffset);
		}
		state[channel].note_sustain.clear();
	}
}

void SustainPedal::sostenuto_on(int32 channel, int32 sampleOffset)
{
	if (!state[channel].sostenuto_on)
	{
		state[channel].sostenuto_on = true;
#ifdef SOS_WITH_SUS_SUSTAINS_ALL
		state[channel].sostenuto_all = state[channel].sustain_on;
#endif
		if (!bypass)
			state[channel].note_sostenuto = state[channel].note_on;
	}
}

void SustainPedal::sostenuto_off(int32 channel, IEventList* events_out, TQuarterNotes pos, int32 sampleOffset)
{
	if (state[channel].sostenuto_on)
	{
		if (!state[channel].sustain_on)
		{
			const Bits128 release_mask = state[channel].note_sostenuto & ~state[channel].note_on;
			send_note_offs(channel, events_out, release_mask, pos, sampleOffset);
		}
		state[channel].sostenuto_on = false;
#ifdef SOS_WITH_SUS_SUSTAINS_ALL
		state[channel].sostenuto_all = false;
#endif
		state[channel].note_sostenuto.clear();
	}
}

static TQuarterNotes position_of_offset(ProcessContext* const ctx, const int32 sampleOffset)
{
	constexpr uint32 flags = ProcessContext::kProjectTimeMusicValid | ProcessContext::kTempoValid;
	return (ctx && (ctx->sampleRate > 0.) && ((ctx->state & flags) == flags))
		? ctx->projectTimeMusic + sampleOffset / (ctx->sampleRate * 60.) * ctx->tempo
		: 0.;
}

void SustainPedal::bypass_on(IEventList* const events_out, ProcessContext* const ctx, int32 sampleOffset)
{
	if (!bypass)
	{
		const TQuarterNotes pos = position_of_offset(ctx, sampleOffset);
		bypass = true;
		for (int16 channel = 0; channel < 16; ++channel)
		{
			const Bits128 release_mask = (state[channel].note_sustain | state[channel].note_sostenuto) & ~state[channel].note_on;
			send_note_offs(channel, events_out, release_mask, pos, sampleOffset);
			state[channel].note_sustain.clear();
			state[channel].note_sostenuto.clear();
		}
	}
}

static bool concurrent_event(const uint16 type, IEventList* const q, int32 index, const int32 index_limit, const int32 t, const uint16 channel, const uint16 pitch, const int32 numSamples)
{
	if (index == index_limit || !q)
		return false;
	const int32 increment = (index < index_limit) ? 1 : -1;
	for (index += increment; index != index_limit; index += increment)
	{
		Event e;
		if (q->getEvent(index, e) != kResultOk)
			return false;
		if (e.sampleOffset < 0) e.sampleOffset = 0; else if (e.sampleOffset >= numSamples) e.sampleOffset = numSamples - 1;
		if (e.sampleOffset != t)
			return false;
		if (e.type == type)
		{
			const uint16 c = ((type == Event::kNoteOnEvent) ? e.noteOn.channel : e.noteOff.channel) % 16;
			const uint16 p = ((type == Event::kNoteOnEvent) ? e.noteOn.pitch : e.noteOff.pitch) % 128;
			if ((c == channel) && (p == pitch))
				return true;
		}
	}
	return false;
}

tresult PLUGIN_API SustainPedal::process(ProcessData& data)
{
	if (data.numSamples < 0)
		return kResultFalse;

	// We shouldn't be asked for audio output, but process it anyway (emit silence) to accommodate uncompliant hosts.
	const bool is32bit = (data.symbolicSampleSize == kSample32);
	const size_t buffersize = data.numSamples * (is32bit ? sizeof(Sample32) : sizeof(Sample64));
	if ((is32bit || (data.symbolicSampleSize == kSample64)) && buffersize > 0)
	{
		for (int32 i = 0; i < data.numOutputs; ++i)
		{
			for (int32 j = 0; j < data.outputs[i].numChannels; ++j)
			{
				if (void* buffer = is32bit ? (void*)data.outputs[i].channelBuffers32[j] : (void*)data.outputs[i].channelBuffers64[j])
					memset(buffer, 0, buffersize);
			}
		}
	}
	for (int32 i = 0; i < data.numOutputs; ++i)
		data.outputs[i].silenceFlags = (1ULL << data.outputs[i].numChannels) - 1ULL;

	IParameterChanges* const params_in = data.inputParameterChanges;
	IEventList* const events_in = data.inputEvents;
	IEventList* const events_out = data.outputEvents;

	const int32 numEvents = events_in ? events_in->getEventCount() : 0;
	int32 numParamChanges[kNumParams] = {};
	IParamValueQueue* paramQueue[kNumParams] = {};
	if (params_in)
	{
		const int32 numParamsChanged = params_in->getParameterCount();

		for (int32 i = 0; i < numParamsChanged; ++i)
		{
			IParamValueQueue* const q = params_in->getParameterData(i);
			if (q)
			{
				ParamID id = q->getParameterId();
				if (id < kNumParams)
				{
					paramQueue[id] = q;
					numParamChanges[id] = q->getPointCount();
				}
			}
		}
	}

	// If this is our first opportunity to release some stuck notes, do so.
	bool pending_releases_flushed = false;
	if (events_out)
	{
		const TQuarterNotes pos = position_of_offset(data.processContext, 0);
		for (int16 channel = 0; channel < 16; ++channel)
		{
			if (state[channel].release_pending)
			{
				send_note_offs(channel, events_out, state[channel].release_pending, pos, 0);
				pending_releases_flushed = true;
			}
		}
	}

	const int32 numSamples = (data.numSamples <= 0) ? 1 : data.numSamples; // for hosts that flush parameters using numSamples=0
	int32 pindex[kNumParams] = {};
	int32 eindex = 0;
	int32 prevOutOffset = -1;
	int32 outOffsetBias = 0;
	for (;;)
	{
		int32 sampleOffset = kMaxInt32;
		int32 changedParamID = -1;

		// Find the first MIDI event that is not a note-on canceled by a concurrent note-off.
		// Events at the same time index are chronologically ambiguous, so we prioritize the
		// note-offs in order to avoid stuck notes.
		Event evt;
		for (; eindex < numEvents; ++eindex)
		{
			if (events_in->getEvent(eindex, evt) != kResultOk)
				continue;
			if (evt.sampleOffset < 0)
				evt.sampleOffset = 0; // should never happen (host served bad time index)
			else if (evt.sampleOffset >= numSamples)
				evt.sampleOffset = numSamples - 1;

			if (evt.type == Event::kNoteOnEvent)
			{
				const int32 t = evt.sampleOffset;
				const uint16 c = (uint16)evt.noteOn.channel % 16;
				const uint16 p = (uint16)evt.noteOn.pitch % 128;
				if (concurrent_event(Event::kNoteOffEvent, events_in, eindex, -1, t, c, p, numSamples) ||
					concurrent_event(Event::kNoteOffEvent, events_in, eindex, numEvents, t, c, p, numSamples))
					continue;
			}

			sampleOffset = evt.sampleOffset;
			changedParamID = kNumParams + 1;
			break;
		}

		// If there's a pedal change strictly before the next MIDI event, process it first.
		// * Pedal changes at the same time index as MIDI events must be processed last.
		//   (If they were processed first, pedal-up might generate note-off, after which a
		//   MIDI note-on could appear at the same time index, which would be chronologically
		//   ambiguous and possibly result in a stuck note.  By processing note-on/off before
		//   pedal, and ignoring all note-ons accompanied by concurrent note-offs, we ensure
		//   that pedal-up never generates a note-off for which we've already generated a
		//   note-on at the same time index.)
		// * Changes to the "Retrigger" parameter are processed first, so that the retrigger
		//   state at time index 0 is known before any events at index 0 are generated.
		ParamValue value;
		for (int32 i = 0; i < kNumParams; ++i)
		{
			if (pindex[i] < numParamChanges[i])
			{
				ParamValue v;
				int32 o;
				paramQueue[i]->getPoint(pindex[i], o, v);
				if (o < 0) o = 0; else if (o >= numSamples) o = numSamples - 1;
				if ((o < sampleOffset) || (((i == kRetrigger) && (o == sampleOffset))))
				{
					sampleOffset = o;
					value = v;
					changedParamID = i;
				}
			}
		}

		if (sampleOffset == 0)
		{
			// If we might be retriggering notes at time offset 0, we must reserve time offset 0
			// for inserted note-off events.
			if ((changedParamID != kRetrigger) && (retrigger || pending_releases_flushed))
				outOffsetBias = 1;
		}
		if ((prevOutOffset < sampleOffset - 1) || (sampleOffset + outOffsetBias >= numSamples))
		{
			// As soon as there's an unused time offset in the buffer, return to time-preserving output.
			outOffsetBias = 0;
		}

		if (changedParamID < 0)
		{
			// no more events
			break;
		}
		else if (changedParamID < kNumParams)
		{
			if (changedParamID == kRetrigger)
			{
				// "Retrigger" param changed
				retrigger = (value > 0.5);
			}
			else if (changedParamID == kBypass)
			{
				// Bypass mode on/off
				if (value > 0.5)
					bypass_on(events_out, data.processContext, sampleOffset);
				else
					bypass = false;
			}
			else if (changedParamID == kReleaseMode)
			{
				// Release mode can be FirstUp (0) or AllUp (1)
				release_allup = (value > 0.5);
			}
			else
			{
				const bool pedal_is_damper = (changedParamID >= kSustain1) && (changedParamID < kSustain1 + 16);
				const int32 channel = changedParamID - (pedal_is_damper ? kSustain1 : kSostenuto1);
				sampleOffset += outOffsetBias;
				if (value > 0.)
				{
					// pedal on
					if (pedal_is_damper)
						sustain_on(channel, sampleOffset);
					else
						sostenuto_on(channel, sampleOffset);
				}
				else
				{
					// pedal off
					const TQuarterNotes pos = position_of_offset(data.processContext, sampleOffset);
					if (pedal_is_damper)
						sustain_off(channel, events_out, pos, sampleOffset);
					else
						sostenuto_off(channel, events_out, pos, sampleOffset);
					prevOutOffset = sampleOffset;
				}
			}
			++pindex[changedParamID];
		}
		else
		{
			// non-pedal event
			evt.sampleOffset += outOffsetBias;

			switch (evt.type)
			{
			case Event::kNoteOnEvent:
			{
				const uint16 channel = (uint16)evt.noteOn.channel % 16;
				const uint16 pitch = (uint16)evt.noteOn.pitch % 128;
				const Bits128 pmask = Bits128(pitch);
				channel_state& s = state[channel];
				bool sounding = (s.note_on | s.note_sustain | s.note_sostenuto) & pmask;

				// We "retrigger" a note that is already sounding by inserting a note-off just
				// before this time offset, and then preserving the note-on.  However, we must
				// take care not to insert a note-off into a time index where there is already
				// a conflicting note-on (which would make the events chronologically ambiguous,
				// possibly resulting in a stuck note).  For example, this could happen if we
				// receive two note-on events for the same pitch on the same channel at
				// consecutive time indexes.  Such notes are so close in time that it probably
				// doesn't make sense to treat them as separate anyway, so in such unusual cases
				// we simply skip the retrigger (i.e., ignore the second note-on).
				if (sounding && retrigger && events_out && (evt.sampleOffset > 0) &&
					!concurrent_event(Event::kNoteOnEvent, events_out, events_out->getEventCount(), -1, evt.sampleOffset - 1, channel, pitch, numSamples))
				{
					s.note_on &= ~pmask;
					if (!s.sustain_on)
						s.note_sustain &= ~pmask;
					s.note_sostenuto &= ~pmask;

					Event e_off = evt;
					e_off.type = Event::kNoteOffEvent;
					--e_off.sampleOffset;
					e_off.ppqPosition = nextafter(evt.ppqPosition, (evt.ppqPosition < 0) ? -DBL_MAX : 0);
					e_off.noteOff.channel = channel;
					e_off.noteOff.pitch = pitch;
					e_off.noteOff.velocity = 64;
					e_off.noteOff.noteId = s.last_event[pitch].noteId;
					e_off.noteOff.tuning = s.last_event[pitch].tuning;
					events_out->addEvent(e_off);
					sounding = false;
				}

				if (!sounding)
				{
					if (events_out)
					{
						events_out->addEvent(evt);
						prevOutOffset = evt.sampleOffset;
					}
					s.last_event[pitch].noteId = evt.noteOn.noteId;
					s.last_event[pitch].tuning = evt.noteOn.tuning;
					if (evt.flags & Event::kIsLive)
						s.last_event_live |= Bits128(pitch);
					else
						s.last_event_live &= ~Bits128(pitch);
				}

				s.note_on |= pmask;
				if (++s.concurrent_noteons[pitch] <= 0)
					--s.concurrent_noteons[pitch];
				s.release_pending &= ~pmask;
				if (s.sustain_on && !bypass)
					s.note_sustain |= pmask;

#ifdef SOS_WITH_SUS_SUSTAINS_ALL
				if (s.sostenuto_all)
					s.note_sostenuto[phi] |= pmask;
#endif

				break;
			}

			case Event::kNoteOffEvent:
			{
				const uint16 channel = (uint16)evt.noteOff.channel % 16;
				const uint16 pitch = (uint16)evt.noteOff.pitch % 128;
				const Bits128 pmask = Bits128(pitch);
				channel_state& s = state[channel];

				if (s.concurrent_noteons[pitch] > 0)
					--s.concurrent_noteons[pitch];

				if ((s.note_on & pmask) && (!release_allup || !s.concurrent_noteons[pitch]))
				{
					s.note_on &= ~pmask;

					if (!(s.sustain_on || (s.note_sostenuto & pmask)))
					{
						s.note_sustain &= ~pmask;
						if (events_out)
						{
							events_out->addEvent(evt);
							prevOutOffset = evt.sampleOffset;
							s.release_pending &= ~pmask;
						}
						else
							s.release_pending |= pmask;
					}
				}

				break;
			}

			case Event::kPolyPressureEvent:
			case Event::kNoteExpressionValueEvent:
			case Event::kNoteExpressionTextEvent:
				prevOutOffset = evt.sampleOffset += outOffsetBias;
				if (events_out)
					events_out->addEvent(evt);
				break;
			}
			++eindex;
		}
	}

	return kResultOk;
}
