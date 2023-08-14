#pragma once
#include "Object.h"

#define I

class Inhibitor :
    public Object
{
public:
    Inhibitor();
    ~Inhibitor();

public:
    ObjectInfo& GetPrevObjectInfo() { return prevObjectInfo; }
    ObjectInfo& GetCurObjectInfo() { return curObjectInfo; }

    void SetPrevObjectInfo(ObjectInfo _prevObjectInfo) { prevObjectInfo = _prevObjectInfo; }
    void SetCurObjectInfo(ObjectInfo _curObjectInfo) { curObjectInfo = _curObjectInfo; }

private:
    ObjectInfo prevObjectInfo;
    ObjectInfo curObjectInfo;
};

