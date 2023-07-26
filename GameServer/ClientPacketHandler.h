#pragma once
#include "BufferReader.h"
#include "BufferWriter.h"
#include "ServerFunc.h"

enum
{
	S_TEST = 0,

	C_LOGIN = 1,
	S_LOGIN = 2,
	C_PICK_FACTION = 3,
	S_PICK_FACTION = 4,
	C_PICK_CHAMPION = 5,
	S_PICK_CHAMPION = 6,

	S_GAME_START = 7,

	C_PLAYER_UPDATE = 8,
	S_PLAYER_UPDATE = 9,

	C_MOVE = 10,
	S_MOVE = 11,
};

class ClientPacketHandler
{
public:
	static void HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_LOGIN(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_PICK_FACTION(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_PICK_CHAMPION(PacketSessionRef& session, BYTE* buffer, int32 len);
	static void Handle_C_MOVE(PacketSessionRef& session, BYTE* buffer, int32 len);

private:
	USE_LOCK;
};

template<typename T, typename C>
class PacketIterator
{
public:
	PacketIterator(C& container, uint16 index) : _container(container), _index(index) { }

	bool				operator!=(const PacketIterator& other) const { return _index != other._index; }
	const T& operator*() const { return _container[_index]; }
	T& operator*() { return _container[_index]; }
	T* operator->() { return &_container[_index]; }
	PacketIterator& operator++() { _index++; return *this; }
	PacketIterator		operator++(int32) { PacketIterator ret = *this; ++_index; return ret; }

private:
	C& _container;
	uint16			_index;
};

template<typename T>
class PacketList
{
public:
	PacketList() : _data(nullptr), _count(0) { }
	PacketList(T* data, uint16 count) : _data(data), _count(count) { }

	T& operator[](uint16 index)
	{
		ASSERT_CRASH(index < _count);
		return _data[index];
	}

	uint16 Count() { return _count; }

	// ranged-base for 지원
	PacketIterator<T, PacketList<T>> begin() { return PacketIterator<T, PacketList<T>>(*this, 0); }
	PacketIterator<T, PacketList<T>> end() { return PacketIterator<T, PacketList<T>>(*this, _count); }

private:
	T* _data;
	uint16		_count;
};

//===============================
//		이 밑은 패킷 구조체 모음입니다. |
// ==============================

#pragma pack(1)
// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem][victim victim][victim victim]
struct PKT_S_TEST
{
	struct BuffsListItem
	{
		uint64 buffId;
		float remainTime;

		// Victim List
		uint16 victimsOffset;
		uint16 victimsCount;
	};

	uint16 packetSize; // 공용 헤더
	uint16 packetId; // 공용 헤더
	uint64 id; // 8
	uint32 hp; // 4
	uint16 attack; // 2
	uint16 buffsOffset;
	uint16 buffsCount;
};
#pragma pack()

#pragma pack(1)
struct PKT_C_LOGIN
{
	struct NickNameStruct {
		wchar_t nickname;
	};

	uint16 packetSize;
	uint16 packetId;
	uint16 nicknameOffset;
	uint16 nicknameCount;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_LOGIN);
		if (packetSize < size)
			return false;

		if (nicknameOffset + nicknameCount * sizeof(BYTE) * 2 > packetSize)
			return false;

		size += nicknameCount * sizeof(BYTE) * 2;

		NickName nickname = GetNickName();

		if (size != packetSize)
			return false;

		return true;
	}

	using NickName = PacketList<NickNameStruct>;

	NickName GetNickName()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += nicknameOffset;
		return NickName(reinterpret_cast<PKT_C_LOGIN::NickNameStruct*>(data), nicknameCount);
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_LOGIN
{
	struct PlayerListItem
	{
		uint64 playerId;
		FactionType playerFaction;
		bool				host;

		uint16 nickNameOffset;
		uint16 nickNameCount;

		bool Validate(BYTE* packetStart, uint16 packetSize, OUT uint32& size)
		{
			if (nickNameOffset + nickNameCount * sizeof(BYTE)*2 > packetSize)
				return false;

			size += nickNameCount * sizeof(BYTE)*2;
			return true;
		}
	};

	struct NickNameStruct {
		wchar_t nickname;
	};

	uint16 packetSize;
	uint16 packetId;
	bool success;
	uint64 playerId;
	uint16 playerListoffset;
	uint16 playerListcount;	

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_S_LOGIN);
		if (packetSize < size)
			return false;

		if (playerListoffset + playerListcount * sizeof(PlayerListItem) > packetSize)
			return false;

		size += playerListcount * sizeof(PlayerListItem);

		PlayerList playerList = GetPlayerList();
		for (int32 i = 0; i < playerList.Count(); i++)
		{
			if (playerList[i].Validate((BYTE*)this, packetSize, OUT size) == false)
				return false;
		}

		if (size != packetSize)
			return false;

		return true;
	}

	using PlayerList = PacketList<PKT_S_LOGIN::PlayerListItem>;
	using NickNameList = PacketList<PKT_S_LOGIN::NickNameStruct>;

	PlayerList GetPlayerList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += playerListoffset;
		return PlayerList(reinterpret_cast<PKT_S_LOGIN::PlayerListItem*>(data), playerListcount);
	}

	NickNameList  GetNickNameList(PlayerListItem * playerList)
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += playerList->nickNameOffset;
		return NickNameList(reinterpret_cast<PKT_S_LOGIN::NickNameStruct*>(data), playerList->nickNameCount);
	}

};
#pragma pack()

#pragma pack(1)
struct PKT_C_PICK_FACTION
{
	uint16 packetSize;
	uint16 packetId;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_PICK_FACTION);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_PICK_FACTION
{
	uint16 packetSize;
	uint16 packetId;
	bool success;
	WaitingStatus waiting;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_S_PICK_FACTION);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_C_PICK_CHAMPION
{
	uint16 packetSize;
	uint16 packetId;
	ChampionType champion;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_PICK_CHAMPION);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_PICK_CHAMPION
{
	uint16 packetSize;
	uint16 packetId;
	bool	success;
	uint16 PlayerID;
	ChampionType champion;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_PICK_CHAMPION);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_GAME_START {
	uint16 packetSize;
	uint16 packetId;
	bool	success;
	uint16 playerInfoOffset;
	uint16 playerInfoCount;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_S_GAME_START);
		if (packetSize < size)
			return false;

		if (playerInfoOffset + playerInfoCount * sizeof(PlayerInfoPacket) > packetSize)
			return false;

		size += playerInfoCount * sizeof(PlayerInfoPacket);

		PlayerInfoList playerInfoList = GetPlayerInfoList();
		for (int32 i = 0; i < playerInfoList.Count(); i++)
		{
			if (playerInfoList[i].Validate((BYTE*)this, packetSize, OUT size) == false)
				return false;
		}

		if (size != packetSize)
			return false;

		return true;
	}

	using PlayerInfoList = PacketList<PlayerInfoPacket>;
	using NickNameList = PacketList<PlayerInfoPacket::NickNameItem>;

	PlayerInfoList GetPlayerInfoList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += playerInfoOffset;
		return PlayerInfoList(reinterpret_cast<PlayerInfoPacket*>(data), playerInfoCount);
	}

	NickNameList GetNickNameList(PlayerInfoPacket* playerInfoPacket)
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += playerInfoPacket->nickNameOffset;
		return NickNameList(reinterpret_cast<PlayerInfoPacket::NickNameItem*>(data), playerInfoPacket->nickNameCount);
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_C_MOVE
{
	uint16 packetSize;
	uint16 packetId;
	PlayerMove playerMove;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_C_MOVE);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

#pragma pack(1)
struct PKT_S_MOVE
{
	uint16 packetSize;
	uint16 packetId;
	uint64 playerId;
	PlayerMove playerMove;

	bool Validate()
	{
		uint32 size = 0;
		size += sizeof(PKT_S_MOVE);
		if (packetSize < size)
			return false;

		if (size != packetSize)
			return false;

		return true;
	}
};
#pragma pack()

//===============================
// 이 밑은 패킷 Write 클래스 모음입니다. |
// ==============================

#pragma pack(1)
class PKT_S_LOGIN_WRITE
{
public:
	using PlayerListItem = PKT_S_LOGIN::PlayerListItem;
	using PlayerList = PacketList< PKT_S_LOGIN::PlayerListItem>;
	using NickNameItem = PKT_S_LOGIN::NickNameStruct;
	using NickNameList = PacketList<PKT_S_LOGIN::NickNameStruct>;

	using PlayerFactionEnum = FactionType;

public:
	PKT_S_LOGIN_WRITE(bool _success, uint64 _playerId, PlayerFactionEnum _faction) {
		_sendBuffer = GSendBufferManager->Open(4096);
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_LOGIN>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_LOGIN;
		_pkt->success = _success;
		_pkt->playerId = _playerId;
		_pkt->playerListoffset = 0;
		_pkt->playerListcount = 0;
	}

	PlayerList ReservePlayerList(uint16 playersCount)
	{
		PlayerListItem* firstBuffsListItem = _bw.Reserve<PlayerListItem>(playersCount);
		_pkt->playerListoffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->playerListcount = playersCount;
		return PlayerList(firstBuffsListItem, playersCount);
	}

	NickNameList ReserveNickNameList(PlayerListItem* playersItem, uint16 nickNamesCount)
	{
		NickNameItem* firstVictimsListItem = _bw.Reserve<NickNameItem>(nickNamesCount);
		playersItem->nickNameOffset = (uint64)firstVictimsListItem - (uint64)_pkt;
		playersItem->nickNameCount = nickNamesCount;
		return NickNameList(firstVictimsListItem, nickNamesCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_LOGIN* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()

#pragma pack(1)
class PKT_S_PICK_FACTION_WRITE
{
public:
	PKT_S_PICK_FACTION_WRITE(bool _success, WaitingStatus _waiting)
	{
		_sendBuffer = GSendBufferManager->Open(4096);
		// 초기화
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_PICK_FACTION>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_PICK_FACTION;
		_pkt->success = _success;
		_pkt->waiting = _waiting;
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_PICK_FACTION* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()

#pragma pack(1)
class PKT_S_PICK_CHAMPION_WRITE
{
public:
	PKT_S_PICK_CHAMPION_WRITE(bool _success, uint16 _playerId, ChampionType _champion)
	{
		_sendBuffer = GSendBufferManager->Open(4096);
		// 초기화
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_PICK_CHAMPION>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_PICK_CHAMPION;
		_pkt->success = _success;
		_pkt->PlayerID = _playerId;
		_pkt->champion = _champion;
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_PICK_CHAMPION* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()

#pragma pack(1)
class PKT_S_GAME_START_WRITE
{
public:
	using PlayerInfoList = PacketList<PlayerInfoPacket>;
	using NickNameList = PacketList<PlayerInfoPacket::NickNameItem>;
	using NickNameItem = PlayerInfoPacket::NickNameItem;

	PKT_S_GAME_START_WRITE(bool _success)
	{
		_sendBuffer = GSendBufferManager->Open(4096);
		// 초기화
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_GAME_START>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_GAME_START;
		_pkt->success = _success;
	}

	PlayerInfoList ReservePlayerInfoList(uint16 playersCount)
	{
		PlayerInfoPacket* firstBuffsListItem = _bw.Reserve<PlayerInfoPacket>(playersCount);
		_pkt->playerInfoOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->playerInfoCount = playersCount;
		return PlayerInfoList(firstBuffsListItem, playersCount);
	}

	NickNameList ReserveNickNameList(PlayerInfoPacket* playerInfo, uint16 nickNameCount)
	{
		NickNameItem* firstBuffsListItem = _bw.Reserve<NickNameItem>(nickNameCount);
		playerInfo->nickNameOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		playerInfo->nickNameCount = nickNameCount;
		return NickNameList(firstBuffsListItem, nickNameCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_GAME_START* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()

#pragma pack(1)
class PKT_S_MOVE_WRITE
{
public:
	PKT_S_MOVE_WRITE(uint64 _playerId, PlayerMove _playerMove) {
		_sendBuffer = GSendBufferManager->Open(4096);
		// 초기화
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_MOVE>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_MOVE;
		_pkt->playerId = _playerId;
		_pkt->playerMove = _playerMove;
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_MOVE* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()





#pragma pack(1)
// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem][victim victim]
class PKT_S_TEST_WRITE
{
public:
	using BuffsListItem = PKT_S_TEST::BuffsListItem;
	using BuffsList = PacketList<PKT_S_TEST::BuffsListItem>;
	using BuffsVictimsList = PacketList<uint64>;

	PKT_S_TEST_WRITE(uint64 id, uint32 hp, uint16 attack)
	{
		_sendBuffer = GSendBufferManager->Open(4096);
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		_pkt = _bw.Reserve<PKT_S_TEST>();
		_pkt->packetSize = 0; // To Fill
		_pkt->packetId = S_TEST;
		_pkt->id = id;
		_pkt->hp = hp;
		_pkt->attack = attack;
		_pkt->buffsOffset = 0; // To Fill
		_pkt->buffsCount = 0; // To Fill
	}

	BuffsList ReserveBuffsList(uint16 buffCount)
	{
		BuffsListItem* firstBuffsListItem = _bw.Reserve<BuffsListItem>(buffCount);
		_pkt->buffsOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->buffsCount = buffCount;
		return BuffsList(firstBuffsListItem, buffCount);
	}

	BuffsVictimsList ReserveBuffsVictimsList(BuffsListItem* buffsItem, uint16 victimsCount)
	{
		uint64* firstVictimsListItem = _bw.Reserve<uint64>(victimsCount);
		buffsItem->victimsOffset = (uint64)firstVictimsListItem - (uint64)_pkt;
		buffsItem->victimsCount = victimsCount;
		return BuffsVictimsList(firstVictimsListItem, victimsCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_TEST* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};
#pragma pack()