// You are free to use this as you will, you can modify it freely!
// It'd be appreciated if you left the credits in the menu functions though :)
// Updates can be found at https://github.com/0x8L4H4J/BGXAddons
// Feel free to join my Discord at https://discord.gg/czsJYCYSn8 as I'll be pinging the 'BGX Dev' role everytime the databases are updated!
#include "SpellDB.h"

namespace Database
{
    namespace db
    {
        std::map<std::string, TreeEntry*> CanCancel;
        std::map<std::string, TreeEntry*> CanOnAllyBuff;
    }
    std::string getDisplayName(game_object_script target)
    {
        std::map<int, std::string> championNames = {
            {(int)champion_id::AurelionSol, "Aurelion Sol"},
            {(int)champion_id::Chogath, "Cho'Gath"},
            {(int)champion_id::DrMundo, "Dr. Mundo"},
            {(int)champion_id::FiddleSticks, "Fiddlesticks"},
            {(int)champion_id::JarvanIV, "Jarvan IV"},
            {(int)champion_id::Kaisa, "Kai'Sa"},
            {(int)champion_id::Khazix, "Kha'Zix"},
            {(int)champion_id::KogMaw, "Kog'Maw"},
            {(int)champion_id::KSante, "K'Sante"},
            {(int)champion_id::Leblanc, "LeBlanc"},
            {(int)champion_id::LeeSin, "Lee Sin"},
            {(int)champion_id::MasterYi, "Master Yi"},
            {(int)champion_id::MissFortune, "Miss Fortune"},
            {(int)champion_id::MonkeyKing, "Wukong"},
            {(int)champion_id::Nunu, "Nunu and Willump"},
            {(int)champion_id::RekSai, "Rek'Sai"},
            {(int)champion_id::Renata, "Renata Glasc"},
            {(int)champion_id::TahmKench, "Tahm Kench"},
            {(int)champion_id::Velkoz, "Vel'Koz"},
            {5000, "Target Dummy"},
        };


        auto id = target->get_champion();
        if (championNames.count((int)id))
            return championNames[(int)id];
        else
            return target->get_model();
    }

    void InitiateSlot(TreeTab* tab, game_object_script entity, spellslot slot, std::string name, std::string spellName, bool defaultValue, int mode)
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
        case spellslot::item_1:
            key = "Q1";
            texture = entity->get_spell(slot)->get_icon_texture_by_index(1);
            break;
        // Ignore because i only do interrupts, not buffs (and to make the errors go away)
        /*case (spellslot)50:
            key = "Rune";
            break;
        case (spellslot)51:
            key = "Item";
            break;*/
        case spellslot::invalid:
            key = "P";
            texture = entity->get_passive_icon_texture();
            break;
        default:
            key = "?";
            texture = entity->get_passive_icon_texture();
            break;
        }

        if (entity != nullptr)
        {
            auto model = entity->get_model();
            auto displayName = getDisplayName(entity);
            auto id = std::to_string((int)entity->get_champion());

            auto t = tab->add_tab(model, "[" + displayName + "]");
            t->set_texture(entity->get_square_icon_portrait());

            switch (mode)
            {
                case 1: // Consider Case 1 as "Cancel"
                    db::CanCancel[model + key] = t->add_checkbox(model + key, "[" + key + "] - " + spellName, defaultValue);
                    if (texture != nullptr)
                    {
                        db::CanCancel[model + key]->set_texture(texture);
                    }
                    //db::CanCancel[model + key]->set_tooltip("[Importance] of " + std::to_string(localImportance(entity->get_champion(), slot)));
                    break;
                case 2: // Consider Case 2 as "Ally Buff"
                    db::CanOnAllyBuff[id + key] = t->add_checkbox(id + key, "[" + key + "] - " + spellName, defaultValue);
                    if (texture != nullptr)
                    {
                        db::CanOnAllyBuff[id + key]->set_texture(texture);
                    }
                    break;
            }
        }
        else // Consider this as for Items/Runes
        {
            auto t = tab->add_tab(spellName, "[" + name + "]");
            switch (mode)
            {
                case 1: // Consider Case 1 as "Cancel"
                    // db::CanCancel[model + key] = t->add_checkbox(model + key, "[" + key + "] - " + spellName, defaultValue);
                    break;
                case 2: // Consider Case 2 as "Ally Buff"
                    db::CanOnAllyBuff[spellName] = t->add_checkbox(spellName, "[" + key + "] - " + spellName, defaultValue);
                    break;
            }

        }
    }

    void InitializeCancelMenu(TreeTab* tab)
    {
        if (!tab) // To prevent crashes in case of bad usage!
        {
            console->print_error("[BlahajDB] - Error Code: DB-IM1");
            return;
        }

        std::vector<game_object_script> enemyList = entitylist->get_enemy_heroes();

        for (auto& e : enemyList)
        {
            if (e == nullptr)
                return;

            auto id = e->get_champion();



            switch (id) {
                case champion_id::Akshan:
                    InitiateSlot(tab, e, spellslot::r, "Akshan", "Comeuppance", true, 1);
                    break;

                case champion_id::Caitlyn:
                    InitiateSlot(tab, e, spellslot::r, "Caitlyn", "Ace in the Hole", true, 1);
                    break;

                case champion_id::FiddleSticks:
                    InitiateSlot(tab, e, spellslot::w, "Fiddlesticks", "Bountiful Harvest", false, 1);
                    InitiateSlot(tab, e, spellslot::r, "Fiddlesticks", "Crowstorm", true, 1);
                    break;

                case champion_id::Galio:
                    InitiateSlot(tab, e, spellslot::w, "Galio", "Shield of Durand", false, 1);
                    InitiateSlot(tab, e, spellslot::r, "Galio", "Hero's Entrance", true, 1);
                    break;

                case champion_id::Gragas:
                    InitiateSlot(tab, e, spellslot::w, "Gragas", "Drunken Rage", false, 1);
                    break;

                case champion_id::Irelia:
                    InitiateSlot(tab, e, spellslot::w, "Irelia", "Defiant Dance", false, 1);
                    break;

                case champion_id::Janna:
                    InitiateSlot(tab, e, spellslot::r, "Janna", "Monsoon", true, 1);
                    break;

                case champion_id::Jhin:
                    InitiateSlot(tab, e, spellslot::r, "Jhin", "Curtain's Call", true, 1);
                    break;

                case champion_id::Karthus:
                    InitiateSlot(tab, e, spellslot::r, "Karthus", "Requiem", true, 1);
                    break;

                case champion_id::Katarina:
                    InitiateSlot(tab, e, spellslot::r, "Katarina", "Death Lotus", true, 1);
                    break;

                case champion_id::Kayn:
                    InitiateSlot(tab, e, spellslot::r, "Kayn", "Umbral Trespass", false, 1);
                    break;

                case champion_id::KSante:
                    InitiateSlot(tab, e, spellslot::w, "K'Sante", "Path Maker", false, 1);
                    break;

                case champion_id::Lucian:
                    InitiateSlot(tab, e, spellslot::r, "Lucian", "The Culling", true, 1);
                    break;

                case champion_id::Malzahar:
                    InitiateSlot(tab, e, spellslot::r, "Malzahar", "Death Grasp", true, 1);
                    break;

                case champion_id::MasterYi:
                    InitiateSlot(tab, e, spellslot::w, "Master Yi", "Meditate", false, 1);
                    break;

                case champion_id::MissFortune:
                    InitiateSlot(tab, e, spellslot::r, "Miss Fortune", "Bullet Time", true, 1);
                    break;

                case champion_id::Nunu:
                    InitiateSlot(tab, e, spellslot::w, "Nunu and Willump", "Biggest Snowball Ever!", false, 1);

                    InitiateSlot(tab, e, spellslot::r, "Nunu and Willump", "Absolute Zero", true, 1);
                    break;

                case champion_id::Pantheon:
                    InitiateSlot(tab, e, spellslot::q, "Pantheon", "Comet Spear", false, 1);

                    InitiateSlot(tab, e, spellslot::r, "Pantheon", "Grand Starfall", true, 1);
                    break;

                case champion_id::Poppy:
                    InitiateSlot(tab, e, spellslot::r, "Poppy", "Keeper's Verdict", true, 1);
                    break;

                case champion_id::Pyke:
                    InitiateSlot(tab, e, spellslot::q, "Pyke", "Bone Skewer", false, 1);
                    break;

                case champion_id::Quinn:
                    InitiateSlot(tab, e, spellslot::r, "Quinn", "Behind Enemy Lines", false, 1);
                    break;

                case champion_id::Rammus:
                    InitiateSlot(tab, e, spellslot::q, "Rammus", "Powerball", false, 1);
                    break;

                case champion_id::Ryze:
                    InitiateSlot(tab, e, spellslot::r, "Ryze", "Realm Warp", true, 1);
                    break;

                case champion_id::Samira:
                    InitiateSlot(tab, e, spellslot::r, "Samira", "Inferno Trigger", true, 1);
                    break;

                case champion_id::Shen:
                    InitiateSlot(tab, e, spellslot::r, "Shen", "Stand United", true, 1);
                    break;

                case champion_id::Sion:
                    InitiateSlot(tab, e, spellslot::q, "Sion", "Decimating Smash", false, 1);
                    //InitiateSlot(tab, e, spellslot::r, "Sion", "Unstoppable Onslaught", false, 1);
                    break;

                case champion_id::TahmKench:
                    InitiateSlot(tab, e, spellslot::w, "Tahm Kench", "Abyssal Dive", false, 1);
                    break;

                case champion_id::Taliyah:
                    InitiateSlot(tab, e, spellslot::r, "Taliyah", "Weaver's Wall", false, 1);
                    break;

                case champion_id::TwistedFate:
                    InitiateSlot(tab, e, spellslot::r, "Twisted Fate", "Gate", true, 1);
                    break;

                case champion_id::Varus:
                    InitiateSlot(tab, e, spellslot::q, "Varus", "Piercing Arrow", false, 1);
                    break;

                case champion_id::Velkoz:
                    InitiateSlot(tab, e, spellslot::r, "Vel'Koz", "Life Form Disintegration Ray", true, 1);
                    break;

                case champion_id::Vi:
                    InitiateSlot(tab, e, spellslot::q, "Vi", "Vault Breaker", false, 1);
                    break;

                case champion_id::Viego:
                    InitiateSlot(tab, e, spellslot::w, "Viego", "Spectral Maw", false, 1);
                    break;

                case champion_id::Vladimir:
                    InitiateSlot(tab, e, spellslot::e, "Vladimir", "Tides of Blood", false, 1);
                    break;

                case champion_id::Warwick:
                    InitiateSlot(tab, e, spellslot::q, "Warwick", "Jaws of the Beast", false, 1);
                    InitiateSlot(tab, e, spellslot::r, "Warwick", "Infinite Duress", true, 1);
                    break;

                case champion_id::Xerath:
                    InitiateSlot(tab, e, spellslot::q, "Xerath", "Arcanopulse", false, 1);
                    InitiateSlot(tab, e, spellslot::r, "Xerath", "Rite of the Arcane", true, 1);
                    break;

                case champion_id::Yuumi:
                    InitiateSlot(tab, e, spellslot::q, "Yuumi", "Prowling Projectile", false, 1);
                    InitiateSlot(tab, e, spellslot::r, "Yuumi", "Final Chapter", false, 1);
                    break;

                case champion_id::Zac:
                    InitiateSlot(tab, e, spellslot::r, "Zac", "Elastic Slingshot", false, 1);
                    break;

                default:
                    break;
            }
        }

        tab->add_separator("databaseInfo", "- Database made by Omori! <3 -");
        tab->add_separator("databaseVersion", "Version: " + DBVersion);
    }

    bool canCancel(game_object_script target)
    {
        auto active = target->get_active_spell();
        if (!active)
            return false;
        auto slot = active->get_spellslot();

        std::string key;

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
                key = "R";
                break;
            default:
                key = "?";
                break;
        }

        auto entity = Database::db::CanCancel.find(target->get_model() + key);
        if (entity == Database::db::CanCancel.end())
            return false;

        return entity->second->get_bool();
    }
}