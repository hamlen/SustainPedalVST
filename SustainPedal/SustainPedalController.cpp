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
	return EditController::queryInterface(iid, obj);
}

tresult PLUGIN_API SustainPedalController::initialize(FUnknown* context)
{
	LOG("SustainPedalController::initialize called.\n");
	tresult result = EditController::initialize(context);

	if (result != kResultOk)
	{
		LOG("SustainPedalController::initialize exited prematurely with code %d.\n", result);
		return result;
	}

	parameters.addParameter(STR16("Retrigger"), nullptr, 1, 1., ParameterInfo::kCanAutomate, kRetrigger);
	parameters.addParameter(STR16("Pedal1"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal1);
	parameters.addParameter(STR16("Pedal2"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal2);
	parameters.addParameter(STR16("Pedal3"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal3);
	parameters.addParameter(STR16("Pedal4"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal4);
	parameters.addParameter(STR16("Pedal5"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal5);
	parameters.addParameter(STR16("Pedal6"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal6);
	parameters.addParameter(STR16("Pedal7"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal7);
	parameters.addParameter(STR16("Pedal8"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal8);
	parameters.addParameter(STR16("Pedal9"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal9);
	parameters.addParameter(STR16("Pedal10"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal10);
	parameters.addParameter(STR16("Pedal11"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal11);
	parameters.addParameter(STR16("Pedal12"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal12);
	parameters.addParameter(STR16("Pedal13"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal13);
	parameters.addParameter(STR16("Pedal14"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal14);
	parameters.addParameter(STR16("Pedal15"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal15);
	parameters.addParameter(STR16("Pedal16"), nullptr, 1, 0., ParameterInfo::kCanAutomate, kPedal16);

	LOG("SustainPedalController::initialize exited normally with code %d.\n", result);
	return result;
}

tresult PLUGIN_API SustainPedalController::terminate()
{
	LOG("SustainPedalController::terminate called.\n");
	tresult result = EditController::terminate();
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
	if (busIndex == 0 && midiControllerNumber == kCtrlSustainOnOff && 0 <= midiChannel && midiChannel < 16)
	{
		tag = kPedal1 + midiChannel;
		LOG("SustainPedalController::getMidiControllerAssignment exited normally.\n");
		return kResultTrue;
	}
	LOG("SustainPedalController:getMidiControllerAssignment exited with failure.\n");
	return kResultFalse;
}