#include "aoe_loot.h"
#include "ScriptMgr.h"
#include "World.h"
#include "LootMgr.h"
#include "ServerScript.h"
#include "WorldSession.h"
#include "WorldPacket.h" 
#include "Player.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "ChatCommandArgs.h"
#include "WorldObjectScript.h"
#include "Creature.h"
#include "Config.h"
#include "Log.h"
#include "Map.h"
#include <fmt/format.h>
#include "Corpse.h"
#include "Group.h"
#include "ObjectMgr.h"

using namespace Acore::ChatCommands;
using namespace WorldPackets;

std::map<uint64, bool> playerAoeLootEnabled;
std::map<uint64, bool> playerAoeLootDebug;

// Server packet handler. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

// >>>>> This is the entry point. This packet triggers the AOE loot system. <<<<< //

bool AoeLootServer::CanPacketReceive(WorldSession* session, WorldPacket& packet)
{
    if (packet.GetOpcode() == CMSG_LOOT)
    {
        Player* player = session->GetPlayer();
        if (player)
        {
            uint64 guid = player->GetGUID().GetRawValue();

            // >>>>> Aoe looting enabled check. <<<<< //
            
            if (playerAoeLootEnabled.find(guid) != playerAoeLootEnabled.end() && 
                playerAoeLootEnabled[guid])
            {
                
                // >>>>> Aoe loot start. <<<<< //
                
                ChatHandler handler(player->GetSession());
                handler.ParseCommands(".aoeloot startaoeloot");
            }
        }
    }
    return true;
}

// Server packet handler end. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< //


// Command table implementation. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

ChatCommandTable AoeLootCommandScript::GetCommands() const
{
    static ChatCommandTable aoeLootSubCommandTable =
    {
        { "startaoeloot",   HandleStartAoeLootCommand,          SEC_PLAYER, Console::No },
        { "toggle",         HandleAoeLootToggleCommand,         SEC_PLAYER, Console::No },
        { "on",             HandleAoeLootOnCommand,             SEC_PLAYER, Console::No },
        { "off",            HandleAoeLootOffCommand,            SEC_PLAYER, Console::No },
        { "debug on",       HandleAoeLootDebugOnCommand,        SEC_PLAYER, Console::No },
        { "debug",          HandleAoeLootDebugToggleCommand,    SEC_PLAYER, Console::No },
        { "debug off",      HandleAoeLootDebugOffCommand,       SEC_PLAYER, Console::No }
    };

    static ChatCommandTable aoeLootCommandTable =
    {
        { "aoeloot",        aoeLootSubCommandTable }
    };
    
    return aoeLootCommandTable;
}

// Command table implementation end. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< //


// Command handlers implementation. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

bool AoeLootCommandScript::HandleAoeLootOnCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    if (!sConfigMgr->GetOption<bool>("AOELoot.Enable", true))
    {
        handler->PSendSysMessage("AOE Loot is disabled by server configuration.");
        return true;
    }
    
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    playerAoeLootEnabled[player->GetGUID().GetRawValue()] = true;
    handler->PSendSysMessage("AOE looting has been enabled for your character. Type: '.aoeloot off' to turn AoE Looting Off.");
    return true;
}

bool AoeLootCommandScript::HandleAoeLootOffCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    playerAoeLootEnabled[player->GetGUID().GetRawValue()] = false;
    handler->PSendSysMessage("AOE Loot disabled for your character.");
    return true;
}

bool AoeLootCommandScript::HandleAoeLootToggleCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;
    playerAoeLootEnabled[player->GetGUID().GetRawValue()] = !playerAoeLootEnabled[player->GetGUID().GetRawValue()];
    handler->PSendSysMessage("AOE Loot toggled for your character.");
    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugOnCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;
    playerAoeLootDebug[player->GetGUID().GetRawValue()] = true;
    handler->PSendSysMessage("AOE Loot debug mode enabled.");
    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugOffCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;
    playerAoeLootDebug[player->GetGUID().GetRawValue()] = false;
    handler->PSendSysMessage("AOE Loot debug mode disabled.");
    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugToggleCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;
    playerAoeLootDebug[player->GetGUID().GetRawValue()] = !playerAoeLootDebug[player->GetGUID().GetRawValue()];
    handler->PSendSysMessage("AOE Loot debug mode toggled.");
    return true;
}

// Command handlers implementation End. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< //


// Helper functions. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

// >>>>> Debugger message function. <<<<< //

void AoeLootCommandScript::DebugMessage(Player* player, const std::string& message)
{
    if (sConfigMgr->GetOption<bool>("AOELoot.Debug", false) || playerAoeLootDebug[player->GetGUID().GetRawValue()])
    {
        ChatHandler(player->GetSession()).PSendSysMessage("AOE Loot: {}", message);
    }
}

std::vector<Player*> AoeLootCommandScript::GetGroupMembers(Player* player)
{
    std::vector<Player*> members;
    Group* group = player->GetGroup();
    if (!group)
        return members;
        
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsInWorld() && !member->isDead())
            members.push_back(member);
    }
    return members;
}

bool AoeLootCommandScript::IsValidLootTarget(Player* player, Creature* creature)
{
    if (!creature || !player)
        return false;

    if (creature->IsAlive())
        return false;
        
    if (creature->loot.empty() || creature->loot.isLooted())
        return false;

    uint64 playerGuid = player->GetGUID().GetRawValue();
    if (playerAoeLootEnabled.find(playerGuid) == playerAoeLootEnabled.end() || 
        !playerAoeLootEnabled[playerGuid])
        return false;

    return true;
}

void AoeLootCommandScript::ProcessQuestItems(Player* player, ObjectGuid lguid, Loot* loot)
{
    if (!player || !loot)
        return;
        
    // >>>>> Process different quest item types. <<<<< //

    const QuestItemMap& questItems = loot->GetPlayerQuestItems();
    for (uint8 i = 0; i < questItems.size(); ++i)
    {
        uint8 lootSlot = loot->items.size() + i;
        ProcessLootSlot(player, lguid, lootSlot);
        DebugMessage(player, fmt::format("Looted quest item in slot {}", lootSlot));
    }
    
    const QuestItemMap& ffaItems = loot->GetPlayerFFAItems();
    for (uint8 i = 0; i < ffaItems.size(); ++i)
    {
        ProcessLootSlot(player, lguid, i);
        DebugMessage(player, fmt::format("Looted FFA item in slot {}", i));
    }
}

std::pair<Loot*, bool> AoeLootCommandScript::GetLootObject(Player* player, ObjectGuid lguid)
{
    if (lguid.IsGameObject())
    {
        
        // >>>>> This protects against looting Game Objects and Structures <<<<< //

        DebugMessage(player, "Skipping GameObject - not supported for AOE loot");
        return {nullptr, false};
    }
    else if (lguid.IsItem())
    {
        Item* pItem = player->GetItemByGuid(lguid);
        if (!pItem)
        {
            DebugMessage(player, fmt::format("Failed to find item {}", lguid.ToString()));
            return {nullptr, false};
        }
        return {&pItem->loot, true};
    }
    else if (lguid.IsCorpse())
    {
        Corpse* bones = ObjectAccessor::GetCorpse(*player, lguid);
        if (!bones)
        {
            DebugMessage(player, fmt::format("Failed to find corpse {}", lguid.ToString()));
            return {nullptr, false};
        }
        return {&bones->loot, true};
    }
    else // >>>>> Creature <<<<< //
    {
        Creature* creature = player->GetMap()->GetCreature(lguid);
        if (!creature)
        {
            DebugMessage(player, fmt::format("Failed to find creature {}", lguid.ToString()));
            return {nullptr, false};
        }
        
        return {&creature->loot, true};
    }
}

bool AoeLootCommandScript::HandleStartAoeLootCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    if (!sConfigMgr->GetOption<bool>("AOELoot.Enable", true))
        return true;

    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    // >>>>> Do not remove the hardcoded value. It is here for crash & data protection. <<<<< //
    
    float range = sConfigMgr->GetOption<float>("AOELoot.Range", 55.0f);
    
    auto validCorpses = GetValidCorpses(player, range);

    uint32 CorpseThreshold = sConfigMgr->GetOption<uint32>("AOELoot.CorpseThreshold", 2);
    if (validCorpses.size() < CorpseThreshold)
    {
        DebugMessage(player, "Not enough corpses for AOE loot. Defaulting to normal looting.");
        return true;
    }
    
    for (auto* creature : validCorpses)
    {
        ProcessCreatureLoot(player, creature);
    }
    
    return true;
}

std::vector<Creature*> AoeLootCommandScript::GetValidCorpses(Player* player, float range)
{
    std::list<Creature*> nearbyCorpses;
    player->GetDeadCreatureListInGrid(nearbyCorpses, range);
    
    DebugMessage(player, fmt::format("Found {} nearby corpses within range {}", nearbyCorpses.size(), range));
    
    std::vector<Creature*> validCorpses;
    for (auto* creature : nearbyCorpses)
    {
        if (IsValidLootTarget(player, creature))
            validCorpses.push_back(creature);
    }

    DebugMessage(player, fmt::format("Found {} valid corpses", validCorpses.size()));
    return validCorpses;
}

void AoeLootCommandScript::ProcessCreatureLoot(Player* player, Creature* creature)
{
    ObjectGuid lguid = creature->GetGUID();
    Loot* loot = &creature->loot;
    
    if (!loot)
        return;

    ObjectGuid originalLootGuid = player->GetLootGUID();    
    player->SetLootGUID(lguid);
    
    ProcessQuestItems(player, lguid, loot);
    
    for (uint8 lootSlot = 0; lootSlot < loot->items.size(); ++lootSlot)
    {
        ProcessLootSlot(player, lguid, lootSlot);
    }
    
    if (loot->gold > 0)
    {
        ProcessLootMoney(player, creature);
    }
    
    if (loot->isLooted())
    {
        ProcessLootRelease(lguid, player, loot);
    }
    player->SetLootGUID(originalLootGuid);
}

bool AoeLootCommandScript::ProcessLootSlot(Player* player, ObjectGuid lguid, uint8 lootSlot)
{
    if (!player)
        return false;

    auto [loot, isValid] = GetLootObject(player, lguid);

    if 
    (
        !isValid                        || 
        !loot                           || 
        lootSlot >= loot->items.size()
    )
        return false;

    InventoryResult msg = EQUIP_ERR_OK;
    LootItem* lootItem = player->StoreLootItem(lootSlot, loot, msg);
    if (!lootItem)
    {
        DebugMessage(player, fmt::format("Failed to loot slot {} of {}: inventory error {}", lootSlot, lguid.ToString(), static_cast<uint32>(msg)));
        return false;
    }
    DebugMessage(player, fmt::format("Looted item from slot {} of {}", lootSlot, lguid.ToString()));
    return true;
}

bool AoeLootCommandScript::ProcessLootMoney(Player* player, Creature* creature)
{
    if (!player || !creature || creature->loot.gold == 0)
        return false;
        
    uint32 goldAmount = creature->loot.gold;
    Group* group = player->GetGroup();
    
    if (group && sConfigMgr->GetOption<bool>("AOELoot.Group", true))
    {
        std::vector<Player*> nearbyMembers;

        // >>>>> Do not remove the hardcoded value. It is here for crash & data protection. <<<<< //
        
        float range = sConfigMgr->GetOption<float>("AOELoot.Range", 55.0f);

        // >>>>> Do not remove the hardcoded value. It is here for crash & data protection. <<<<< //
        
        float moneyShareMultiplier = sConfigMgr->GetOption<float>("AOELoot.MoneyShareDistanceMultiplier", 2.0f);
        
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (member && member->IsWithinDistInMap(player, range * moneyShareMultiplier))
                nearbyMembers.push_back(member);
        }
        
        if (!nearbyMembers.empty())
        {
            uint32 goldPerPlayer = goldAmount / nearbyMembers.size();
            for (Player* member : nearbyMembers)
            {
                member->ModifyMoney(goldPerPlayer);
                ChatHandler(member->GetSession()).PSendSysMessage("AOE Loot: +{} gold", goldPerPlayer);
            }
        }
    }
    else
    {
        player->ModifyMoney(goldAmount);
        ChatHandler(player->GetSession()).PSendSysMessage("AOE Loot: +{} gold", goldAmount);
    }
    
    creature->loot.gold = 0;
    return true;
}

void AoeLootCommandScript::ProcessLootRelease(ObjectGuid lguid, Player* player, Loot* loot)
{
    if (!player || !loot)
        return;
        
    player->SetLootGUID(ObjectGuid::Empty);
    player->SendLootRelease(lguid);
    
    if (lguid.IsCreature())
    {
        Creature* creature = player->GetMap()->GetCreature(lguid);
        if (creature && loot->isLooted())
        {
            creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
            creature->AllLootRemovedFromCorpse();
        }
    }
    
    DebugMessage(player, fmt::format("Released loot for {}", lguid.ToString()));
}

void AoeLootPlayer::OnPlayerLogin(Player* player)
{
    if (sConfigMgr->GetOption<bool>("AOELoot.Enable", true) && 
        sConfigMgr->GetOption<bool>("AOELoot.Message", true))
    {
        ChatHandler(player->GetSession()).PSendSysMessage("AOE looting has been enabled for your character. Commands: .aoeloot debug | .aoeloot off | .aoeloot on");
    }
}

// Helper functions end. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

// >>>>> Add the scripts for registration as a module. <<<<< //

void AddSC_AoeLoot()
{
    new AoeLootPlayer();
    new AoeLootServer();
    new AoeLootCommandScript();
}
