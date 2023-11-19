#include "../plugin_sdk/plugin_sdk.hpp"
#include "sona.h"
#include "nautilus.h"

PLUGIN_NAME( "Flofian" );
PLUGIN_TYPE(plugin_type::champion);
SUPPORTED_CHAMPIONS(champion_id::Sona) //, champion_id::Nautilus);

PLUGIN_API bool on_sdk_load( plugin_sdk_core* plugin_sdk_good )
{
    DECLARE_GLOBALS( plugin_sdk_good );

    switch (myhero->get_champion()) {
    case champion_id::Sona:
        sona::load();
        break;
    case champion_id::Nautilus:
        //nautilus::load();
        break;

    default:
        console->print("Champion %s is not supported!", myhero->get_model_cstr());
        return false;
    }

    return true;
}

PLUGIN_API void on_sdk_unload( )
{
    switch (myhero->get_champion())
    {
    case champion_id::Sona:
        sona::unload();
        break;
    case champion_id::Nautilus:
        //nautilus::unload();
        break;
    default:
        break;
    }
}