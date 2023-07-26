#pragma once
#include "ServerFunc.h"

class GameSession;
class Room;

class Object
{
public:
	Object(Object_Type _objectType);
	virtual ~Object();

public:
	uint64 GetObjectId() { return objectId; }
	Room* GetOwnerRoom() { return ownerRoom; }
	FactionType GetFaction() { return factionType; }

	void SetOwnerRoom(Room* _ownerRoom) { ownerRoom = _ownerRoom; }
	void SetFaction(FactionType _factionType) { factionType = _factionType; }

private:
	static atomic<uint64>	objectId;
	Object_Type				objectType;
	Room*							ownerRoom;
	FactionType					factionType;
};

