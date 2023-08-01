#include "pch.h"
#include "Object.h"

Object::Object(UnitType _unitType)
	: unitType(_unitType)
	, ownerRoom(nullptr)
	, factionType(Faction::END)
{
	objectStaticId.fetch_add(1);
	objectId = objectStaticId;
}

Object::~Object()
{
}
