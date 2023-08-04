#pragma once
#include "Object.h"

class Nexus : public Object
{
public:
    Nexus();
    ~Nexus();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

