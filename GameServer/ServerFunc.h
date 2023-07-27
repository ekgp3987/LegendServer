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
    STUN, // ±âÀý
    SLOW, // µÐÈ­
    SILENCE, // Ä§¹¬
    SNARE, // ¼Ó¹Ú
    BLEED, // ÃâÇ÷
    AIRBORNE, // ¿¡¾îº»
};

struct ObjectMove
{
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
    MoveDir moveDir;
    Pos pos;
};

struct ObjectInfo {
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

struct AnimInfo {
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