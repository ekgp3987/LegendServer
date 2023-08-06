#pragma once

#define WIN32_LEAN_AND_MEAN // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

#ifdef _DEBUG
#pragma comment(lib, "Debug\\ServerCore.lib")
#else
#pragma comment(lib, "Release\\ServerCore.lib")
#endif

#include "CorePch.h"

using GameSessionRef = shared_ptr<class GameSession>;
using PlayerRef = shared_ptr<class Player>;
using RoomRef = shared_ptr<class Room>;

using NexusRef = shared_ptr<class Nexus>;
using InhibitorRef = shared_ptr<class Inhibitor>;
using TurretRef = shared_ptr<class Turret>;
using MinionRef = shared_ptr<class Minion>;
using MonsterRef = shared_ptr<class Monster>;

using ObjectRef = shared_ptr<class Object>;