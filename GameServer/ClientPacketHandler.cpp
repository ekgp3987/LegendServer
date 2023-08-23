#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"

#include "Player.h"
#include "Room.h"
#include "Projectile.h"
#include "Sound.h"

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
	case C_DESPAWN_OBJECT:
		Handle_C_DESPAWN_OBJECT(session, buffer, len);
		break;
	case C_KDA_CS:
		Handle_C_KDA_CS(session, buffer, len);
		break;
	case C_SOUND:
		Handle_C_SOUND(session, buffer, len);
		break;
	case C_OBJECT_MTRL:
		Handle_C_OBJECT_MTRL(session, buffer, len);
		break;
	case C_CHAT:
		Handle_C_CHAT(session, buffer, len);
		break;
	case C_EFFECT:
		Handle_C_EFFECT(session, buffer, len);
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

	cout << "�г����� ���̴� " << nickNames.Count() << "�Դϴ�."<<endl;

	wstring resultNickName = L"";
	for (auto& nickname : nickNames)
	{
		resultNickName.push_back(nickname.nickname);
	}
	
	wcout << "�г����� " << resultNickName << "�Դϴ�." << endl;

	bool success = true;
	Faction faction = Faction::BLUE;

	//�÷��̾� ����	
	
		PlayerRef playerRef = MakeShared<Player>();
		playerRef->SetName(resultNickName);
		playerRef->SetOwnerSession(gameSession);
		playerRef->SetOwnerRoom(&GRoom);
		gameSession->SetPlayer(playerRef);

		//�÷��̾� �뿡 �־���
		if (GRoom.GetBluePlayerSize() > GRoom.GetRedPlayerSize()) {
			GRoom.RedEnter(playerRef, gameSession);
			faction = Faction::RED;
			playerRef->SetFaction(faction);
		}
		else if (GRoom.GetRedPlayerSize() >= GRoom.GetBluePlayerSize()) {
			GRoom.BlueEnter(playerRef, gameSession);
			faction = Faction::BLUE;
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

	cout << "HANDLE_C_PICK_FACTION�� ������" << endl;

	PKT_C_PICK_FACTION* pkt = reinterpret_cast<PKT_C_PICK_FACTION*>(buffer);

	if (pkt->Validate() == false)
		return;
	
	bool success = false;

	//���� ���߿� ==4�� �������ֱ� 4���� �� �־�߸� �÷��� �ǰԲ�
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

	cout << "Handle_C_PICK_CHAMPION�� ������" << endl;

	PKT_C_PICK_CHAMPION* pkt = reinterpret_cast<PKT_C_PICK_CHAMPION*>(buffer);

	if (pkt->Validate() == false)
		return;

	gameSession->GetPlayer()->SetChampionType(pkt->champion);

	bool success = false;

	//���� ���߿� ==4�� �������ֱ� 4���� �� �־�߸� �÷��� �ǰԲ�
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

	//cout << "Handle_C_MOVE�� ������" << endl;

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
	cout << "C_OBJECT_ANIM�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "������ �ƴմϴ�." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_OBJECT_ANIM* pkt = reinterpret_cast<PKT_C_OBJECT_ANIM*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "C_OBJECT_ANIM Validate ����" << endl;
		return;
	}

	uint64 sendId = pkt->sendId;
	AnimInfoPacket animInfo = pkt->animInfo;

	cout << "�ش� ������Ʈ ID : " << sendId << endl;

	PKT_C_OBJECT_ANIM::AnimNameList animNames = pkt->GetAnimNameList();

	wstring resultAnimName = L"";
	for (auto& animName : animNames) {
		resultAnimName.push_back(animName.animName);
	}

	PKT_S_OBJECT_ANIM_WRITE pktWriter(sendId, animInfo);

	PKT_S_OBJECT_ANIM_WRITE::AnimNameList animName = pktWriter.ReserveAnimNameList(resultAnimName.size());
	for (int i = 0; i < resultAnimName.size(); i++) {
		animName[i] = { resultAnimName[i] };
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_OBJECT_MOVE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	//cout << "C_OBJECT_MOVE�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "������ �ƴմϴ�." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_OBJECT_MOVE* pkt = reinterpret_cast<PKT_C_OBJECT_MOVE*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_OBJECT_MOVE Validate ����" << endl;
		return;
	}

	uint64 objectId = pkt->objectId;
	ObjectMove objectMove = pkt->objectMove;

	InhibitorRef inhibitorRef = GRoom.InhibitorFind(objectId);
	if (inhibitorRef) {
		GRoom.InhibitorStatusCheck(inhibitorRef, objectMove);
	}

	//cout << "������Ʈ ID : " << objectId << endl;

	PKT_S_OBJECT_MOVE_WRITE pktWriter(objectId, objectMove);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_SKILL_PROJECTILE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_PROJECTILE�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "������ �ƴմϴ�." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_SKILL_PROJECTILE* pkt = reinterpret_cast<PKT_C_SKILL_PROJECTILE*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_PROJECTILE Validate ����" << endl;
		return;
	}
	//������� ��Ŷ �޴� �κ� �ؿ��� ������ ��Ŷ �ۼ�


	Projectile* projectile = new Projectile;

	SkillInfo skillInfo = pkt->skillInfo;
	int projectileNum = pkt->skillInfo.projectileCount;

	PKT_S_SKILL_PROJECTILE_WRITE pktWriter(projectile->GetObjectId(), skillInfo);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	for (int i = 1; i < projectileNum; i++) {
		delete projectile;
		projectile = new Projectile;
	}
	delete projectile;
}

void ClientPacketHandler::Handle_C_SKILL_HIT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "C_SKILL_HIT�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession->GetPlayer()->GetHost() == false) {
		cout << "������ �ƴմϴ�." << endl;
		return;
	}

	BufferReader br(buffer, len);

	PKT_C_SKILL_HIT* pkt = reinterpret_cast<PKT_C_SKILL_HIT*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "C_SKILL_HIT Validate ����" << endl;
		return;
	}

	PKT_S_SKILL_HIT_WRITE pktWriter(pkt->objecId, pkt->skillInfo);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_DESPAWN_OBJECT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_DESPAWN_OBJECT�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "������ �ƴմϴ�." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_DESPAWN_OBJECT* pkt = reinterpret_cast<PKT_C_DESPAWN_OBJECT*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "Handle_C_DESPAWN_OBJECT Validate ����" << endl;
		return;
	}

	PKT_S_DESPAWN_OBJECT_WRITE pktWriter(pkt->objId, pkt->time);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	GRoom.RemoveObject(pkt->objId);
}

void ClientPacketHandler::Handle_C_KDA_CS(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_KDA_CS�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "������ �ƴմϴ�." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_KDA_CS* pkt = reinterpret_cast<PKT_C_KDA_CS*>(buffer);

	if (pkt->Validate() == false)
	{
		cout << "Handle_C_KDA_CS Validate ����" << endl;
		return;
	}

	PKT_S_KDA_CS_WRITE pktWriter(pkt->kdacsInfo);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_SOUND(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_SOUND�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	//if (gameSession->GetPlayer()->GetHost() == false) {
	//	cout << "������ �ƴմϴ�." << endl;
	//	return;
	//}

	BufferReader br(buffer, len);

	PKT_C_SOUND* pkt = reinterpret_cast<PKT_C_SOUND*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "Handle_C_SOUND Validate ����" << endl;
		return;
	}

	SoundInfoPacket soundInfo = pkt->soundInfo;

	PKT_C_SOUND::SoundNameList soundNames = pkt->GetSoundNameList();

	wstring resultSoundName = L"";
	for (auto& soundName : soundNames) {
		resultSoundName.push_back(soundName.soundName);
	}

	Sound* sound = new Sound;

	PKT_S_SOUND_WRITE pktWriter(sound->GetObjectId(), soundInfo);

	PKT_S_SOUND_WRITE::SoundNameList soundName = pktWriter.ReserveAnimNameList(resultSoundName.size());
	for (int i = 0; i < resultSoundName.size(); i++) {
		soundName[i] = { resultSoundName[i] };
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	delete sound;
}

void ClientPacketHandler::Handle_C_OBJECT_MTRL(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_OBJECT_MTRL�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	PKT_C_OBJECT_MTRL* pkt = reinterpret_cast<PKT_C_OBJECT_MTRL*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "Handle_C_OBJECT_MTRL Validate ����" << endl;
		return;
	}

	MtrlInfoPacket mtrlInfo = pkt->mtrlInfo;

	PKT_C_OBJECT_MTRL::TexNameList texNames = pkt->GetTexNameList();
	PKT_C_OBJECT_MTRL::MtrlNameList mtrlNames = pkt->GetMtrlNameList();

	wstring resultTexName = L"";
	for (auto& texName : texNames) {
		resultTexName.push_back(texName.texName);
	}

	wstring resultMtrlName = L"";
	for (auto& mtrlName : mtrlNames) {
		resultMtrlName.push_back(mtrlName.mtrlName);
	}

	PKT_S_OBJECT_MTRL_WRITE pktWriter(gameSession->GetPlayer()->GetObjectId(), mtrlInfo);
	cout << "Owner�� ���̵� : " << gameSession->GetPlayer()->GetObjectId() << endl;

	PKT_S_OBJECT_MTRL_WRITE::TexNameList texName = pktWriter.ReserveTexNameList(resultTexName.size());
	for (int i = 0; i < resultTexName.size(); i++) {
		texName[i] = { resultTexName[i] };
	}

	PKT_S_OBJECT_MTRL_WRITE::MtrlNameList mtrlName = pktWriter.ReserveMtrlNameList(resultMtrlName.size());
	for (int i = 0; i < resultMtrlName.size(); i++) {
		mtrlName[i] = { resultMtrlName[i] };
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_CHAT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_CHAT�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	PKT_C_CHAT* pkt = reinterpret_cast<PKT_C_CHAT*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "Handle_C_CHAT Validate ����" << endl;
		return;
	}

	PKT_C_CHAT::ChatLog chatLogs = pkt->GetChatLog();

	wstring resultChatLog = L"";
	for (auto& chatLog : chatLogs) {
		resultChatLog.push_back(chatLog.chatLog);
	}

	
	PKT_S_CHAT_WRITE pktWriter(pkt->ownerId);
	cout << "Owner�� ���̵� : " << pkt->ownerId << endl;

	PKT_S_CHAT_WRITE::ChatLog chatLog = pktWriter.ReserveChatLog(resultChatLog.size());
	for (int i = 0; i < resultChatLog.size(); i++) {
		chatLog[i] = { resultChatLog[i] };
	}


	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_EFFECT(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	cout << "Handle_C_EFFECT�� ����" << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	PKT_C_EFFECT* pkt = reinterpret_cast<PKT_C_EFFECT*>(buffer);


	if (pkt->Validate() == false)
	{
		cout << "Handle_C_EFFECT Validate ����" << endl;
		return;
	}

	PKT_C_EFFECT::PrefabName prefabNames = pkt->GetPrefabName();

	wstring resultPrefabName = L"";
	for (auto& prefabName : prefabNames) {
		resultPrefabName.push_back(prefabName.prefabName);
	}

	PKT_S_EFFECT_WRITE pktWriter(pkt->Lifespan, pkt->Pos, pkt->Dir);
	
	PKT_S_EFFECT_WRITE::PrefabName prefabName = pktWriter.ReservePrefabName(resultPrefabName.size());
	for (int i = 0; i < resultPrefabName.size(); i++) {
		prefabName[i] = { resultPrefabName[i] };
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}
