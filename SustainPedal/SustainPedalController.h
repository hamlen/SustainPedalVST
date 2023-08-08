#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID SustainPedalControllerUID(0x152C7B8D, 0x71604051, 0x8FD3A939, 0x17EB5369);

enum SustainPedalUnitId : Steinberg::Vst::UnitID
{
	kSustainUnitId = 1,
	kSostenutoUnitId = 2
};

class SustainPedalController : public EditControllerEx1, public IMidiMapping
{
public:
	SustainPedalController(void);

	static FUnknown* createInstance(void* context)
	{
		return (IEditController*) new SustainPedalController;
	}

	DELEGATE_REFCOUNT(EditControllerEx1)
	tresult PLUGIN_API queryInterface(const char* iid, void** obj) SMTG_OVERRIDE;

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id) SMTG_OVERRIDE;

	~SustainPedalController(void);
};

