/* DrawRibbonDisplayTab — extracted from port_imgui_menu.cpp so the
 * per-tab UI code stays in smaller, navigable chunks. This file is
 * included into the same translation unit and intentionally relies on
 * the surrounding ImGui/Port_* declarations already visible there. */
extern "C" { typedef void (*PortStepCycle)(int); }
typedef const char* (*PortStepValue)(char* buf, size_t cap);

/* One Display-tab settings row: label, then a <value> stepper flanked by
 * < / > buttons. getValue() is read *between* the buttons so a click's new
 * value shows the same frame, matching the hand-written rows. */
static void StepperRow(const char* label, const char* id,
                       PortStepValue getValue, PortStepCycle onCycle) {
    char tag[24];
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    std::snprintf(tag, sizeof(tag), "<##%s", id);
    if (ImGui::Button(tag)) onCycle(-1);
    ImGui::SameLine();
    char buf[24];
    ImGui::Text("%s", getValue(buf, sizeof(buf)));
    ImGui::SameLine();
    std::snprintf(tag, sizeof(tag), ">##%s", id);
    if (ImGui::Button(tag)) onCycle(+1);
}

static void DrawRibbonDisplayTab(void) {
    const bool gpuActive = Port_GPU_IsActive();

    if (ImGui::BeginTable("##display_settings_table", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Window scale - value captured before the buttons (read-before).
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("窗口缩放");
        ImGui::TableSetColumnIndex(1);
        int scale = (int)Port_PPU_WindowScale();
        if (ImGui::Button("<##scale")) Port_PPU_CycleWindowScale(-1);
        ImGui::SameLine(); ImGui::Text("%dx", scale); ImGui::SameLine();
        if (ImGui::Button(">##scale")) Port_PPU_CycleWindowScale(+1);

        StepperRow("滤镜", "filter",
                   [](char*, size_t) { return Port_PPU_PresentationModeName(); },
                   Port_PPU_CyclePresentationMode);

        StepperRow("目标帧率", "fps",
                   [](char* b, size_t n) -> const char* {
                       unsigned fps = Port_Config_TargetFps();
                       if (fps == 0) return "不限";
                       std::snprintf(b, n, "%u", fps);
                       return b;
                   },
                   Port_Config_CycleTargetFps);
        RandoUi_HelpTooltip(
            "画面以多快的频率呈现到显示器。开启“解耦游戏速度”（默认）时，"
            "这里只影响显示流畅度——游戏始终以正常速度运行，“不限”相当于持续快进。"
            "关闭解耦时，此值直接驱动引擎，120 即以 2 倍速运行游戏。");

        /* Decoupled pacing: game logic on a fixed tick grid, presents on the
         * Target FPS grid (port_bios.c). Off = legacy coupled loop. */
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("解耦游戏速度");
        ImGui::TableSetColumnIndex(1);
        {
            bool dec = Port_Config_GetDecoupleRender();
            if (ImGui::Checkbox("##decouple_render", &dec)) {
                Port_Config_SetDecoupleRender(dec);
            }
        }
        RandoUi_HelpTooltip(
            "无论目标帧率或 VSync 如何，游戏逻辑都固定运行在 60 Hz"
            "（主机一致模式下为 GBA 精确帧率）。快进也会更快：引擎会跳过不显示的帧。");

        /* On-screen FPS/TPS counter (DrawFpsOverlay, port_imgui_menu.cpp). */
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("显示 FPS 计数");
        ImGui::TableSetColumnIndex(1);
        {
            bool sf = Port_Config_GetShowFps();
            if (ImGui::Checkbox("##show_fps", &sf)) {
                Port_Config_SetShowFps(sf);
            }
        }
        RandoUi_HelpTooltip(
            "右上角覆盖层。FPS = 每秒显示的帧数（渲染率）；"
            "TPS = 每秒游戏逻辑帧数。解耦开启时，TPS 稳定在 60、FPS 跟随目标帧率，"
            "说明分离生效。游戏偏离正常速度时（快进，或机器性能不足）TPS 会变红。");

        StepperRow("内部倍率", "iscale",
                   [](char* b, size_t n) -> const char* {
                       std::snprintf(b, n, "%ux", (unsigned)Port_Config_InternalScale());
                       return b;
                   },
                   Port_Config_CycleInternalScale);
        RandoUi_HelpTooltip(
            "在最终放大之前，以该整数倍在内部渲染 GBA 帧。"
            "只有在着色器或宽屏路径对画面重采样时，它才会影响采样质量——"
            "默认的最近邻 2D 输出下没有可见效果。除非所用着色器能从超采样获益，"
            "否则保持 1x。");

        StepperRow("宽高比", "aspect",
                   [](char*, size_t) { return Port_Config_AspectModeName(Port_Config_AspectMode()); },
                   Port_Config_CycleAspectMode);

        StepperRow("渲染后端", "renderer",
                   [](char*, size_t) { return Port_Config_RenderBackendName(Port_Config_RenderBackend()); },
                   Port_Config_CycleRenderBackend);
        ImGui::SameLine(); ImGui::TextDisabled("（需重启）");

        /* GPU PPU rasterizer toggle (docs/gpu-rasterizer-design.md). Only
         * meaningful on the SDL_GPU backend; the CPU rasterizer is the
         * automatic fallback. Shown greyed-out otherwise. */
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("GPU 光栅化");
        ImGui::TableSetColumnIndex(1);
        {
            bool raster = Port_Config_GetGpuRaster();
            ImGui::BeginDisabled(!gpuActive);
            if (ImGui::Checkbox("##gpu_raster", &raster)) {
                Port_Config_SetGpuRaster(raster);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled(gpuActive ? "（CPU 回退）" : "（需要 GPU 后端）");
        }

        /* GLES compute rasterizer — opt-in fallback for devices without Vulkan.
         * ~5x slower than CPU on weak GLES GPUs (Adreno 405), so default off and
         * only relevant when the SDL_GPU (Vulkan) backend is NOT active. */
        if (!gpuActive) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("GLES 光栅化（实验）");
            ImGui::TableSetColumnIndex(1);
            bool gles = Port_Config_GetGpuRasterGles();
            if (ImGui::Checkbox("##gpu_raster_gles", &gles)) {
                Port_Config_SetGpuRasterGles(gles);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("（通常比 CPU 慢）");
        }

        StepperRow("背景填充", "bgfill",
                   [](char*, size_t) { return Port_Config_BgFillName(Port_Config_BgFill()); },
                   Port_Config_CycleBgFill);

        StepperRow("CRT 滤镜", "crt",
                   [](char*, size_t) { return Port_PPU_FilterName(); },
                   Port_PPU_CycleFilter);

        ImGui::EndTable();
    }

    if (Port_Config_BgFill() == PORT_BG_FILL_SOLID_COLOR) {
        uint8_t r8 = 0, g8 = 0, b8 = 0;
        Port_Config_BgFillColor(&r8, &g8, &b8);
        int rgb[3] = { (int)r8, (int)g8, (int)b8 };
        bool changed = false;
        ImGui::Indent(150.0f);
        ImGui::PushItemWidth(120.0f);
        if (ImGui::SliderInt("R##bgcol_r", &rgb[0], 0, 255)) {
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SliderInt("G##bgcol_g", &rgb[1], 0, 255)) {
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SliderInt("B##bgcol_b", &rgb[2], 0, 255)) {
            changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImVec4 sw = ImVec4(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f);
        ImGui::ColorButton("##bgcol_preview", sw,
                           ImGuiColorEditFlags_NoPicker |
                           ImGuiColorEditFlags_NoTooltip,
                           ImVec2(22, 22));
        ImGui::Unindent(150.0f);
        if (changed) {
            Port_Config_SetBgFillColor((uint8_t)rgb[0], (uint8_t)rgb[1], (uint8_t)rgb[2]);
        }
    }

    const bool glslpSupported = gpuActive && Port_GPU_SupportsGlslpRuntime();
    if (!glslpSupported) {
        ImGui::Indent(150.0f);
        ImGui::TextDisabled("着色器预设：%s", gpuActive ? "（需要 SPIR-V 后端）" : "（需要 GPU 后端）");
        ImGui::Unindent(150.0f);
    } else {
        static std::vector<std::string> sPresetPaths;
        static std::string              sActivePreset;
        static bool                     sScannedOnce = false;

        auto refresh = [&] {
            sPresetPaths.clear();
            std::error_code ec;
            std::vector<std::string> roots;
            if (const char* base = SDL_GetBasePath()) {
                std::string b = base;
                roots.push_back(b + "shaders");
                roots.push_back(b + "glslp");
                roots.push_back(b);
            }
            roots.emplace_back("shaders");
            roots.emplace_back("glslp");
            roots.emplace_back(".");
            for (const auto& root : roots) {
                std::error_code dec;
                if (!std::filesystem::is_directory(root, dec)) continue;
                /* skip_permission_denied + the non-throwing increment(ec)
                 * overload: the range-for/`operator++` form throws
                 * filesystem_error mid-recursion on an unreadable entry,
                 * and nothing here catches it -> std::terminate. */
                std::filesystem::recursive_directory_iterator it(
                    root, std::filesystem::directory_options::skip_permission_denied, dec);
                const std::filesystem::recursive_directory_iterator end;
                for (; !dec && it != end; it.increment(dec)) {
                    std::error_code fec;
                    if (it->is_regular_file(fec) && it->path().extension() == ".glslp") {
                        sPresetPaths.push_back(it->path().string());
                    }
                }
            }
            std::sort(sPresetPaths.begin(), sPresetPaths.end());
            sPresetPaths.erase(std::unique(sPresetPaths.begin(), sPresetPaths.end()),
                               sPresetPaths.end());
        };
        if (!sScannedOnce) {
            refresh();
            /* Reflect the preset loaded from config at startup (#146) so the
             * list shows it selected and "active: ..." renders. */
            if (sActivePreset.empty() && Port_GlslpRuntime_IsActive()) {
                const char* sp = Port_Config_GetShaderPreset();
                if (sp && sp[0]) sActivePreset = sp;
            }
            sScannedOnce = true;
        }

        ImGui::Indent(150.0f);
        ImGui::TextUnformatted("着色器预设"); ImGui::SameLine();
        if (ImGui::Button("关闭##preset")) {
            Port_GlslpRuntime_Unload();
            sActivePreset.clear();
            Port_Config_SetShaderPreset("");                  /* persist (#146) */
            Port_DebugMenu_ToastFromExternal("着色器预设已关闭");
        }
        ImGui::SameLine();
        if (ImGui::Button("重新扫描##preset")) refresh();
        ImGui::SameLine();
        ImGui::TextDisabled("（找到 %zu 个）", sPresetPaths.size());

        const bool active = Port_GlslpRuntime_IsActive() != 0;
        if (active && !sActivePreset.empty()) {
            ImGui::TextDisabled("  当前：%s", sActivePreset.c_str());
        }

        if (!sPresetPaths.empty()) {
            if (ImGui::BeginListBox("##preset_list",
                    ImVec2(ImGui::GetContentRegionAvail().x - 4.0f, ImGui::GetTextLineHeightWithSpacing() * 3.5f))) {
                for (size_t i = 0; i < sPresetPaths.size(); ++i) {
                    bool selected = (sPresetPaths[i] == sActivePreset)
                                    && Port_GlslpRuntime_IsActive();
                    std::string label = std::filesystem::path(sPresetPaths[i]).filename().string();
                    label += "    "; label += sPresetPaths[i];
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        if (selected) {
                            Port_GlslpRuntime_Unload();
                            sActivePreset.clear();
                            Port_Config_SetShaderPreset("");          /* persist (#146) */
                            Port_DebugMenu_ToastFromExternal("着色器预设已关闭");
                        } else if (Port_GlslpRuntime_Load(sPresetPaths[i].c_str())) {
                            sActivePreset = sPresetPaths[i];
                            Port_Config_SetShaderPreset(sActivePreset.c_str()); /* persist (#146) */
                            Port_DebugMenu_ToastFromExternal("预设已加载");
                        } else {
                            Port_DebugMenu_ToastFromExternal("预设加载失败");
                        }
                    }
                }
                ImGui::EndListBox();
            }
        }
        ImGui::Unindent(150.0f);
    }

    ImGui::Separator();

#if defined(MODE1_GBA_WIDTH) && (MODE1_GBA_WIDTH > 240)
    {
        bool ws = Port_Config_WidescreenEnabled();
        if (ImGui::Checkbox("宽屏（试验中）", &ws)) {
            Port_Config_SetWidescreenEnabled(ws);
            Port_PPU_ApplyWindowScale();
            Port_DebugMenu_ToastFromExternal(ws ? "宽屏已启用" : "宽屏已禁用");
        }
        ImGui::SameLine();
    }
#endif

    bool fs = Port_PPU_IsFullscreen();
    if (ImGui::Checkbox("全屏", &fs)) {
        Port_PPU_ToggleFullscreen();
        Port_Config_SetFullscreen(Port_PPU_IsFullscreen());   /* persist (#146) */
    }
    ImGui::SameLine();

    bool hideCur = Port_Config_GetFullscreenHideCursor();
    if (ImGui::Checkbox("全屏时隐藏光标", &hideCur)) {
        Port_Config_SetFullscreenHideCursor(hideCur);         /* persist */
        Port_PPU_ApplyCursorVisibility();                     /* live, if already fullscreen */
    }
    ImGui::SameLine();

    bool vsync = Port_PPU_VSyncEnabled();
    if (ImGui::Checkbox("垂直同步", &vsync)) {
        Port_PPU_SetVSync(vsync);
        Port_Config_SetVSync(vsync);                          /* persist (#146) */
    }
    ImGui::SameLine();

    bool drp = Port_DiscordRpc_IsEnabled();
    if (ImGui::Checkbox("Discord RPC", &drp)) {
        Port_DiscordRpc_SetEnabled(drp);
        Port_Config_SetDiscordRpc(drp);                       /* persist (#146) */
    }
    RandoUi_HelpTooltip(
        "把当前所在区域与心/卢比数量作为 Rich Presence 状态发布到 Discord。\n\n"
        "需要构建中编译进一个 Discord 应用 ID（discord_app_id.txt，默认不提供），"
        "因此未附带该文件的发布版中此功能保持停用。目前在 Windows 上也是空操作"
        "（命名管道 IPC 客户端尚未实现）。");

    bool colourCorrect = Port_PPU_ColorCorrectionEnabled();
    if (ImGui::Checkbox("GBA 色彩校正", &colourCorrect)) {
        Port_PPU_SetColorCorrection(colourCorrect);           /* live */
        Port_Config_SetColorCorrection(colourCorrect);        /* persist */
    }
    RandoUi_HelpTooltip(
        "按 GBA 昏暗、高伽马的反射式 LCD 响应曲线重新映射原始 15 位调色板，"
        "让颜色与掌机一致，而不是在现代 sRGB 显示器上显得过亮。默认开启。");

    bool lcdPersist = Port_Config_GetLcdPersistence();
    if (ImGui::Checkbox("LCD 残影", &lcdPersist)) {
        Port_Config_SetLcdPersistence(lcdPersist);            /* persist */
        Port_PPU_SetPersistence(lcdPersist, Port_Config_GetLcdPersistenceRho());
    }
    RandoUi_HelpTooltip(
        "通过把每一帧与上一帧混合，重现 GBA LCD 的慢速像素响应（拖影）。"
        "部分闪烁/抖动透明效果是按此设计的。默认关闭。");
    if (lcdPersist) {
        float rho = Port_Config_GetLcdPersistenceRho();
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::SliderFloat("残影强度", &rho, 0.0f, 0.95f, "%.2f")) {
            Port_Config_SetLcdPersistenceRho(rho);            /* persist */
            Port_PPU_SetPersistence(true, rho);               /* live */
        }
    }

    bool holdAdv = Port_Config_GetHoldToAdvanceText();
    if (ImGui::Checkbox("按住推进文本", &holdAdv)) {
        Port_Config_SetHoldToAdvanceText(holdAdv);
    }
    RandoUi_HelpTooltip(
        "开启后，按住推进键（A / B / R / 十字键）会让文本持续翻页，"
        "而不必每页重新按一次。默认关闭以保持 GBA 精确的计时（用于计时演出场景）。");
}
