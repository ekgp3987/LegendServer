#pragma once
#include "ServerFunc.h"
#include "Object.h"

class GameSession;
class Room;

class Player : public Object
{
public:
	Player();
	~Player();

public:
	wstring GetName() { return name; }
	weak_ptr<GameSession> GetOwnerSession() { return ownerSession.lock(); }	
	ChampionType GetChampionType() { return championType; }
	bool	GetHost() { return host; }


	void SetName(wstring _name) { name = _name; }
	void SetOwnerSession(weak_ptr<GameSession> _ownerSession) { ownerSession = _ownerSession; }	
	void SetChampionType(ChampionType _championType) { championType = _championType; }
	void SetHost(bool _host) { host = _host; }

private:
	wstring					name;
	weak_ptr<GameSession>	ownerSession; // Cycle
	ChampionType	championType;
	bool						host;
};

