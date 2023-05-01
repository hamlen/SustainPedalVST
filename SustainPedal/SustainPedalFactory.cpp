#include "SustainPedal.h"
#include "SustainPedalController.h"

#include "public.sdk/source/main/pluginfactory.h"

#define PluginCategory "Fx"
#define PluginName "SustainPedal"

#define PLUGINVERSION "1.0.0"

bool InitModule()
{
	LOG("InitModule called and exited.\n");
	return true;
}

bool DeinitModule()
{
	LOG("DeinitModule called and exited.\n");
	return true;
}

BEGIN_FACTORY_DEF("Kevin Hamlen",
	"no website",
	"no contact")

	LOG("GetPluginFactory called.\n");

	DEF_CLASS2(INLINE_UID_FROM_FUID(SustainPedalProcessorUID),
		PClassInfo::kManyInstances,
		kVstAudioEffectClass,
		PluginName,
		Vst::kDistributable,
		PluginCategory,
		PLUGINVERSION,
		kVstVersionString,
		SustainPedal::createInstance)

	DEF_CLASS2(INLINE_UID_FROM_FUID(SustainPedalControllerUID),
		PClassInfo::kManyInstances,
		kVstComponentControllerClass,
		PluginName "Controller",
		0, // unused
		"", //unused
		PLUGINVERSION,
		kVstVersionString,
		SustainPedalController::createInstance)

END_FACTORY
