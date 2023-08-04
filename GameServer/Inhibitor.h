#pragma once
#include "Object.h"
class Inhibitor :
    public Object
{
public:
    Inhibitor();
    ~Inhibitor();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

