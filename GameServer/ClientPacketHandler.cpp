#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"

#include "Player.h"
#include "Room.h"
#include "Projectile.h"

#include <string>
#include <codecvt>

void ClientPacketHandler::HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	if (header.id == C_PICK_CHAMPION)
		int a = 0;

	switch (header.id)
	{
	case C_LOGIN:
		Handle_C_LOGIN(session, buffer, len);
		break;
	case C_PICK_FACTION:
		Handle_C_PICK_FACTION(session, buffer, len);
		break;
	case C_PICK_CHAMPION:
		Handle_C_PICK_CHAMPION(session, buffer, len);
		break;
	case C_PLAYER_MOVE:
		Handle_C_PLAYER_MOVE(session, buffer, len);
		break;
	case C_OBJECT_ANIM:
		Handle_C_OBJECT_ANIM(session, buffer, len);
		break;
	case C_OBJECT_MOVE:
		Handle_C_OBJECT_MOVE(session, buffer, len);
		break;
	case C_SKILL_PROJECTILE:
		Handle_C_SKILL_PROJECTILE(session, buffer, len);
		break;
	case C_SKILL_HIT:
		Handle_C_SKILL_HIT(session, buffer, len);
		break;
	case C_SKILL_DAMAGE:
		Handle_C_SKILL_DAMAGE(session, buffer, len);
		break;
	case C_SKILL_CC:
		Handle_C_SKILL_CC(session, buffer, len);
		break;
	default:
		break;
	}
}

void ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	PKT_C_LOGIN* pkt = reinterpret_cast<PKT_C_LOGIN*>(buffer);

	if (pkt->Validate() == false)
		return;

	PKT_C_LOGIN::NickName nickNames = pkt->GetNickName();

	cout << "닉네임의 길이는 " << nickNames.Count() << "입니다."<<endl;

	wstring resultNickName = L"";
	for (auto& nickname : nickNames)
	{
		resultNickName.push_back(nickname.nickname);
	}
	
	wcout << "닉네임은 " << resultNickName << "입니다." << endl;

	bool success = true;
	FactionType faction = FactionType::BLUE;

	//플레이어 생성	
	
		PlayerRef playerRef = MakeShared<Player>();
		playerRef->SetName(resultNickName);
		playerRef->SetOwnerSession(gameSession);
		playerRef->SetOwnerRoom(&GRoom);
		gameSession->SetPlayer(playerRef);

		//플레이어 룸에 넣어줌
		if (GRoom.GetBluePlayerSize() > GRoom.GetRedPlayerSize()) {
			GRoom.RedEnter(playerRef, gameSession);
			faction = FactionType::RED;
			playerRef->SetFaction(faction);
		}
		else if (GRoom.GetRedPlayerSize() >= GRoom.GetBluePlayerSize()) {
			GRoom.BlueEnter(playerRef, gameSession);
			faction = FactionType::BLUE;
			playerRef->SetFaction(faction);
		}
	
	
	uint64 playerNum = GRoom.GetPlayerSize();
	if (playerNum > 4)
		success = false;
	
	bool host = false;
	if (playerNum == 1)
		host = true;

	playerRef->SetHost(host);

	PKT_S_LOGIN_WRITE pktWriter(success, playerRef->GetObjectId(), faction);
	PKT_S_LOGIN_WRITE::PlayerList playerList = pktWriter.ReservePlayerList(playerNum);
	map<uint64, PlayerRef> players = GRoom.GetPlayers();
	int i = 0;

	for (auto &p : players) {
		playerList[i] = { p.second->GetObjectId(), p.second->GetFaction(), p.second->GetHost()};
		wstring playerNickName = p.second->GetName();
		PKT_S_LOGIN_WRITE::NickNameList nick = pktWriter.ReserveNickNameList(&playerList[i], playerNickName.length());
		for (int j = 0; j < playerNickName.length(); j++) {
			nick[j] = { playerNickName[j] };
		}
		i++;
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_PICK_FACTION(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "HANDLE_C_PICK_FACTION에 진입함" << endl;

	PKT_C_PICK_FACTION* pkt = reinterpret_cast<PKT_C_PICK_FACTION*>(buffer);

	if (pkt->Validate() == false)
		return;
	
	bool success = false;

	//여기 나중에 ==4로 수정해주기 4명이 꼭 있어야만 플레이 되게끔
	if (GRoom.GetPlayerSize() < 5)
		success = true;

	PKT_S_PICK_FACTION_WRITE pktWriter(true, WaitingStatus::RUN);
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	if(success == true)
		GRoom.GameStart(sendBuffer,3000);
}

void ClientPacketHandler::Handle_C_PICK_CHAMPION(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "Handle_C_PICK_CHAMPION에 진입함" << endl;

	PKT_C_PICK_CHAMPION* pkt = reinterpret_cast<PKT_C_PICK_CHAMPION*>(buffer);

	if (pkt->Validate() == false)
		return;

	gameSession->GetPlayer()->SetChampionType(pkt->champion);

	bool success = false;

	//여기 나중에 ==4로 수정해주기 4명이 꼭 있어야만 플레이 되게끔
	if (GRoom.GetPlayerSize() < 5)
		success = true;

	PKT_S_PICK_CHAMPION_WRITE pktWriter(success, gameSession->GetPlayer()->GetObjectId(), pkt->champion);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_PLAYER_MOVE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "Handle_C_MOVE에 진입함" << endl;

	PKT_C_PLAYER_MOVE* pkt = reinterpret_cast<PKT_C_PLAYER_MOVE*>(buffer);

	if (pkt->Validate() == false)
		return;

	ObjectMove playerMove = pkt->playerMove;

	PKT_S_PLAYER_MOVE_WRITE pktWriter(gameSession->GetPlayer()->GetObjectId(), playerMove);
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_OBJECT_ANIM(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_OBJECT_ANIM에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "방장이 아닙니다." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_OBJECT_ANIM* pkt = reinterpret_cast<PKT_C_OBJECT_ANIM*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "C_OBJECT_ANIM Validate 실패" << endl;
		return;
	}

	uint64 objectId = pkt->targetId;
	AnimInfoPacket animInfo = pkt->animInfo;

	cout << "해당 오브젝트 ID : " << objectId<< endl;

	PKT_C_OBJECT_ANIM::AnimNameList animNames = pkt->GetAnimNameList();

	wstring resultAnimName = L"";
	for (auto& animName : animNames) {
		resultAnimName.push_back(animName.animName);
	}

	PKT_S_OBJECT_ANIM_WRITE pktWriter(objectId, animInfo);

	PKT_S_OBJECT_ANIM_WRITE::AnimNameList animName = pktWriter.ReserveAnimNameList(resultAnimName.size());
	for (int i = 0; i < resultAnimName.size(); i++) {
		animName[i] = { resultAnimName[i] };
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_OBJECT_MOVE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_OBJECT_MOVE에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "방장이 아닙니다." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_OBJECT_MOVE* pkt = reinterpret_cast<PKT_C_OBJECT_MOVE*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_OBJECT_MOVE Validate 실패" << endl;
		return;
	}

	uint64 objectId = pkt->objectId;
	ObjectMove objectMobe = pkt->objectMove;

	cout << "오브젝트 ID : " << objectId << endl;

	PKT_S_OBJECT_MOVE_WRITE pktWriter(objectId, objectMobe);	

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_SKILL_PROJECTILE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_PROJECTILE에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "방장이 아닙니다." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_SKILL_PROJECTILE* pkt = reinterpret_cast<PKT_C_SKILL_PROJECTILE*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_PROJECTILE Validate 실패" << endl;
		return;
	}
	//여기까지 패킷 받는 부분 밑에서 보내는 패킷 작성


	Projectile* projectile = new Projectile;

	SkillInfo skillInfo = pkt->skillInfo;

	PKT_S_SKILL_PROJECTILE_WRITE pktWriter(projectile->GetObjectId(), skillInfo);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	delete projectile;
}

void ClientPacketHandler::Handle_C_SKILL_HIT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_HIT에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "방장이 아닙니다." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_SKILL_HIT* pkt = reinterpret_cast<PKT_C_SKILL_HIT*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_HIT Validate 실패" << endl;
		return;
	}

	PKT_S_SKILL_HIT_WRITE pktWriter(pkt->objecId, pkt->skillInfo);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_SKILL_DAMAGE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_DAMAGE에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "방장이 아닙니다." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_SKILL_DAMAGE* pkt = reinterpret_cast<PKT_C_SKILL_DAMAGE*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_DAMAGE Validate 실패" << endl;
		return;
	}

	PKT_S_SKILL_DAMAGE_WRITE pktWriter(pkt->objecId, pkt->damage);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_SKILL_CC(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_CC에 진입" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "방장이 아닙니다." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_SKILL_CC* pkt = reinterpret_cast<PKT_C_SKILL_CC*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_CC Validate 실패" << endl;
		return;
	}

	PKT_S_SKILL_CC_WRITE pktWriter(pkt->objectId, pkt->CCType, pkt->time);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}