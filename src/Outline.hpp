// Reaction-coloured silhouette outline on mouseover and target units.
// Copyright (C) 2026 WarcraftXL. GPLv3.

#pragma once

#include "game/Gx.hpp"
#include "wxl/EventScript.hpp"

namespace wxl_unit_outline
{
    class Outline final : public wxl::ext::EventScript
    {
    public:
        Outline();

    private:
        void OnWorldRenderEnd(const wxl::events::WorldRenderEndArgs& args);
        void OnM2Batch(const wxl::events::M2BatchDrawArgs& args);
        void OnDeviceLost(const wxl::events::DeviceResetArgs& args);
        void OnWorldLeave(const wxl::events::WorldLeaveArgs& args);

        bool EnsureResources(wxl::game::gx::Device9 device);
        void ReleaseResources();
        void RebuildTargets();
        void StampSilhouette(wxl::game::gx::Device9 device,
                             const wxl::events::M2BatchDrawArgs& args, int index);
        void EdgePass(wxl::game::gx::Device9 device);

        bool ShouldStampBatch(wxl::game::gx::Device9 device) const;
        int FindTarget(void* model) const;
        void AddTarget(unsigned long long guid, void* player);
        static void ColorForReaction(int reaction, float* rgba);

        static constexpr int kMaxTargets = 2;
        struct Target
        {
            void* model = nullptr;
            float color[4]{};
            bool isPlayer = false;
        };

        Target targets_[kMaxTargets]{};
        int count_ = 0;
        wxl::game::gx::RenderTarget mask_{};
        void* colorShader_ = nullptr;
        void* cutoutShader_ = nullptr;
        void* edgeShader_ = nullptr;
        void* resourceDevice_ = nullptr;
        bool maskCleared_ = false;
        float thickness_ = 1.5f;
    };
}
