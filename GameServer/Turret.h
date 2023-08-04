#pragma once
#include "Object.h"
class Turret :
    public Object
{
public:
    Turret();
    ~Turret();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

