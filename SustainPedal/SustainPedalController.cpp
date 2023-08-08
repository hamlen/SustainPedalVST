#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include <pluginterfaces/vst/ivstmidicontrollers.h>

#include "SustainPedal.h"
#include "SustainPedalController.h"

SustainPedalController::SustainPedalController(void)
{
	LOG("SustainPedalController constructor called and exited.\n");
}

SustainPedalController::~SustainPedalController(void)
{
	LOG("SustainPedalController destructor called and exited.\n");
}

tresult PLUGIN_API SustainPedalController::queryInterface(const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	return EditControllerEx1::queryInterface(iid, obj);
}

tresult PLUGIN_API SustainPedalController::initialize(FUnknown* context)
{
	LOG("SustainPedalController::initialize called.\n");
	tresult result = EditControllerEx1::initialize(context);

	if (result != kResultOk)
	{
		LOG("SustainPedalController::initialize exited prematurely with code %d.\n", result);
		return result;
	}

	parameters.addParameter(STR16("Retrigger"), nullptr, 1, 1., ParameterInfo::kCanAutomate, kRetrigger);

	addUnit(new Unit(STR16("Sustain Pedals"), kSustainUnitId));
	addUnit(new Unit(STR16("Sostenuto Pedals"), kSostenutoUnitId));

	{
		TChar sustain_name[] = STR16("Sustain0\0");
		TChar* sustain_name_suffix = sustain_name + (sizeof("Sustain") - 1);
		for (int32 i = 0; i < 16; ++i)
		{
			if (*sustain_name_suffix < u'9')
				++*sustain_name_suffix;
			else
			{
				*sustain_name_suffix++ = u'1';
				*sustain_name_suffix = u'0';
			}
			parameters.addParameter(sustain_name, nullptr, 1, 0., ParameterInfo::kCanAutomate, kSustain1 + i, kSustainUnitId);
		}
	}

	{
		TChar sostenuto_name[] = STR16("Sostenuto0\0");
		TChar* sostenuto_name_suffix = sostenuto_name + (sizeof("Sostenuto") - 1);
		for (int32 i = 0; i < 16; ++i)
		{
			if (*sostenuto_name_suffix < u'9')
				++*sostenuto_name_suffix;
			else
			{
				*sostenuto_name_suffix++ = u'1';
				*sostenuto_name_suffix = u'0';
			}
			parameters.addParameter(sostenuto_name, nullptr, 1, 0., ParameterInfo::kCanAutomate, kSostenuto1 + i, kSostenutoUnitId);
		}
	}

	parameters.addParameter(STR16("Bypass"), nullptr, 1, 0., ParameterInfo::kIsBypass, kBypass);

	LOG("SustainPedalController::initialize exited normally with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedalController::terminate()
{
	LOG("SustainPedalController::terminate called.\n");
	tresult result = EditControllerEx1::terminate();
	LOG("SustainPedalController::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedalController::setComponentState(IBStream* state)
{
	LOG("SustainPedalController::setComponentState called.\n");
	if (!state)
	{
		LOG("SustainPedalController::setComponentState failed because no state argument provided.\n");
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);
	bool val;
	if (!streamer.readBool(val))
	{
		LOG("SustainPedalController::setComponentState failed due to streamer error.\n");
		return kResultFalse;
	}
	setParamNormalized(kRetrigger, val ? 1. : 0.);

	LOG("SustainPedalController::setComponentState exited normally.\n");
	return kResultOk;
}

tresult PLUGIN_API SustainPedalController::getMidiControllerAssignment(int32 busIndex, int16 midiChannel, CtrlNumber midiControllerNumber, ParamID& tag)
{
	LOG("SustainPedalController::getMidiControllerAssignment called.\n");
	if (busIndex == 0 && (midiControllerNumber == kCtrlSustainOnOff || midiControllerNumber == kCtrlSustenutoOnOff) && 0 <= midiChannel && midiChannel < 16)
	{
		tag = ((midiControllerNumber == kCtrlSustainOnOff) ? kSustain1 : kSostenuto1) + midiChannel;
		LOG("SustainPedalController::getMidiControllerAssignment exited normally.\n");
		return kResultTrue;
	}
	LOG("SustainPedalController:getMidiControllerAssignment exited with failure.\n");
	return kResultFalse;
}