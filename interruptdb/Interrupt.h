#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"
// Please check out the examples at https://github.com/Flofian/BGXInterrupt 
// Credits are appreciated, but you can remove the last line in the InitializeCancelMenu function if you want to remove them

namespace InterruptDB {
    struct interruptableSpell {
        float minRemainingTime;
        float maxRemainingTime;
        float expectedRemainingTime;
        int dangerLevel;
        interruptableSpell() {
            minRemainingTime = 0;
            maxRemainingTime = 0;
            expectedRemainingTime = 0;
            dangerLevel = -1;
        }
    };
    struct internalSpellData {
        float minTime;
        float maxTime;
        float expectedTime;
        float castTime;
        internalSpellData() {
            minTime = 0;
            maxTime = 0;
            expectedTime = 0;
            castTime = 0;
        }
        internalSpellData(float min, float max, float e, float c) {
            minTime = min;
            maxTime = max;
            expectedTime = e;
            castTime = c;
        }
    };


    // Call this to get the "prettier" version of the champions name, since the internal names are slightly different
	std::string getDisplayName(game_object_script target);

    /* 
    Call this with the tab which is meant to contain the Spell Importance sliders
    Small mode reduces the Menu size, instead of a full tab for each champ, it just does the slider/checkbox for the spells
    Bool mode replaces the sliders with checkboxes, useful for champs with only 1 CC ability, pass in the min importance value
        Example: if you pass in 2, Fiddlesticks W is by default off, but Fiddlesticks R is by default on
    The debug option is only used to find problems with my code, do not use
    */
	void InitializeCancelMenu(TreeTab* tab, bool smallMode=false, int boolMode = -1, bool debugUseAllies=false);


    /*
    This function is similar to my old DB / Omori DB
    If the target has an active spell thats in the Menu, this will return their Importance Value, else it will return 0
    If boolMode is set, importance is 1 if they are enabled, else 0
    Use this in your on_update method to check if they have a certain Importance, then cast your spell on them
    Warning: This contains Spells during which the Target can move, so make sure to call prediction as well
    */
	int getCastingImportance(game_object_script target);

    /*
    This function returns a struct containing the Casting Importance, and the following cast time values:
    If boolMode is set, importance is 1 if they are enabled, else 0
    maxRemainingTime:       How long the target could theoretically continue charging the spell
    minRemainingTime:       How long the target has to continue casting before he can move again
    expectedRemainingTime:  How long the target will probably continue casting
    If you want to "guarantee" a hit, make sure the minRemainingTime is greater than your spells delay/time to hit  (Check out the Nami Example)
    Examples:
        Pyke Q:
            max = 3         He can hold it for up to 3 seconds
            min = 0         He can release it whenever he wants
            expected = 1    He needs 1 second to charge to max out the q range
        Caitlyn R:
            max = min = expected = 1.375, since she charges for 1s after 0.375s cast time
        Velkoz R:
            max = 2.8       He can cast for 2.8s
            min = 1         He needs to wait 1s until he can recast it, he cant move until then
            expected = 2.8  Most of the time, spells like this get cast for the full duration
    */
    interruptableSpell getInterruptable(game_object_script target);
}