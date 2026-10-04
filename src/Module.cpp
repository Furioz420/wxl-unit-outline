// wxl-unit-outline: v1.1 extension entry point.
// Copyright (C) 2026 WarcraftXL. GPLv3.

#include "ExtensionApi.hpp"
#include "wxl/EventScript.hpp"

namespace wxl_unit_outline
{
    const WXL_Api* g_api = nullptr;
    const WXL_M2DrawApi* g_m2Draw = nullptr;
}

const WXL_PluginInfo* __cdecl WXL_Query(void)
{
    static const WXL_PluginInfo info{
        sizeof(WXL_PluginInfo), WXL_API_VERSION, "wxl-unit-outline", 1, WXL_CLIENT_BUILD,
    };
    return &info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    if (!api || api->apiVersion != WXL_API_VERSION) return 0;
    wxl_unit_outline::g_api = api;
    wxl::ext::EventScript::Bind(api);

    if (!wxl_unit_outline::ConfigBool("WXL_UNIT_OUTLINE", true))
    {
        api->Log(WXL_LOG_INFO, "wxl-unit-outline", "extension disabled by configuration");
        return 1;
    }
    if (!wxl_unit_outline::M2Draw())
    {
        api->Log(WXL_LOG_ERROR, "wxl-unit-outline",
                 "required wxl.m2draw v1 is unavailable; ensure wxl-modern-m2 is installed");
        return 0;
    }
    return wxl_unit_outline::InstallUnitOutline() ? 1 : 0;
}
