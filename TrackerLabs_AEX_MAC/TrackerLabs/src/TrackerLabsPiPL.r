#include "AEConfig.h"
#include "AE_EffectVers.h"
#ifndef AE_OS_WIN
	#include <AE_General.r>
#endif

resource 'PiPL' (16000) {
	{
		Kind { AEEffect },
		Name { "Tracker Labs" },
		Category { "Stylize" },
		Version { 0x00010000 },
#ifdef AE_OS_WIN
    #ifdef AE_PROC_INTELx64
		CodeWin64X86 {"EffectMain"},
    #endif
#else
    #ifdef AE_OS_MAC
		CodeMacIntel64 {"EffectMain"},
		CodeMacARM64 {"EffectMain"},
    #endif
#endif
		AE_Reserved_Info { 34 },
		/* NOTE: Older AE builds (e.g. 25.2.x) reject "future" SDK subversions. */
		/* [6] */
		AE_PiPL_Version {
			2,
			0
		},
		/* [7] */
		AE_Effect_Spec_Version {
			PF_PLUG_IN_VERSION,
			PF_PLUG_IN_SUBVERS
		},
		/* [8] */

		AE_Effect_Version { 524289 },
		AE_Effect_Info_Flags { 0 },
		AE_Effect_Global_OutFlags { 0x02000000 },
		AE_Effect_Global_OutFlags_2 { 0x000 },
		AE_Effect_Match_Name { "TrackerLabs_v3_Restore" }
	}
};
