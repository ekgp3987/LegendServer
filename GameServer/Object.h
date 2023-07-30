#pragma once
#include "ServerFunc.h"

class GameSession;
class Room;

static atomic<uint64>	objectStaticId = 0;

class Object
{
public:
	Object(ObjectType _objectType);
	virtual ~Object();

public:
	uint64 GetObjectId() { return objectId; }
	Room* GetOwnerRoom() { return ownerRoom; }
	Faction GetFaction() { return factionType; }

	void SetOwnerRoom(Room* _ownerRoom) { ownerRoom = _ownerRoom; }
	void SetFaction(Faction _factionType) { factionType = _factionType; }

private:
	uint64							objectId=0;
	ObjectType				objectType;
	Room*							ownerRoom;
	Faction				       factionType;
};

