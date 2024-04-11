#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

#define M_PI 3.14159265358

using namespace std;

struct Vector2 { float x, y; };
class Vector3 {
public:
	float x, y, z;
	Vector3() {};
	Vector3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
	Vector3 operator + (const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
	Vector3 operator - (const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
	Vector3 operator * (const float& rhs) const { return Vector3(x * rhs, y * rhs, z * rhs); }
	Vector3 operator / (const float& rhs) const { return Vector3(x / rhs, y / rhs, z / rhs); }
	Vector3& operator += (const Vector3& rhs) { return *this = *this + rhs; }
	Vector3& operator -= (const Vector3& rhs) { return *this = *this - rhs; }
	Vector3& operator *= (const float& rhs) { return *this = *this * rhs; }
	Vector3& operator /= (const float& rhs) { return *this = *this / rhs; }
	float Length() const { return sqrtf(x * x + y * y + z * z); }
	Vector3 Normalize() const { return *this * (1 / Length()); }
	float Distance(const Vector3& o) { Vector3 self(this->x, this->y, this->z); Vector3 diff = self - o; return sqrt((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z)); }
};

class Ent {
public:
	char pad_0094[0x94]; //0x0000
	int Health; //0x0094
	char pad_0091[0x4];
	int Team; //0x0x9C
	char pad_0000[0x94]; //0x012C
	Vector3 Velocity; //0x0130
	char pad_0124[0x120]; //0x025C
	Vector3 Pos; //0x0260 m_vecOrigin
	Vector2 CamAngles; //0x026C
};

class Client {
public:

	uintptr_t* GetLocalPlayer () { //works
		getLocalPlayer = (GetLocalPlayer_)(ModBase + 0x000A23E0);
		return getLocalPlayer();
	}

	Ent* GetEntity(int id) { //works
		return (Ent*)*(uintptr_t*)(GetEntityList() + (id * 4));
	}


	uintptr_t* GetEntityList() { //works
		return (uintptr_t*)(GetModuleBase() + 0x4D5AE4);
	}

	Vector3 GetOrigin() { //works
		Ent* lPlr = (Ent*)GetLocalPlayer();
		return lPlr->Pos;
	}

	Vector3 GetVecViewOffset() {//works
		return *(Vector3*)((uintptr_t)GetLocalPlayer() + 0xE8);
	}
	
private:
	uintptr_t ModBase = GetModuleBase();

	typedef uintptr_t*(__cdecl* GetLocalPlayer_)();
	GetLocalPlayer_ getLocalPlayer;

	uintptr_t GetModuleBase() {
		return (uintptr_t)GetModuleHandle("client.dll");
	}
};

class Engine {
public:

	int GetPlayerCount() { //works
		return (*(int*)(Modbase + 0x5EC82C));
	}

	int GetBotCount() { //works
		return (*(int*)(Modbase + 0x5EC82C)) - 1;
	}

	void WriteViewAngles(Vector2 newAngles) {//works
		uintptr_t* xPtr = (uintptr_t*)(Modbase + 0x47C33C);
		uintptr_t* yPtr = (uintptr_t*)(Modbase + 0x47C340);
		*(float*)xPtr = newAngles.x, *(float*)yPtr = newAngles.y;
	}

private:
	uintptr_t Modbase = GetModuleBase();

	uintptr_t GetModuleBase() {
		return (uintptr_t)GetModuleHandle("engine.dll");
	}
};

Ent* GetClosestEnemy(Client client, Engine engine) { //work
	int PlayerCount = engine.GetPlayerCount();
	Ent* lPlr = (Ent*)client.GetLocalPlayer();

	int index = -1;
	float distance = 100000;
	if (PlayerCount <= 0) {
		std::cout << "player count: " << PlayerCount << std::endl;
		return NULL;
	}
	for (int i = 1; i < PlayerCount; i++) {
		Ent* tempEnt = client.GetEntity(i);
		if (tempEnt->Health <= 1) {
			continue;
		}
		if ((uintptr_t*)tempEnt == client.GetLocalPlayer()) {
			continue;
		}
		if (tempEnt->Team == lPlr->Team) {
			continue;
		}
		float tempDistance = lPlr->Pos.Distance(tempEnt->Pos);
		if (tempDistance < distance) {
			distance = tempDistance;
			index = i;
		}
	}
	if (index != -1) {
		return client.GetEntity(index);
	} else {
		std::cout << "Index: " << index << std::endl;
		return NULL;
	}
}

void Aimbot(Client client, Engine engine) {
	Ent* lPlr = (Ent*)client.GetLocalPlayer();
	Ent* target = GetClosestEnemy(client, engine);
	if (target) {
		Vector3 src = lPlr->Pos;
		Vector3 dst = target->Pos;
		Vector3 myPos = (src + client.GetVecViewOffset());

		Vector3 deltaVec = { dst.x - myPos.x, dst.y - myPos.y, dst.z - myPos.z };

		float deltaVecLength = sqrt(deltaVec.x * deltaVec.x + deltaVec.y * deltaVec.y + deltaVec.z * deltaVec.z);

		float pitch = -asin(deltaVec.z / deltaVecLength) * (180 / M_PI);
		float yaw = atan2(deltaVec.y, deltaVec.x) * (180 / M_PI);

		engine.WriteViewAngles(Vector2{ pitch, yaw });
		this_thread::sleep_for(chrono::milliseconds(500));
	}
	else {
		std::cout << "Null Ent" << std::endl;
	}
}

DWORD WINAPI MainThread(HMODULE hModule) {
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);

	Client client;
	Engine engine;

	bool bAimbot = false;

	while (1) {
		if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
			bAimbot = !bAimbot;
		}

		if (bAimbot) {
			Aimbot(client,engine);
		}

		if (GetAsyncKeyState(VK_NUMPAD0) & 1) {
			break;
		}
		this_thread::sleep_for(chrono::milliseconds(1500));
	}

	fclose(f);
	FreeConsole();
	FreeLibraryAndExitThread(hModule,NULL);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, NULL, 0, NULL);
		break;
	default:
		break;
	}
	return true;
}