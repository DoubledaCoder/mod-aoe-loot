#ifndef MODULE_AOELOOT_H
#define MODULE_AOELOOT_H

#include "ScriptMgr.h"
#include "Config.h"
#include "ServerScript.h"
#include "Chat.h"
#include "Player.h"
#include "Item.h"
#include "ScriptedGossip.h"
#include "ChatCommand.h"       
#include "ChatCommandArgs.h" 
#include "AccountMgr.h"
#include <vector> 
#include <list>
#include <map>
#include <ObjectGuid.h>

using namespace Acore::ChatCommands;

class AoeLootServer : public ServerScript
{
public:
    AoeLootServer() : ServerScript("AoeLootServer") {}

    bool CanPacketReceive(WorldSession* session, WorldPacket& packet) override;
};

class AoeLootPlayer : public PlayerScript
{
public:
    AoeLootPlayer() : PlayerScript("AoeLootPlayer") {}

    void OnPlayerLogin(Player* player) override;
};

class AoeLootCommandScript : public CommandScript
{
public:
    AoeLootCommandScript() : CommandScript("AoeLootCommandScript") {}
    ChatCommandTable GetCommands() const override;

    // Command handlers
    static bool HandleAoeLootOnCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleAoeLootOffCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleStartAoeLootCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleAoeLootToggleCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleAoeLootDebugOnCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleAoeLootDebugOffCommand(ChatHandler* handler, Optional<std::string> args);
    static bool HandleAoeLootDebugToggleCommand(ChatHandler* handler, Optional<std::string> args);
    
    // Core loot processing functions
    static bool ProcessLootSlot(Player* player, ObjectGuid lguid, uint8 lootSlot);
    static bool ProcessLootMoney(Player* player, Creature* creature);
    static void ProcessLootRelease(ObjectGuid lguid, Player* player, Loot* loot);

    // Helper functions
    static void DebugMessage(Player* player, const std::string& message);
    static std::vector<Player*> GetGroupMembers(Player* player);
    static void ProcessQuestItems(Player* player, ObjectGuid lguid, Loot* loot);
    static std::pair<Loot*, bool> GetLootObject(Player* player, ObjectGuid lguid);
    static std::vector<Creature*> GetValidCorpses(Player* player, float range);
    static void ProcessCreatureLoot(Player* player, Creature* creature);
    static bool IsValidLootTarget(Player* player, Creature* creature);
};

void AddSC_AoeLoot();

#endif //MODULE_AOELOOT_H
