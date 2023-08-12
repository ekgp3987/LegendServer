#pragma once

enum class DimensionType
{
    TwoD,
    ThreeD,
};

enum class Faction
{
    NONE,
    RED,
    BLUE,
    END,
};

// LoL 게임 내에서 플레이어가 상호작용할 수 있는 모든 오브젝트
enum class UnitType
{
    CHAMPION,

    MELEE_MINION,
    RANGED_MINION,
    SIEGE_MINION,
    SUPER_MINION,

    SOUTH_GROMP,
    SOUTH_MURKWOLF,
    SOUTH_MURKWOLF_MINI_L,
    SOUTH_MURKWOLF_MINI_R,
    SOUTH_RAZORBEAK,
    SOUTH_RAZORBEAK_MINI_1,
    SOUTH_RAZORBEAK_MINI_2,
    SOUTH_RAZORBEAK_MINI_3,
    SOUTH_RAZORBEAK_MINI_4,
    SOUTH_RAZORBEAK_MINI_5,
    SOUTH_KRUG,
    SOUTH_KRUG_MINI,
    SOUTH_RED,      // 붉은 덩굴정령
    SOUTH_BLUE ,    // 푸른 파수꾼

    NORTH_GROMP,
    NORTH_MURKWOLF,
    NORTH_MURKWOLF_MINI_L,
    NORTH_MURKWOLF_MINI_R,
    NORTH_RAZORBEAK,
    NORTH_RAZORBEAK_MINI_1,
    NORTH_RAZORBEAK_MINI_2,
    NORTH_RAZORBEAK_MINI_3,
    NORTH_RAZORBEAK_MINI_4,
    NORTH_RAZORBEAK_MINI_5,
    NORTH_KRUG,
    NORTH_KRUG_MINI,
    NORTH_RED,             // 붉은 덩굴정령
    NORTH_BLUE,            // 푸른 파수꾼

    DRAGON,
    BARON,   // 렌더링만

    TURRET,
    INHIBITOR,
    NEXUS,

    PROJECTILE,
    EFFECT,

    SOUND,

    END,
};

//enum class UnitType
//{
//    CHAMPION,
//
//    MELEE_MINION,
//    RANGED_MINION,
//    SIEGE_MINION,
//    SUPER_MINION,
//
//    RAPTORS, // 엄마, 자식들
//    WOLF,    // 대장, 부하
//    KRUG,    // 돌거북 큰애, 작은 애
//    DRAGON,
//    BARON,   // 렌더링만
//
//    JUNGLE_RED,  // 붉은 덩굴정령
//    JUNGLE_BLUE, // 푸른 파수꾼
//
//    TURRET,
//    INHIBITOR,
//    NEXUS,
//
//    PROJECTILE,
//    EFFECT,
//
//    END,
//};


enum class Lane
{
    NONE,
    TOP,
    MID,
    BOTTOM,
    END,
};


enum ChampionType
{
    NONE,
    BLITZCRANK,
    JINX,
    AMUMU,
    MALPHITE,
};

enum class SkillType
{
    NONE,
    BASIC_ATTACK,      // 모든 종류의 평타 (미니언/정글몹의 기본공격 포함)
    JINX_Q,
    JINX_W,
    JINX_E,
    JINX_R,
    DARIUS_Q,
    DARIUS_W,
    DARIUS_E,
    DARIUS_R,
    MALPHITE_Q,
    MALPHITE_W,
    MALPHITE_E,
    MALPHITE_R,
};

// 군중 제어기
enum CC : uint32_t
{
    NO_CC = 0b00000000,
    SLOW = 0b00000001,
    SILENCE = 0b00000010,
    ROOT = 0b00000100,
    STUN = 0b00001000,
    AIRBORNE = 0b00010000,
};

enum TEX_PARAM
{
    TEX_0,
    TEX_1,
    TEX_2,
    TEX_3,
    TEX_4,
    TEX_5,
    TEX_6,
    TEX_7,

    TEX_CUBE_0,
    TEX_CUBE_1,

    TEX_ARR_0,
    TEX_ARR_1,

    TEX_END,
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
    ObjectMove(int _LV, float _HP, float _MP, float _AttackPower, float _DefencePower, float MaxHP, float MaxMP, bool bUnitDead, ObjectMove::MoveDir _moveDir, ObjectMove::Pos _pos, UINT _CC)
        : LV(_LV)
        , HP(_HP)
        , MP(_MP)
        , AttackPower(_AttackPower)
        , DefencePower(_DefencePower)
        , moveDir(_moveDir)
        , pos(_pos)
        , CC(_CC)
    {}
    ~ObjectMove() {}

    int   LV;
    float HP;
    float MP;
    float AttackPower;
    float DefencePower;
    float MaxHP;
    float MaxMP;
    bool  bUnitDead;

    MoveDir moveDir;
    Pos pos;
    UINT  CC;
};

struct ObjectInfo {
    ObjectInfo() {}
    ObjectInfo(uint64 _objectId, UnitType _unitType, Faction _faction, Lane _lane, ObjectMove _objectMove)
        : objectId(_objectId)
        , unitType(_unitType)
        , faction(_faction)
        , lane(_lane)
        , objectMove(_objectMove)
    {}
    ~ObjectInfo() {}

    uint64     objectId;
    UnitType   unitType;
    Faction    faction;
    Lane       lane;
    ObjectMove objectMove;
};

struct PlayerInfo
{
    uint64  id;
    wstring nickname;
    Faction faction;
    ChampionType champion;
    bool host;

    ObjectMove posInfo;
};

struct PlayerInfoPacket
{
    uint64  id;
    Faction faction;
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
    uint16 targetId;
    bool bRepeat;            // 애니메이션 반복 여부
    bool bRepeatBlend;  // 블렌드 반복 여부
    bool blend;               // 블렌드 사용 여부
    float blendTime;
    float   animSpeed;      // 애니메이션 속도

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

struct SkillInfo
{
public:
    struct vec3Server
    {
        float x;
        float y;
        float z;
    };
    UINT64    SkillId;    // 스킬 투사체 id
    UINT64    OwnerId;    // 스킬을 사용한 플레이어 id
    UINT64    TargetId;   // 타겟팅일시 맞을 플레이어 id (논타겟일 경우 -1)

    UINT16    SkillLevel; // 스킬레벨
    SkillType skillType;  // 어떤 스킬인지 모아둔 enum 중 하나

    vec3Server	offsetPos;  // 중점기준 offset
    int        projectileCount = 1; // 투사체가 몇개인지

    // TargetId == -1일 경우 사용
    bool		UseMousePos;    // mousePos에 생겨야하는 스킬
    vec3Server  MousePos;

    bool		UseMouseDir;    // 이 방향으로 생겨야 하는 스킬
    vec3Server  MouseDir;
};

enum WaitingStatus
{
    WAITING = 0,
    RUN = 1,
};

struct SoundInfoPacket
{
    struct vec3Server
    {
        float x;
        float y;
        float z;
    };

    DimensionType dimensionType;
    Faction faction;
    int iLoopCount;
    float fVolume;
    bool bOverlap;
    float fRange;
    vec3Server soundPos;

    uint16 soundNameOffset;
    uint16 soundNameCount;

    struct soundNameItem {
        wchar_t soundName;
    };

    bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size) {
        if (soundNameOffset + soundNameCount * sizeof(soundNameItem) > packetSize)
            return false;

        size += soundNameCount * sizeof(soundNameItem);
        return true;
    }
};

struct KDACSInfo
{
    UINT64      killerId;
    UINT64      victimId;
    UnitType   deadObjUnitType;
};

struct MtrlInfoPacket
{
    UINT64 targetId;
    int iMtrlIndex;
    TEX_PARAM  tex_param;
    
    uint16 texNameOffset;
    uint16 texNameCount;

    struct texNameItem {//예시 L"texture\\FBXTexture\\alphaTex.png"
        wchar_t texName;
    };
    
    bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size) {
        if (texNameOffset + texNameCount * sizeof(texNameItem) > packetSize)
            return false;

        size += texNameCount * sizeof(texNameItem);
        return true;
    }
};

extern PlayerInfo MyPlayer;