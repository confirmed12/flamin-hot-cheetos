#include "RenderView.h"

RenderView_t originalRenderView;

void __stdcall RenderView(CViewSetup& view, int nClearFlags, int whatToDraw)
{
	CBaseEntity* local = interfaces::entitylist->GetClientEntity(interfaces::engine->GetLocalPlayer());
	if (local)
	{
		if (cvar::misc_overridefov)
		{
			if (!local->IsScoped())
				view.fov = cvar::misc_overridefov_value;
		}
	}

	originalRenderView(interfaces::client, view, nClearFlags, whatToDraw);
}