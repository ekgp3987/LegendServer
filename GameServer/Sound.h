#pragma once
#include "Object.h"
class Sound : public Object
{
public:
    Sound();
    ~Sound();

public:
    ObjectInfo& GetObjectInfo() { return objectInfo; }

private:
    ObjectInfo objectInfo;
};

