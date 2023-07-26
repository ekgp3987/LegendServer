#include "pch.h"
#include "Object.h"

Object::Object(ObjectType _objectType)
	: objectType(_objectType)
	, ownerRoom(nullptr)
	, factionType(FactionType::END)
{
	objectStaticId.fetch_add(1);
	objectId = objectStaticId;
}

Object::~Object()
{
}
