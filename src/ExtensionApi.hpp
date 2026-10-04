// wxl-unit-outline access to the Hub ABI and shared render services.
// Copyright (C) 2026 WarcraftXL. GPLv3.

#pragma once

#include "common/ExtensionConfig.hpp"
#include "wxl/M2DrawApi.h"
#include "wxl/PluginApi.h"

namespace wxl_unit_outline
{
    extern const WXL_Api* g_api;
    extern const WXL_M2DrawApi* g_m2Draw;

    inline const WXL_M2DrawApi* M2Draw()
    {
        if (!g_m2Draw)
            g_m2Draw = static_cast<const WXL_M2DrawApi*>(
                g_api->GetInterface("wxl.m2draw", WXL_M2DRAW_API_VERSION));
        return g_m2Draw;
    }

    inline bool ConfigBool(const char* name, bool fallback)
    {
        char value[16] = {};
        return wxl::ext::config::Raw(name, value, sizeof value,
                                     "Extensions\\wxl-unit-outline\\wxl-unit-outline.cfg")
            ? wxl::ext::config::Truthy(value, fallback)
            : fallback;
    }

    bool InstallUnitOutline();
}

#define WLOG_TRACE(...) ::wxl_unit_outline::g_api->Log(WXL_LOG_TRACE, "wxl-unit-outline", __VA_ARGS__)
#define WLOG_DEBUG(...) ::wxl_unit_outline::g_api->Log(WXL_LOG_DEBUG, "wxl-unit-outline", __VA_ARGS__)
#define WLOG_INFO(...)  ::wxl_unit_outline::g_api->Log(WXL_LOG_INFO,  "wxl-unit-outline", __VA_ARGS__)
#define WLOG_WARN(...)  ::wxl_unit_outline::g_api->Log(WXL_LOG_WARN,  "wxl-unit-outline", __VA_ARGS__)
#define WLOG_ERROR(...) ::wxl_unit_outline::g_api->Log(WXL_LOG_ERROR, "wxl-unit-outline", __VA_ARGS__)
