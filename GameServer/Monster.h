#pragma once
#include "Object.h"
#include "ServerFunc.h"

class Monster :
    public Object
{
public:
    Monster(UnitType _monsterType);
    ~Monster();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

