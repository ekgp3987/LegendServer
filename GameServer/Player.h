#pragma once
#include "ServerFunc.h"

class GameSession;
class Room;

class Player
{
public:
	Player();
	~Player();

public:
	uint64 GetPlayerId() { return playerId; }
	wstring GetName() { return name; }
	weak_ptr<GameSession> GetOwnerSession() { return ownerSession.lock(); }
	Room* GetOwnerRoom() { return ownerRoom; }
	FactionType GetPlayerFaction() { return factionType; }
	ChampionType GetChampionType() { return championType; }

	void SetPlayerId(uint64 _playerId) { playerId = _playerId; }
	void SetName(wstring _name) { name = _name; }
	void SetOwnerSession(weak_ptr<GameSession> _ownerSession) { ownerSession = _ownerSession; }
	void SetOwnerRoom(Room* _ownerRoom) { ownerRoom = _ownerRoom; }
	void SetPlayerFaction(FactionType _factionType) { factionType = _factionType; }
	void SetChampionType(ChampionType _championType) { championType = _championType; }

private:
	uint64					playerId = 0;
	wstring					name;
	weak_ptr<GameSession>	ownerSession; // Cycle
	Room*					ownerRoom;
	FactionType			factionType;
	ChampionType	championType;
};

