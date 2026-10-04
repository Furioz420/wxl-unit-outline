// Shader programs for the unit silhouette mask and edge composite.
// Copyright (C) 2026 WarcraftXL. GPLv3.

#pragma once

namespace wxl_unit_outline::hlsl
{
    // Opaque model batches write only the target's reaction colour into the mask. Diffuse alpha is
    // deliberately ignored: on many models it carries material/detail data instead of transparency.
    inline constexpr char kColor[] =
        "float4 c0 : register(c0);\n"
        "float4 main(float2 uv : TEXCOORD0) : COLOR0 {\n"
        "  return c0;\n"
        "}\n";

    // Alpha-tested geometry keeps its real cutout (hair cards, wings, leaves) in the silhouette.
    inline constexpr char kCutoutColor[] =
        "sampler2D s0 : register(s0);\n"
        "float4 c0 : register(c0);\n"
        "float4 main(float2 uv : TEXCOORD0) : COLOR0 {\n"
        "  clip(tex2D(s0, uv).a - 0.5);\n"
        "  return c0;\n"
        "}\n";

    // Eight samples form a thin anti-aliased line outside the silhouette. The mask's RGB carries
    // the reaction colour; its alpha carries coverage.
    inline constexpr char kEdge[] =
        "sampler2D m : register(s0);\n"
        "float4 px : register(c0);\n"
        "float4 main(float2 uv : TEXCOORD0) : COLOR0 {\n"
        "  float2 o = px.xy * px.z;\n"
        "  float4 a = tex2D(m,uv+float2(o.x,0)) + tex2D(m,uv+float2(-o.x,0))\n"
        "           + tex2D(m,uv+float2(0,o.y)) + tex2D(m,uv+float2(0,-o.y))\n"
        "           + tex2D(m,uv+float2(o.x,o.y)) + tex2D(m,uv+float2(-o.x,-o.y))\n"
        "           + tex2D(m,uv+float2(o.x,-o.y)) + tex2D(m,uv+float2(-o.x,o.y));\n"
        "  float inside = tex2D(m, uv).a;\n"
        "  float outline = saturate(a.a * 0.125 * 1.6) * (1.0 - inside);\n"
        "  clip(outline - 0.02);\n"
        "  float3 col = a.a > 0.001 ? a.rgb / a.a : float3(1,1,1);\n"
        "  return float4(col, saturate(outline * 1.4));\n"
        "}\n";
}
