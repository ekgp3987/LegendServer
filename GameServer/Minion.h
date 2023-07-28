#pragma once
#include "Object.h"
class Minion :
    public Object
{
public:
    Minion();
    ~Minion();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

