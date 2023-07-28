#pragma once



enum class FactionType
{
    BLUE = 0,
    RED = 1,
    NONE = 2,
    END = 3,
};

enum class ObjectType {
    PLAYER,

    MELEE_MINION,
    CASTER_MINION,
    SIEGE_MINION,
    SUPER_MINION,

    RAPTORS,
    WOLF,
    KRUG,
    DRAGON,
    BARON,

    TOWER,
    INHIBITOR,
    NEXUS,

    PROJECTILE,

    END,
};

enum class LaneType {
    NONE,
    TOP,
    MID,
    BOTTOM,
    END,
};


enum WaitingStatus
{
    WAITING = 0,
    RUN = 1,
};
enum ChampionType
{
    NONE,
    BLITZCRANK,
    JINX,
    AMUMU,
    MALPHITE,
};

enum SkillType {
    AUTO_ATTACK,
    JINX_W,
    JINX_E,
    JINX_R,
};

enum class CC_TYPE
{
    NONE,
    STUN, // ±âÀý
    SLOW, // µÐÈ­
    SILENCE, // Ä§¹¬
    SNARE, // ¼Ó¹Ú
    BLEED, // ÃâÇ÷
    AIRBORNE, // ¿¡¾îº»
};

struct ObjectMove
{
public:
    struct MoveDir
    {
        float x;
        float y;
        float z;
    };

    struct Pos
    {
        float x;
        float y;
        float z;
    };
public:
    ObjectMove() {}
    ObjectMove(int _LV, float _HP, float _MP, float _AD, float _Defence, ObjectMove::MoveDir _moveDir, ObjectMove::Pos _pos, CC_TYPE _CCType)
        : LV(_LV)
        , HP(_HP)
        , MP(_MP)
        , AD(_AD)
        , Defence(_Defence)
        , moveDir(_moveDir)
        , pos(_pos)
        , CCType(_CCType)
    {}
    ~ObjectMove() {}

    int   LV;
    float HP;
    float MP;
    float AD;
    float Defence;

    MoveDir moveDir;
    Pos pos;
    CC_TYPE CCType;
};


struct ObjectInfo {
    ObjectInfo() {}
    ObjectInfo(uint64 _objectId, ObjectType _objectType, FactionType _factionType, LaneType _laneType, ObjectMove _objectMove)
        : objectId(_objectId)
        , objectType(_objectType)
        , factionType(_factionType)
        , laneType(_laneType)
        , objectMove(_objectMove)
    {}
    ~ObjectInfo() {}

    uint64 objectId;
    ObjectType objectType;
    FactionType factionType;
    LaneType    laneType;
    ObjectMove objectMove;
};

struct PlayerInfo
{
    uint64  id;
    FactionType faction;
    ChampionType champion;

    ObjectMove posInfo;
    // Vec3 Pos;
};

struct PlayerInfoPacket
{
    uint64  id;
    FactionType faction;
    ChampionType champion;
    bool host;
    ObjectMove posInfo;

    uint16 nickNameOffset;
    uint16 nickNameCount;

    struct NickNameItem {
        wchar_t nickname;
    };

    bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size) {
        if (nickNameOffset + nickNameCount * sizeof(NickNameItem) > packetSize)
            return false;

        size += nickNameCount * sizeof(NickNameItem);
        return true;
    }
};

struct AnimInfoPacket {
    uint16 animIdx;
    bool blend;
    float blendTime;

    uint16 animNameOffset;
    uint16 animNameCount;

    struct animNameItem {
        wchar_t animName;
    };

    bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size) {
        if (animNameOffset + animNameCount * sizeof(animNameItem) > packetSize)
            return false;

        size += animNameCount * sizeof(animNameItem);
        return true;
    }
};

struct SkillInfo {
    uint64 OwnerId;
    uint64 TargetId;
    SkillType skillType;
};

extern PlayerInfo MyPlayer;