#pragma once
#include "ServerFunc.h"

class GameSession;
class Room;

static atomic<uint64>	objectStaticId = 0;

class Object
{
public:
	Object(UnitType _unitType);
	virtual ~Object();

public:
	uint64 GetObjectId() { return objectId; }
	Room* GetOwnerRoom() { return ownerRoom; }
	Faction GetFaction() { return factionType; }

	void SetOwnerRoom(Room* _ownerRoom) { ownerRoom = _ownerRoom; }
	void SetFaction(Faction _factionType) { factionType = _factionType; }

private:
	uint64							objectId=0;
	UnitType					   unitType;
	Room*							ownerRoom;
	Faction				       factionType;
};

