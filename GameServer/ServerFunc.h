#pragma once



enum class FactionType
{
    BLUE = 0,
    RED = 1,
    END = 2,
};

enum class Object_Type {
    PLAYER,
    MINION,
    TOWER,
    MONSTER,
    END,
};

enum WaitingStatus
{
    WAITING = 0,
    RUN = 1,
};
enum ChampionType
{
    BLITZCRANK,
    JINX,
    AMUMU,
    MALPHITE,
};

struct PlayerMove
{
    enum PlayerState
    {
        IDLE = 0,
        MOVE = 1,
    };

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

    PlayerState state;
    MoveDir moveDir;
    Pos pos;
};

struct PlayerInfo
{
    uint64  id;
    FactionType faction;
    ChampionType champion;

    PlayerMove posInfo;
    // Vec3 Pos;
};

struct PlayerInfoPacket
{
    uint64  id;
    FactionType faction;
    ChampionType champion;
    bool host;

    PlayerMove posInfo;


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

extern PlayerInfo MyPlayer;