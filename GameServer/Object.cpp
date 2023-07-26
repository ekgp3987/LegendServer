#include "pch.h"
#include "Object.h"

atomic<uint64> Object::objectId = 0;

Object::Object(Object_Type _objectType)
	: objectType(_objectType)
	, ownerRoom(nullptr)
	, factionType(FactionType::END)
{
	objectId.fetch_add(1);
}

Object::~Object()
{
}
