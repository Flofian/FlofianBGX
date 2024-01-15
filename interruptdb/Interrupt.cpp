#include "../plugin_sdk/plugin_sdk.hpp"
#include "Interrupt.h"
namespace InterruptDB {
    std::map<std::string, TreeEntry*> menuMap;
    bool useSmallMenu = false;
    int boolMenuMinImportance = -1;
    auto rbuffhashes = { buff_hash("LucianR"), buff_hash("SamiraR"), buff_hash("RyzeRChannel"), buff_hash("Gate"), buff_hash("warwickrsound") };  // I hate this, also buff times are wrong, maybe need to use onProcessSpellCast
    std::map<std::string, internalSpellData> channelTime = { // I cant get every Spell via spelldata->channeltime
        {"AkshanR", internalSpellData(0.5,2.5,2.5,0)},    // I will not do the calculations to find out how many he needs
        // Belveth E and Briar E are not interruptable
        {"CaitlynR", internalSpellData(1,1,1,0.375)},
        {"FiddleSticksW", internalSpellData(0,2,2,0.25)},
        {"FiddleSticksR", internalSpellData(1.5,1.5,1.5,0)},
        {"GalioW", internalSpellData(0,2,1.25,0)},
        {"GalioR", internalSpellData(1.25,1.25,1.25,0)},
        {"GragasW", internalSpellData(0.75,0.75,0.75,0)},
        // Irelia W not interruptable
        {"JannaR", internalSpellData(0,3,3,0)}, // Could in theory do calculations if/when team is full hp
        {"JhinR", internalSpellData(0,10,4,1)}, // Could try to figure out how many shots he has left?
        {"KarthusR", internalSpellData(3,3,3,0.25)},
        {"KatarinaR", internalSpellData(0,2.5,2.5,0)},  // Could check targets in range
        // KSante W not interruptable
        {"LucianR", internalSpellData(0.75,3,3,0)},    //Could check targets, Also since buff time is wrong, this is not accurate
        {"MalzaharR", internalSpellData(0,2.5,2.5,0)},
        {"MasterYiW", internalSpellData(0,4,4,0)},  // Again, a full hp yi will probably not W for 4 seconds
        {"MissFortuneR", internalSpellData(0,3,3,0)},
        {"NaafiriW", internalSpellData(0.75,0.75,0.75,0)},
        {"NunuW", internalSpellData(0.5,10,5,0)},
        {"NunuR", internalSpellData(0,3,3,0)},
        {"PantheonQ", internalSpellData(0,3.25,0.35,0)},
        {"PantheonR", internalSpellData(2,2,2,0.1)},
        {"PoppyR", internalSpellData(0,4,1,0)},
        {"PykeQ", internalSpellData(0,3,1,0)},
        {"QuinnR", internalSpellData(2,2,2,0)},
        // I dont think having rammus q here makes sense, nunu w should be okay with prediction, but rammus q is impossible
        {"RyzeR", internalSpellData(2,2,2,0)},
        {"SamiraR", internalSpellData(2.25,2.25,2.25,0)},
        {"ShenR", internalSpellData(3,3,3,0)},
        {"SionQ", internalSpellData(0,2,1,0)},
        {"TahmKenchW", internalSpellData(1.35,1.35,1.35,0)},
        {"TaliyahR", internalSpellData(1,1,1,0)},
        {"TwistedFateR", internalSpellData(1.5,1.5,1.5,0)},
        {"VarusQ", internalSpellData(0,4.5,1.25,0)},
        {"VelkozR", internalSpellData(0.8,2.6,2.6,0.2)},
        {"ViQ", internalSpellData(0,4.5,1.25,0)},
        {"ViegoW", internalSpellData(0,3,1,0)},
        {"VladimirE", internalSpellData(0,1.5,1,0)},
        {"WarwickQ", internalSpellData(0,0.5,0,0)}, // Not sure if this makes sense
        {"WarwickR", internalSpellData(1.5,1.5,1.5,0)},
        {"XerathQ", internalSpellData(0,3,1.75,0)},
        {"XerathR", internalSpellData(0,10,2,0)},
        {"ZacE", internalSpellData(0,4.5,0.9,0)},



    };   

	std::string getDisplayName(game_object_script target) {
		switch (target->get_champion())
		{
		case champion_id::AurelionSol: return "Aurelion Sol";
		case champion_id::Belveth: return "Bel'Veth";
		case champion_id::Chogath: return "Cho'Gath";
		case champion_id::DrMundo: return "Dr. Mundo";
		case champion_id::FiddleSticks: return "Fiddlesticks";
		case champion_id::JarvanIV: return "Jarvan IV";
		case champion_id::Kaisa: return "Kai'Sa";
		case champion_id::Khazix: return "Kha'Zix";
		case champion_id::KogMaw: return "Kog'Maw";
		case champion_id::KSante: return "K'Sante";
		case champion_id::Leblanc: return "LeBlanc";
		case champion_id::LeeSin: return "Lee Sin";
		case champion_id::MasterYi: return "Master Yi";
		case champion_id::MissFortune: return "Miss Fortune";
		case champion_id::MonkeyKing: return "Wukong";
		case champion_id::Nunu: return "Nunu and Willump";
		case champion_id::RekSai: return "Rek'Sai";
		case champion_id::Renata: return "Renata Glasc";
		case champion_id::TahmKench: return "Tahm Kench";
		case champion_id::Velkoz: return "Vel'Koz";
		default: return target->get_model();
		}
	}
    void InitiateSlot(TreeTab* tab, game_object_script entity, spellslot slot, std::string name, std::string spellName, int defaultValue)
    {
        std::string key;
        void* texture;
        switch (slot)
        {
        case spellslot::q:
            key = "Q";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::w:
            key = "W";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::e:
            key = "E";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::r:
            key = "R";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        default:
            key = "?";
            texture = entity->get_passive_icon_texture();
            break;
        }
        auto model = entity->get_model();
        auto displayName = getDisplayName(entity);
        auto id = std::to_string((int)entity->get_champion());

        if (useSmallMenu) {
            if (boolMenuMinImportance < 0) {
                menuMap[model + key] = tab->add_slider(model + key, displayName + " | " + key, defaultValue, 0, 3, true);
            }
            else {
                menuMap[model + key] = tab->add_checkbox(model + key, displayName + " | " + key, defaultValue>=boolMenuMinImportance, true);
                menuMap[model + key]->set_texture(texture);
            }
        }
        else {
            auto t = tab->add_tab(model, "[" + displayName + "]");
            t->set_texture(entity->get_square_icon_portrait());

            if (boolMenuMinImportance < 0) {
                menuMap[model + key] = t->add_slider(model + key, "[" + key + "] - " + spellName, defaultValue, 0, 3, true);
            }
            else {
                menuMap[model + key] = t->add_checkbox(model + key, "[" + key + "] - " + spellName, defaultValue >= boolMenuMinImportance, true);
                menuMap[model + key]->set_texture(texture);
            }
        }
    }

    void InitializeCancelMenu(TreeTab* tab,bool smallMode, int boolMode, bool debugUseAllies)
    {
        useSmallMenu = smallMode;
        boolMenuMinImportance = boolMode;
        std::vector<game_object_script> enemyList;
        if (debugUseAllies) {
            enemyList = entitylist->get_ally_heroes();
        }else{
            enemyList = entitylist->get_enemy_heroes();
        }
        for (auto& e : enemyList)
        {
            if (e == nullptr)
                return;

            auto id = e->get_champion();



            switch (id) {
            case champion_id::Akshan:
                InitiateSlot(tab, e, spellslot::r, "Akshan", "Comeuppance", 2);
                break;


            case champion_id::Caitlyn:
                InitiateSlot(tab, e, spellslot::r, "Caitlyn", "Ace in the Hole", 3);
                break;

            case champion_id::FiddleSticks:
                InitiateSlot(tab, e, spellslot::w, "Fiddlesticks", "Bountiful Harvest", 1);
                InitiateSlot(tab, e, spellslot::r, "Fiddlesticks", "Crowstorm", 3);
                break;

            case champion_id::Galio:
                InitiateSlot(tab, e, spellslot::w, "Galio", "Shield of Durand", 0);
                InitiateSlot(tab, e, spellslot::r, "Galio", "Hero's Entrance", 3);
                break;

            case champion_id::Gragas:
                InitiateSlot(tab, e, spellslot::w, "Gragas", "Drunken Rage", 0);
                break;

            case champion_id::Janna:
                InitiateSlot(tab, e, spellslot::r, "Janna", "Monsoon", 3);
                break;

            case champion_id::Jhin:
                InitiateSlot(tab, e, spellslot::r, "Jhin", "Curtain's Call", 3);
                break;

            case champion_id::Karthus:
                InitiateSlot(tab, e, spellslot::r, "Karthus", "Requiem", 3);
                break;

            case champion_id::Katarina:
                InitiateSlot(tab, e, spellslot::r, "Katarina", "Death Lotus", 3);
                break;

            case champion_id::Kayn:
                InitiateSlot(tab, e, spellslot::r, "Kayn", "Umbral Trespass", 0);
                break;

            case champion_id::Lucian:
                InitiateSlot(tab, e, spellslot::r, "Lucian", "The Culling", 3);
                break;

            case champion_id::Malzahar:
                InitiateSlot(tab, e, spellslot::r, "Malzahar", "Death Grasp", 3);
                break;

            case champion_id::MasterYi:
                InitiateSlot(tab, e, spellslot::w, "Master Yi", "Meditate", 1);
                break;

            case champion_id::MissFortune:
                InitiateSlot(tab, e, spellslot::r, "Miss Fortune", "Bullet Time", 3);
                break;

            case champion_id::Naafiri:
                InitiateSlot(tab, e, spellslot::w, "Naafiri", "Hounds' Pursuit", 0);
                break;

            case champion_id::Nunu:
                InitiateSlot(tab, e, spellslot::w, "Nunu and Willump", "Biggest Snowball Ever!", 2);
                InitiateSlot(tab, e, spellslot::r, "Nunu and Willump", "Absolute Zero", 3);
                break;

            case champion_id::Pantheon:
                InitiateSlot(tab, e, spellslot::q, "Pantheon", "Comet Spear", 0);

                InitiateSlot(tab, e, spellslot::r, "Pantheon", "Grand Starfall", 3);
                break;

            case champion_id::Poppy:
                InitiateSlot(tab, e, spellslot::r, "Poppy", "Keeper's Verdict", 3);
                break;

            case champion_id::Pyke:
                InitiateSlot(tab, e, spellslot::q, "Pyke", "Bone Skewer", 0);
                break;

            case champion_id::Quinn:
                InitiateSlot(tab, e, spellslot::r, "Quinn", "Behind Enemy Lines", 1);
                break;

            case champion_id::Rammus:
                InitiateSlot(tab, e, spellslot::q, "Rammus", "Powerball", 0);
                break;

            case champion_id::Ryze:
                InitiateSlot(tab, e, spellslot::r, "Ryze", "Realm Warp", 3);
                break;

            case champion_id::Samira:
                InitiateSlot(tab, e, spellslot::r, "Samira", "Inferno Trigger", 3);
                break;

            case champion_id::Shen:
                InitiateSlot(tab, e, spellslot::r, "Shen", "Stand United", 3);
                break;

            case champion_id::Sion:
                InitiateSlot(tab, e, spellslot::q, "Sion", "Decimating Smash", 2);
                break;

            case champion_id::TahmKench:
                InitiateSlot(tab, e, spellslot::w, "Tahm Kench", "Abyssal Dive", 2);
                break;

            case champion_id::Taliyah:
                InitiateSlot(tab, e, spellslot::r, "Taliyah", "Weaver's Wall", 0);
                break;

            case champion_id::TwistedFate:
                InitiateSlot(tab, e, spellslot::r, "Twisted Fate", "Gate", 3);
                break;

            case champion_id::Varus:
                // Maybe add WQ as separate? Cause its more worth to cancel
                InitiateSlot(tab, e, spellslot::q, "Varus", "Piercing Arrow", 0);
                break;

            case champion_id::Velkoz:
                InitiateSlot(tab, e, spellslot::r, "Vel'Koz", "Life Form Disintegration Ray", 3);
                break;

            case champion_id::Vi:
                InitiateSlot(tab, e, spellslot::q, "Vi", "Vault Breaker", 0);
                break;

            case champion_id::Viego:
                InitiateSlot(tab, e, spellslot::w, "Viego", "Spectral Maw", 0);
                break;

            case champion_id::Vladimir:
                InitiateSlot(tab, e, spellslot::e, "Vladimir", "Tides of Blood", 0);
                break;

            case champion_id::Warwick:
                InitiateSlot(tab, e, spellslot::q, "Warwick", "Jaws of the Beast", 0);
                InitiateSlot(tab, e, spellslot::r, "Warwick", "Infinite Duress", 3);
                break;

            case champion_id::Xerath:
                InitiateSlot(tab, e, spellslot::q, "Xerath", "Arcanopulse", 0);
                InitiateSlot(tab, e, spellslot::r, "Xerath", "Rite of the Arcane", 3);
                break;

            case champion_id::Yuumi:
                InitiateSlot(tab, e, spellslot::q, "Yuumi", "Prowling Projectile", 0);
                InitiateSlot(tab, e, spellslot::r, "Yuumi", "Final Chapter", 0);
                break;

            case champion_id::Zac:
                InitiateSlot(tab, e, spellslot::e, "Zac", "Elastic Slingshot", 2);
                break;

            default:
                break;
            }
        }
        if(boolMode < 0) 
            tab->add_separator("sep1", "Change Spell Importance here");
        tab->add_separator("sep2", "Made by Flofian");
        // Credits are appreciated, but i dont care if you remove this line
        
    }
    int getCastingImportance(game_object_script target)
    {
        auto active = target->get_active_spell();
        if (!active)
            return 0;
        auto slot = active->get_spellslot();

        std::string key = "?";

        switch (slot)
        {
        case spellslot::q:
            key = "Q";
            break;
        case spellslot::w:
            key = "W";
            break;
        case spellslot::e:
            key = "E";
            break;
        case spellslot::r:
            // I exclude Warwick here, dont want to "interrupt" his jump, only the channel
            // So i ignore his R Active Spell, but check for the effect below
            if (target->get_champion() != champion_id::Warwick)
                key = "R";
            break;
        default:
            break;
        }
        if (target->has_buff(rbuffhashes)) key = "R";

        auto entity = menuMap.find(target->get_model() + key);
        if (entity == menuMap.end())
            return 0;
        if (boolMenuMinImportance == -1) {
            return entity->second->get_int();
        }
        else {
            return entity->second->get_bool();
        }
    }

    interruptableSpell getInterruptable(game_object_script target) {
        interruptableSpell output = interruptableSpell();
        if (!target || !target->is_valid() || !target->is_ai_hero()) return output; // to fix rare crash that i cant reproduce
        auto active = target->get_active_spell();
        if (!active && !target->has_buff(rbuffhashes))
            return output;
        std::string key = "?";
        float realStartTime = -1;
        // This split is needed since SamiraR, LucianR etc are not in activeSpell, they are only a buff
        if (active) {   
            auto slot = active->get_spellslot();

            switch (slot)
            {
            case spellslot::q:
                key = "Q";
                break;
            case spellslot::w:
                key = "W";
                break;
            case spellslot::e:
                key = "E";
                break;
            case spellslot::r:
            // I exclude Warwick here, dont want to "interrupt" his jump, only the channel
            // So i ignore him if he uses r here
                if (target->get_champion() != champion_id::Warwick)
                    key = "R";
                break;
            default:
                //console->print("Spellslot %i", (int) slot);
                break;
            }
            realStartTime = active->get_time();
        }
        if (target->has_buff(rbuffhashes)) {
            // yeah i could just check buff endtime, but i will keep it like this
            key = "R";
            for (const auto buffhash : rbuffhashes) {
                auto b = target->get_buff(buffhash);
                if (b) realStartTime = b->get_start();
            }
        }


        auto entity = menuMap.find(target->get_model() + key);
        if (entity == menuMap.end()) return output;
        if (boolMenuMinImportance == -1) {
            output.dangerLevel = entity->second->get_int();
        }
        else {
            output.dangerLevel = entity->second->get_bool();
        }
        auto spellListIter = channelTime.find(target->get_model() + key);
        if (spellListIter == channelTime.end()) return output;
        
        

        
        float timeSinceStart = gametime->get_time() - realStartTime - spellListIter->second.castTime;
        float expTimeLeft = spellListIter->second.expectedTime - timeSinceStart;
        float minTimeLeft = spellListIter->second.minTime - timeSinceStart;
        float maxTimeLeft = spellListIter->second.maxTime - timeSinceStart;

        const auto& tempBuff2 = target->get_buff(buff_hash("xerathrshots"));    // works okay
        if (tempBuff2)
            expTimeLeft = (tempBuff2->get_count() - 1)*0.5;

        if (target->get_champion() == champion_id::Zac)
            expTimeLeft = 0.8 + 0.1 * target->get_spell(spellslot::e)->level() - timeSinceStart;

        output.expectedRemainingTime = fmax(expTimeLeft, 0);
        output.minRemainingTime = fmax(minTimeLeft, 0);
        output.maxRemainingTime = fmax(maxTimeLeft, 0);


        return output;
    }
}