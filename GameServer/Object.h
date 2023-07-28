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
	FactionType GetFaction() { return factionType; }

	void SetOwnerRoom(Room* _ownerRoom) { ownerRoom = _ownerRoom; }
	void SetFaction(FactionType _factionType) { factionType = _factionType; }

private:
	uint64							objectId=0;
	ObjectType				objectType;
	Room*							ownerRoom;
	FactionType					factionType;
};

