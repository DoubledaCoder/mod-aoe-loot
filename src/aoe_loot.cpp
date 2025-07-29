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

std::map<uint64, bool> AoeLootCommandScript::playerAoeLootEnabled;
std::map<uint64, bool> AoeLootCommandScript::playerAoeLootDebug;


// Server packet handler. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

// >>>>> This is the entry point. This packet triggers the AOE loot system. <<<<< //

bool AoeLootManager::CanPacketReceive(WorldSession* session, WorldPacket& packet)
{
    if (packet.GetOpcode() == CMSG_LOOT)
    {
        Player* player = session->GetPlayer();
        if (player)
        {
            uint64 guid = player->GetGUID().GetRawValue();

            // >>>>> Do not remove the hardcoded value. It is here for crash & data protection. <<<<< //

            if (!AoeLootCommandScript::hasPlayerAoeLootEnabled(guid))
            {
                AoeLootCommandScript::SetPlayerAoeLootEnabled(guid, sConfigMgr->GetOption<bool>("AOELoot.Enable", true));
            }
            if (!AoeLootCommandScript::hasPlayerAoeLootDebug(guid)) 
            {
                AoeLootCommandScript::SetPlayerAoeLootDebug(guid, sConfigMgr->GetOption<bool>("AOELoot.Debug", false));
            }

            // >>>>> Aoe looting enabled check. <<<<< //

            if (AoeLootCommandScript::hasPlayerAoeLootEnabled(guid) && AoeLootCommandScript::GetPlayerAoeLootEnabled(guid))
            {

                // >>>>> Aoe loot start. <<<<< //

                AoeLootCommandScript::DebugMessage(player, "AOE Looting started.");
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


// Getters and setters for player AOE loot settings. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

bool AoeLootCommandScript::GetPlayerAoeLootEnabled(uint64 guid)
{
    auto it = playerAoeLootEnabled.find(guid);
    if (it != playerAoeLootEnabled.end())
        return it->second;
    return false;  
}

bool AoeLootCommandScript::GetPlayerAoeLootDebug(uint64 guid)
{
    auto it = playerAoeLootDebug.find(guid);
    if (it != playerAoeLootDebug.end())
        return it->second;
    return false;
}

void AoeLootCommandScript::SetPlayerAoeLootEnabled(uint64 guid, bool mode)
{
    if (playerAoeLootEnabled.find(guid) == playerAoeLootEnabled.end())
    {
        playerAoeLootEnabled[guid] = mode;
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Set AOE loot enabled for GUID {}: {}", guid, mode));
    }
    else
    {
        playerAoeLootEnabled[guid] = mode;
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Updated AOE loot enabled for GUID {}: {}", guid, mode));
    }
    AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Set AOE loot enabled for GUID {}: {}", guid, mode));
}

void AoeLootCommandScript::SetPlayerAoeLootDebug(uint64 guid, bool mode)
{
    if (playerAoeLootDebug.find(guid) == playerAoeLootDebug.end())
    {
        playerAoeLootDebug[guid] = mode;
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Set AOE loot debug for GUID {}: {}", guid, mode));
    }
    else
    {
        playerAoeLootDebug[guid] = mode;
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Updated AOE loot debug for GUID {}: {}", guid, mode));
    }
    AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Set AOE loot debug for GUID {}: {}", guid, mode));
}

void AoeLootCommandScript::RemovePlayerLootEnabled(uint64 guid)
{
    if (playerAoeLootEnabled.find(guid) != playerAoeLootEnabled.end())
    {
        playerAoeLootEnabled.erase(guid);
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Removed AOE loot enabled for GUID {}", guid));
    }
    else
    {
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("No AOE loot enabled found for GUID {}", guid));
    }
}

void AoeLootCommandScript::RemovePlayerLootDebug(uint64 guid)
{
    if (playerAoeLootDebug.find(guid) != playerAoeLootDebug.end())
    {
        playerAoeLootDebug.erase(guid);
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("Removed AOE loot debug for GUID {}", guid));
    }
    else
    {
        AoeLootCommandScript::DebugMessage(nullptr, fmt::format("No AOE loot debug found for GUID {}", guid));
    }
}

bool AoeLootCommandScript::hasPlayerAoeLootEnabled(uint64 guid)
{
    return playerAoeLootEnabled.count(guid) > 0;
}

bool AoeLootCommandScript::hasPlayerAoeLootDebug(uint64 guid)
{
    return playerAoeLootDebug.count(guid) > 0;
}

// Getters and setters end. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //


// Command handlers implementation. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

bool AoeLootCommandScript::HandleAoeLootOnCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    uint64 playerGuid = player->GetGUID().GetRawValue();

    if (AoeLootCommandScript::hasPlayerAoeLootEnabled(playerGuid) && 
        AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        handler->PSendSysMessage("AOE Loot is already enabled for your character.");
        return true;
    }
    if (AoeLootCommandScript::hasPlayerAoeLootEnabled(playerGuid) && 
        !AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, true);
        handler->PSendSysMessage("AOE Loot enabled for your character. Type: '.aoeloot off' to turn AoE Looting off.");
        return true;
    }
   
    return true;
}

bool AoeLootCommandScript::HandleAoeLootOffCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    uint64 playerGuid = player->GetGUID().GetRawValue();
    if (AoeLootCommandScript::hasPlayerAoeLootEnabled(playerGuid) && 
        AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, false);
        handler->PSendSysMessage("AOE Loot disabled for your character. Type: '.aoeloot on' to turn AoE Looting on.");
        DebugMessage(player, "AOE Loot disabled for your character.");
    }
    else
    {
        handler->PSendSysMessage("AOE Loot is already disabled for your character.");
    }
    return true;
}

bool AoeLootCommandScript::HandleAoeLootToggleCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    uint64 playerGuid = player->GetGUID().GetRawValue();
    if (!AoeLootCommandScript::hasPlayerAoeLootEnabled(playerGuid))
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, true);
        handler->PSendSysMessage("AOE Loot enabled for your character. Type: '.aoeloot off' to turn AoE Looting off.");
        DebugMessage(player, "AOE Loot enabled for your character.");
    }
    else if (!AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, true);
        handler->PSendSysMessage("AOE Loot is now enabled for your character. Type: '.aoeloot off' to turn AoE Looting off.");
        DebugMessage(player, "AOE Loot is now enabled for your character.");
    }
    else if (AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, false);
        handler->PSendSysMessage("AOE Loot is now disabled for your character. Type: '.aoeloot on' to turn AoE Looting on.");
        DebugMessage(player, "AOE Loot is now disabled for your character.");
    }
    else
    {
        AoeLootCommandScript::SetPlayerAoeLootEnabled(playerGuid, false);
        handler->PSendSysMessage("AOE Loot disabled for your character. Type: '.aoeloot on' to turn AoE Looting on.");
        DebugMessage(player, "AOE Loot disabled for your character.");
    }   

    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugOnCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    if (AoeLootCommandScript::hasPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == 0)
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), true);
        handler->PSendSysMessage("AOE Loot debug mode enabled for your character.");
        DebugMessage(player, "AOE Loot debug mode enabled for your character.");
    }   
    else if (!AoeLootCommandScript::GetPlayerAoeLootDebug(player->GetGUID().GetRawValue()))
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), true);
        handler->PSendSysMessage("AOE Loot debug mode is now enabled for your character.");
        DebugMessage(player, "AOE Loot debug mode is now enabled for your character.");
    }
    else
    {
        handler->PSendSysMessage("AOE Loot debug mode is already enabled for your character.");
    }
    DebugMessage(player, "AOE Loot debug mode enabled for your character.");

    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugOffCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    if (AoeLootCommandScript::hasPlayerAoeLootDebug(player->GetGUID().GetRawValue()) > 0 && 
        AoeLootCommandScript::GetPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == true)
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), false);
        handler->PSendSysMessage("AOE Loot debug mode disabled for your character.");
        DebugMessage(player, "AOE Loot debug mode disabled for your character.");
    }
    else if (AoeLootCommandScript::hasPlayerAoeLootDebug(player->GetGUID().GetRawValue()) > 0 && 
             AoeLootCommandScript::GetPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == false)
    {
        handler->PSendSysMessage("AOE Loot debug mode is already disabled for your character.");
        DebugMessage(player, "AOE Loot debug mode is already disabled for your character.");
    }
    else
    {
        handler->PSendSysMessage("AOE Loot debug mode is not enabled for your character.");
        DebugMessage(player, "AOE Loot debug mode is not enabled for your character.");
    }

    return true;
}

bool AoeLootCommandScript::HandleAoeLootDebugToggleCommand(ChatHandler* handler, Optional<std::string> /*args*/)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return true;

    if (AoeLootCommandScript::hasPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == 0)
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), true);
        handler->PSendSysMessage("AOE Loot debug mode enabled for your character.");
    }
    else if (AoeLootCommandScript::GetPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == false)
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), true);
        handler->PSendSysMessage("AOE Loot debug mode is now enabled for your character."); 
    }
    else if (AoeLootCommandScript::GetPlayerAoeLootDebug(player->GetGUID().GetRawValue()) == true)
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), false);
        handler->PSendSysMessage("AOE Loot debug mode is now disabled for your character.");
    }
    else
    {
        AoeLootCommandScript::SetPlayerAoeLootDebug(player->GetGUID().GetRawValue(), false);
        handler->PSendSysMessage("AOE Loot debug mode disabled for your character.");
    }

    return true;
}

// Command handlers implementation End. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< //


// Helper functions. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //

// >>>>> Debugger message function. <<<<< //

void AoeLootCommandScript::DebugMessage(Player* player, const std::string& message)
{
    if (!player)
        return;

    uint64 guid = player->GetGUID().GetRawValue();

    if (AoeLootCommandScript::GetPlayerAoeLootDebug(guid) && AoeLootCommandScript::hasPlayerAoeLootDebug(guid))
    {
        // >>>>> This will send debug messages to the player. <<<<< //

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
    if (!AoeLootCommandScript::hasPlayerAoeLootEnabled(playerGuid))
    {
        DebugMessage(player, "Player AOE loot setting not found.");
        return false;
    }

    if (!AoeLootCommandScript::GetPlayerAoeLootEnabled(playerGuid))
    {
        DebugMessage(player, "Player AOE loot is disabled.");
        return false;
    }

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

     // >>>>> Basic validation checks <<<<< //
    if (!isValid || !loot)
    {
        DebugMessage(player, fmt::format("Failed to loot slot {} of {}: invalid loot object", lootSlot, lguid.ToString()));
        return false;
    }

    // >>>>> Check if loot has items <<<<< //
    if (loot->items.empty() || lootSlot >= loot->items.size())
    {
        DebugMessage(player, fmt::format("Failed to loot slot {} of {}: invalid slot or no items", lootSlot, lguid.ToString()));
        return false;
    }

    // >>>>> Check if the specific loot item exists <<<<< //
    LootItem& lootItem = loot->items[lootSlot];

    if (lootItem.is_blocked || lootItem.is_looted)
    {
        DebugMessage(player, fmt::format("Failed to loot slot {} of {}: item is blocked", lootSlot, lguid.ToString()));
        return false;
    }

    InventoryResult msg = EQUIP_ERR_OK;
    LootItem* storedItem = player->StoreLootItem(lootSlot, loot, msg);
    if (!storedItem)
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
                ChatHandler(member->GetSession()).PSendSysMessage("AOE Loot: +{} copper", goldPerPlayer);
            }
        }
    }
    else
    {
        player->ModifyMoney(goldAmount);
        ChatHandler(player->GetSession()).PSendSysMessage("AOE Loot: +{} copper", goldAmount);
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

void AoeLootPlayer::OnPlayerLogout(Player* player)
    {
        uint64 guid = player->GetGUID().GetRawValue();
        // Clean up player data
        if (AoeLootCommandScript::hasPlayerAoeLootEnabled(guid))
            AoeLootCommandScript::RemovePlayerLootEnabled(guid);
        if (AoeLootCommandScript::hasPlayerAoeLootDebug(guid))
            AoeLootCommandScript::RemovePlayerLootDebug(guid);
    }

// Helper functions end. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> //



// >>>>> Add the scripts for registration as a module. <<<<< //

void AddSC_AoeLoot()
{
    new AoeLootPlayer();
    new AoeLootManager();
    new AoeLootCommandScript();
}
