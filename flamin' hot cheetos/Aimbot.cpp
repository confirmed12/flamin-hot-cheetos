#include "Aimbot.h"

Aimbot aimbot;

Aimbot::Aimbot(void)
{
	bestTarget = -1;

	viewAngles = QAngle(0.f, 0.f, 0.f);
	hitboxPosition = Vector(0.f, 0.f, 0.f);
	finalAngles = QAngle(0.f, 0.f, 0.f);
}

void Aimbot::think(CBaseEntity* local, CBaseCombatWeapon* weapon)
{
	if (!(GetAsyncKeyState(cvar::general_key_aimbot) & 0x8000))
		return;

	bestTarget = getBestTarget(local, weapon, hitboxPosition);
	if (bestTarget == -1)
		return;

	CBaseEntity* entity = interfaces::entitylist->GetClientEntity(bestTarget);
	if (!entity)
		return;

	if (tools.getDistance(local->GetEyePosition(), hitboxPosition) > 8192.f)
		return;

	hitboxPosition.x += tools.random(-cvar::aimbot_randomize_hitbox, cvar::aimbot_randomize_hitbox);
	hitboxPosition.y += tools.random(-cvar::aimbot_randomize_hitbox, cvar::aimbot_randomize_hitbox);
	hitboxPosition.z += tools.random(-cvar::aimbot_randomize_hitbox, cvar::aimbot_randomize_hitbox);

	tools.computeAngle(local->GetEyePosition(), hitboxPosition, finalAngles);
	tools.normalizeAngles(finalAngles);

	finalAngles.x -= getRandomizedRecoil(local).x;
	finalAngles.y -= getRandomizedRecoil(local).y;

	finalAngles = viewAngles - finalAngles;
	tools.normalizeAngles(finalAngles);

	float sensitivity = *(float*)(interfaces::clientdll + 0xA34CB4);
	if (!sensitivity)
		return;

	float pixels = sensitivity * 0.22f / tools.random(0.8f, 1.2f);
	float smoothRate = tools.random(0.8f, 1.2f) * 2.f;

	if (finalAngles.x > smoothRate)
		finalAngles.x = smoothRate;
	else if (finalAngles.x < -smoothRate)
		finalAngles.x = -smoothRate;

	if (finalAngles.y > smoothRate)
		finalAngles.y = smoothRate;
	else if (finalAngles.y < -smoothRate)
		finalAngles.y = -smoothRate;

	finalAngles.x /= pixels * -1.f;
	finalAngles.y /= pixels;
	finalAngles.y += tools.random(-cvar::aimbot_randomize_angle, cvar::aimbot_randomize_angle);

	moveMouse(finalAngles.y, finalAngles.x);
}

void Aimbot::moveMouse(float x, float y)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dx = x;
	input.mi.dy = y;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(INPUT));
}

Vector Aimbot::getRandomizedRecoil(CBaseEntity* local)
{
	QAngle punchAngles = local->GetPunchAngles() * tools.random(cvar::aimbot_rcs_min, cvar::aimbot_rcs_max);
	return (local->GetShotsFired() > 1 ? punchAngles : Vector(0, 0, 0));
}

bool Aimbot::getClosestHitbox(CBaseEntity* local, CBaseEntity* entity, Vector& dest)
{
	int bestHitbox = -1;
	float bestFov = 180.f;

	std::vector<int> hitboxes;
	hitboxes.push_back(HITBOX_HEAD);
	hitboxes.push_back(HITBOX_NECK);
	hitboxes.push_back(HITBOX_UPPER_CHEST);
	hitboxes.push_back(HITBOX_CHEST);
	hitboxes.push_back(HITBOX_BODY);
	// add more hitboxes if you wish

	for (auto hitbox : hitboxes)
	{
		Vector temp;
		if (!tools.getHitboxPosition(hitbox, temp, entity))
			continue;

		float fov = tools.getFov(viewAngles + getRandomizedRecoil(local), tools.computeAngle(local->GetEyePosition(), temp));
		if (fov < bestFov)
		{
			bestFov = fov;
			bestHitbox = hitbox;
		}
	}

	if (bestHitbox != -1)
	{
		if (!tools.getHitboxPosition(bestHitbox, dest, entity))
			return true;
	}

	return false;
}

int Aimbot::getBestTarget(CBaseEntity* local, CBaseCombatWeapon* weapon, Vector& dest)
{
	int bestTarget = -1;
	float bestFov = 180.f;

	interfaces::engine->GetViewAngles(viewAngles);

	for (int i = 1; i <= interfaces::engine->GetMaxClients(); i++)
	{
		if (i == local->GetIndex())
			continue;

		CBaseEntity* entity = interfaces::entitylist->GetClientEntity(i);
		if (!entity ||
			entity->IsDormant() ||
			entity->GetLifeState() != LIFE_ALIVE ||
			entity->IsProtected() ||
			entity->GetClientClass()->GetClassID() != CCSPlayer ||
			entity->GetTeamNum() == local->GetTeamNum())
			continue;

		Vector hitbox;
		if (getClosestHitbox(local, entity, hitbox))
			continue;

		float fov = tools.getFov(viewAngles + getRandomizedRecoil(local), tools.computeAngle(local->GetEyePosition(), hitbox));
		if (fov < bestFov && fov < cvar::aimbot_fov)
		{
			if (tools.isVisible(local->GetEyePosition(), hitbox, entity))
			{
				bestFov = fov;
				dest = hitbox;
				bestTarget = i;
			}
		}
	}

	return bestTarget;
}