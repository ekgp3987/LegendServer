#pragma once



enum class Faction
{
    NONE,
    RED,
    BLUE,
    END,
};

// LoL ���� ������ �÷��̾ ��ȣ�ۿ��� �� �ִ� ��� ������Ʈ
enum class UnitType
{
    CHAMPION,

    MELEE_MINION,
    RANGED_MINION,
    SIEGE_MINION,
    SUPER_MINION,

    RAPTORS, // ����, �ڽĵ�
    WOLF,    // ����, ����
    KRUG,    // ���ź� ū��, ���� ��
    DRAGON,
    BARON,   // ��������

    JUNGLE_RED,  // ���� ��������
    JUNGLE_BLUE, // Ǫ�� �ļ���

    TURRET,
    INHIBITOR,
    NEXUS,

    PROJECTILE,
    EFFECT,

    END,
};


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
    BASIC_ATTACK,		// ��� ������ ��Ÿ (�̴Ͼ�/���۸��� �⺻���� ����)
    JINX_Q,
    JINX_W,
    JINX_E,
    JINX_R,
    DARIUS_Q,
    DARIUS_W,
    DARIUS_E,
    DARIUS_R,
};

// ���� �����
enum class CC
{
    CLEAR = 0,
    SLOW = 1 << 0,
    SILENCE = 1 << 1,
    ROOT = 1 << 2,
    STUN = 1 << 3,
    AIRBORNE = 1 << 4,
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
    ObjectMove(int _LV, float _HP, float _MP, float _AttackPower, float _DefencePower, ObjectMove::MoveDir _moveDir, ObjectMove::Pos _pos, CC _CC)
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

    MoveDir moveDir;
    Pos pos;
    CC  CC;
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
    bool bRepeat;
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

struct SkillInfo
{
public:
    struct vec3Server
    {
        float x;
        float y;
        float z;
    };

    UINT64		SkillId;    // ��ų ����ü id
    UINT64		OwnerId;    // ��ų�� ����� �÷��̾� id
    UINT64		TargetId;   // Ÿ�����Ͻ� ���� �÷��̾� id (��Ÿ���� ��� -1)

    UINT16		SkillLevel; // ��ų����
    SkillType	skillType;  // � ��ų���� ��Ƶ� enum �� �ϳ�

    vec3Server	offsetPos;  // �������� offset

    // TargetId == -1�� ��� ���
    bool		UseMousePos;    // mousePos�� ���ܾ��ϴ� ��ų
    vec3Server  MousePos;

    bool		UseMouseDir;    // �� �������� ���ܾ� �ϴ� ��ų
    vec3Server  MouseDir;
};

enum WaitingStatus
{
    WAITING = 0,
    RUN = 1,
};

extern PlayerInfo MyPlayer;