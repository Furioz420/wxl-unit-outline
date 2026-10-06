// Reaction-coloured silhouette outline on mouseover and target units.
// Copyright (C) 2026 WarcraftXL. GPLv3.

#include "Outline.hpp"

#include "ExtensionApi.hpp"
#include "Hlsl.hpp"
#include "game/Unit.hpp"
#include "game/World.hpp"

#include <d3d9.h>

namespace wxl_unit_outline
{
    namespace ev = wxl::events;
    namespace gx = wxl::game::gx;
    namespace unit = wxl::game::unit;
    namespace world = wxl::game::world;

    constexpr uint32_t kFormatA8R8G8B8 = 21;

    class ScopedDeviceState final
    {
    public:
        explicit ScopedDeviceState(gx::Device9 device)
        {
            auto* raw = static_cast<IDirect3DDevice9*>(device.raw());
            if (raw && SUCCEEDED(raw->CreateStateBlock(D3DSBT_ALL, &state_)) && state_)
                state_->Capture();
        }

        ~ScopedDeviceState()
        {
            if (!state_) return;
            state_->Apply();
            state_->Release();
        }

        ScopedDeviceState(const ScopedDeviceState&) = delete;
        ScopedDeviceState& operator=(const ScopedDeviceState&) = delete;

    private:
        IDirect3DStateBlock9* state_ = nullptr;
    };

    Outline::Outline()
    {
        on<&Outline::OnWorldRenderEnd>(ev::Event::OnWorldRenderEnd);
        on<&Outline::OnM2Batch>(ev::Event::OnM2BatchDraw);
        on<&Outline::OnDeviceLost>(ev::Event::OnDeviceLost);
        on<&Outline::OnWorldLeave>(ev::Event::OnWorldLeave);
    }

    void Outline::ColorForReaction(int reaction, float* color)
    {
        if (reaction < 2)
        {
            color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f;
        }
        else if (reaction < 4)
        {
            color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.0f;
        }
        else
        {
            color[0] = 0.0f; color[1] = 1.0f; color[2] = 0.0f;
        }
        color[3] = 1.0f;
    }

    void Outline::ReleaseResources()
    {
        gx::Release(mask_);
        gx::Release(colorShader_);
        gx::Release(cutoutShader_);
        gx::Release(edgeShader_);
        colorShader_ = nullptr;
        cutoutShader_ = nullptr;
        edgeShader_ = nullptr;
        resourceDevice_ = nullptr;
        maskCleared_ = false;
    }

    bool Outline::EnsureResources(gx::Device9 device)
    {
        if (!device) return false;
        if (resourceDevice_ && resourceDevice_ != device.raw())
            ReleaseResources();
        resourceDevice_ = device.raw();

        if (!colorShader_)
            colorShader_ = gx::CompilePixelShader(device, hlsl::kColor, "ps_2_0");
        if (!cutoutShader_)
            cutoutShader_ = gx::CompilePixelShader(device, hlsl::kCutoutColor, "ps_2_0");
        if (!edgeShader_)
            edgeShader_ = gx::CompilePixelShader(device, hlsl::kEdge, "ps_2_0");
        if (!mask_.surface)
            gx::EnsureBackbufferTarget(device, mask_, kFormatA8R8G8B8);

        return colorShader_ && cutoutShader_ && edgeShader_ && mask_.surface;
    }

    int Outline::FindTarget(void* model) const
    {
        for (int hop = 0; model && hop < 8; ++hop, model = unit::ModelParent(model))
            for (int index = 0; index < count_; ++index)
                if (targets_[index].model == model) return index;
        return -1;
    }

    void Outline::AddTarget(unsigned long long guid, void* player)
    {
        if (!guid || count_ >= kMaxTargets) return;
        const bool isPlayer = (guid >> 32) == 0;

        void* object = world::ResolveObject(
            guid, isPlayer ? world::kTypeMaskPlayer : world::kTypeMaskUnit);
        if (!object) return;

        void* model = unit::Model(object);
        if (!model) return;
        for (int index = 0; index < count_; ++index)
            if (targets_[index].model == model) return;

        const int reaction = player ? unit::Reaction(object, player) : 5;
        targets_[count_].model = model;
        targets_[count_].isPlayer = isPlayer;
        ColorForReaction(reaction, targets_[count_].color);
        ++count_;
    }

    void Outline::RebuildTargets()
    {
        void* player = world::ResolveObject(world::ActivePlayerGuid(), world::kTypeMaskPlayer);
        count_ = 0;
        AddTarget(world::MouseoverGuid(), player);
        AddTarget(world::TargetGuid(), player);
    }

    bool Outline::ShouldStampBatch(gx::Device9 device) const
    {
        // Attached particles and billboard glows share the parent model context. Excluding blended
        // batches keeps their broad cards out of the unit's silhouette.
        return device && device.GetRenderState(gx::rs::kAlphaBlend) == 0;
    }

    void Outline::StampSilhouette(gx::Device9 device, const ev::M2BatchDrawArgs& args, int index)
    {
        ScopedDeviceState state(device);

        void* oldTarget = nullptr;
        void* oldDepth = nullptr;
        void* oldShader = nullptr;
        unsigned char oldViewport[24]{};
        device.GetRenderTarget(0, &oldTarget);
        device.GetDepthStencil(&oldDepth);
        device.GetPixelShader(&oldShader);
        device.GetViewport(oldViewport);

        const unsigned alphaReference = device.GetRenderState(D3DRS_ALPHAREF);
        const bool alphaCutout = device.GetRenderState(D3DRS_ALPHATESTENABLE) != 0 &&
                                 alphaReference >= 8;

        device.SetRenderTarget(0, mask_.surface);
        if (targets_[index].isPlayer)
        {
            // Preserve the original module's policy: player outlines obey scene depth, NPCs remain
            // discoverable through obstructions.
            device.SetDepthStencil(oldDepth);
            device.SetRenderState(gx::rs::kZEnable, 1);
            device.SetRenderState(gx::rs::kZWrite, 0);
            device.SetRenderState(gx::rs::kZFunc, gx::cmp::kLessEqual);
        }
        else
        {
            device.SetDepthStencil(nullptr);
            device.SetRenderState(gx::rs::kZEnable, 0);
        }

        if (!maskCleared_)
        {
            device.Clear(0, nullptr, gx::clear::kTarget, 0x00000000, 1.0f, 0);
            maskCleared_ = true;
        }

        device.SetRenderState(gx::rs::kAlphaBlend, 0);
        device.SetPixelShader(alphaCutout ? cutoutShader_ : colorShader_);
        device.SetPixelShaderConstantF(0, targets_[index].color, 1);
        device.DrawIndexedPrimitive(args.primType, args.baseVertex, args.minIndex, args.numVerts,
                                    args.startIndex, args.primCount);

        device.SetPixelShader(oldShader);
        device.SetRenderTarget(0, oldTarget);
        device.SetDepthStencil(oldDepth);
        device.SetViewport(oldViewport);
        gx::Release(oldTarget);
        gx::Release(oldDepth);
        gx::Release(oldShader);
    }

    void Outline::EdgePass(gx::Device9 device)
    {
        ScopedDeviceState state(device);

        device.SetRenderState(gx::rs::kZEnable, 0);
        device.SetRenderState(gx::rs::kCullMode, gx::cull::kNone);
        device.SetRenderState(gx::rs::kAlphaBlend, 1);
        device.SetRenderState(gx::rs::kSrcBlend, gx::blend::kSrcAlpha);
        device.SetRenderState(gx::rs::kDestBlend, gx::blend::kInvSrcAlpha);
        device.SetVertexShader(nullptr);
        device.SetTexture(0, mask_.texture);
        device.SetPixelShader(edgeShader_);

        const float constants[4] = {
            1.0f / static_cast<float>(mask_.width),
            1.0f / static_cast<float>(mask_.height),
            thickness_, 0.0f,
        };
        device.SetPixelShaderConstantF(0, constants, 1);
        gx::DrawFullscreenQuad(device);
    }

    void Outline::OnM2Batch(const ev::M2BatchDrawArgs& args)
    {
        if (count_ == 0 || !colorShader_ || !cutoutShader_ || !mask_.surface) return;
        const int index = FindTarget(args.model);
        if (index < 0) return;

        gx::Device9 device(args.device);
        if (!ShouldStampBatch(device)) return;
        StampSilhouette(device, args, index);
    }

    void Outline::OnWorldRenderEnd(const ev::WorldRenderEndArgs& args)
    {
        gx::Device9 device(args.device);
        if (!device) return;

        if (EnsureResources(device) && maskCleared_)
            EdgePass(device);

        // The next frame stamps the targets selected at the end of this one. Rebuilding after the
        // composite also prevents a target change from colouring a stale mask with the new target.
        RebuildTargets();
        maskCleared_ = false;
    }

    void Outline::OnDeviceLost(const ev::DeviceResetArgs&)
    {
        ReleaseResources();
        WLOG_INFO("device-lost resources released");
    }

    void Outline::OnWorldLeave(const ev::WorldLeaveArgs&)
    {
        count_ = 0;
        maskCleared_ = false;
    }

    bool InstallUnitOutline()
    {
        static Outline outline;
        WLOG_INFO("reaction-coloured target and mouseover silhouette installed");
        return true;
    }
}
