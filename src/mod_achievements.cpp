/*
<--------------------------------------------------------------------------->
- Developer(s): Lille Carl, Grim/Render
- Complete: %100
- ScriptName: 'AccountAchievements'
- Comment: Tested and Works.
- Orginial Creator: Lille Carl
- Edited: Render/Grim
- Edited: Blkht01(Qeme) Classic Plus - Added Exclusion
<--------------------------------------------------------------------------->
*/

#include "Config.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include <unordered_set>
#include <sstream>

class AccountAchievements : public PlayerScript
{
	static const bool limitrace = false;
	static const bool limitlevel = false;
	int minlevel = 80;
	int setlevel = 1;

	std::unordered_set<uint32> excludedAchievements; // Store excluded achievement IDs

public:
	AccountAchievements() : PlayerScript("AccountAchievements") 
	{
		// Load excluded achievements from config
		std::string excludeList = sConfigMgr->GetOption<std::string>("Account.Achievements.Excluded", "");
		std::stringstream ss(excludeList);
		std::string token;

		while (std::getline(ss, token, ',')) 
		{
			try {
				uint32 id = std::stoul(token);
				excludedAchievements.insert(id);
			} catch (...) {
				LOG_ERROR("module", "Invalid achievement ID in Account.Achievements.Excluded: {}", token);
			}
		}
	}

	void OnLogin(Player* pPlayer) override
	{
		if (sConfigMgr->GetOption<bool>("Account.Achievements.Enable", true))
		{
			if (sConfigMgr->GetOption<bool>("Account.Achievements.Announce", true))
			{
				ChatHandler(pPlayer->GetSession()).SendSysMessage("This server is running the |cff4CFF00AccountAchievements |rmodule.");
			}

			std::vector<uint32> Guids;
			QueryResult result1 = CharacterDatabase.Query("SELECT guid, race FROM characters WHERE account = {}", pPlayer->GetSession()->GetAccountId());
			if (!result1)
				return;

			do
			{
				Field* fields = result1->Fetch();
				uint32 race = fields[1].Get<uint8>();

				if ((Player::TeamIdForRace(race) == Player::TeamIdForRace(pPlayer->getRace())) || !limitrace)
					Guids.push_back(fields[0].Get<uint32>());

			} while (result1->NextRow());

			std::vector<uint32> Achievement;

			for (auto& i : Guids)
			{
				QueryResult result2 = CharacterDatabase.Query("SELECT achievement FROM character_achievement WHERE guid = {}", i);
				if (!result2)
					continue;

				do
				{
					uint32 achievementID = result2->Fetch()[0].Get<uint32>();
					// Only add if not in the exclusion list
					if (excludedAchievements.find(achievementID) == excludedAchievements.end()) 
					{
						Achievement.push_back(achievementID);
					}

				} while (result2->NextRow());
			}

			for (auto& i : Achievement)
			{
				auto sAchievement = sAchievementStore.LookupEntry(i);
				if (sAchievement)
				{
					AddAchievements(pPlayer, sAchievement->ID);
				}
			}
		}
	}

	void AddAchievements(Player* player, uint32 AchievementID)
	{
		if (sConfigMgr->GetOption<bool>("Account.Achievements.Enable", true))
		{
			if (limitlevel)
				setlevel = minlevel;

			if (player->GetLevel() >= setlevel)
			{
				// Check exclusion before granting achievement
				if (excludedAchievements.find(AchievementID) == excludedAchievements.end()) 
				{
					player->CompletedAchievement(sAchievementStore.LookupEntry(AchievementID));
				}
			}
		}
	}
};

void AddAccountAchievementsScripts()
{
	new AccountAchievements;
}
