#include "PCH.h"
#include "Menu.h"
#include "GUI.h"
#include "AudioPlayer/AudioPlayer.h"
#pragma execution_character_set("utf-8")

namespace Menu {
    std::string StatusMessage = "";
    BNKManager* CurrentBnk = nullptr;
    Searcher* CurrentSearcher = nullptr;
    static char search_buffer[128] = "";
    static char searcher_search_buffer[128] = "";
    static ImGuiTableSortSpecs* s_SortSpecs = nullptr;
    static std::vector<std::pair<size_t, BNK::WemFile>> filtered_wems;
    static std::string last_search;

    std::vector<int> selected_indices = {};
    int selected_index = -1;
    int last_selected_index = -1;

    void DrawTitleBar(GLFWwindow* window, const char* title, float width) {
        constexpr float height = 36.0f;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.08f, 0.98f));

        ImGui::Begin("TitleBar", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::Text(">");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
        ImGui::Text(title);

        float buttonWidth = 32;
        float spacing = 4;
        float totalButtonsWidth = buttonWidth * 2 + spacing;

        ImGui::SameLine();

        ImGui::SetCursorPosX(width - totalButtonsWidth - 10);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.40f, 0.45f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.30f, 0.35f, 1.0f));

        if (ImGui::Button("_", ImVec2(buttonWidth, 22))) {
            HWND hwnd = glfwGetWin32Window(window);
            ShowWindow(hwnd, SW_MINIMIZE);
        }

        ImGui::SameLine();

        if (ImGui::Button("X", ImVec2(buttonWidth, 22))) {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        GUI::HandleWindowDrag(window, ImVec2(0, 0), ImVec2(width - buttonWidth * 2 + spacing, height));

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

	void DrawMainWindow(GLFWwindow* window) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::GetStyle().FrameRounding = 5.f;
        ImGui::GetStyle().TabBarBorderSize = 0.f;

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        DrawTitleBar(window, "Growl", windowWidth);

        ImGui::Begin("GrowlMainWindow", (bool*)true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::SetWindowSize(ImVec2(windowWidth, windowHeight - 36));
        ImGui::SetWindowPos(ImVec2(0, 36), ImGuiCond_FirstUseEver);

        ImVec2 AvailRegion = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTabBar("##GrowlTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {

            if (ImGui::BeginTabItem("Manager")) {

                if (ImGui::Button("Load BNK")) {
                    std::string path = Utils::OpenFileDialog("bnk;");
                    if (!path.empty()) {
                        try {
                            delete CurrentBnk;
                            CurrentBnk = new BNKManager(path);
                            filtered_wems.clear();
                            selected_indices.clear();
                            last_selected_index = -1;
                            selected_index = -1;
                            StatusMessage = "Loaded BNK: " + path;
                        }
                        catch (const std::exception& ex) {
                            StatusMessage = ex.what();
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Settings")) {
                    ImGui::OpenPopup("SettingsPopup");
                }
                if (ImGui::BeginPopup("SettingsPopup")) {

                    ImGui::SeparatorText("SoundbanksInfo##Setup");
                    {
                        ImGui::Text("XML file for viewing shortname by ID");
                        if (ImGui::Button("Select##SoundbanksInfoPath")) {
                            std::string path = Utils::OpenFileDialog("xml");
                            if (!path.empty()) {
                                Config::Instance().SetSoundbanksInfoPath(path);
                            }
                        }

                        ImGui::SameLine();

                        std::string soundbanksInfoPath = Config::Instance().GetSoundbanksInfoPath();
                        ImGui::Text("- %s", !soundbanksInfoPath.empty() ? soundbanksInfoPath.c_str() : std::string("").c_str());
                    }

                    ImGui::SeparatorText("Subtitles##Setup");
                    {
                        ImGui::Text("INI file for viewing subtitles by shortname (requires SoundbanksInfo also)");
                        if (ImGui::Button("Select##SubtitlesPath")) {
                            std::string path = Utils::OpenFileDialog("");
                            if (!path.empty()) {
                                Config::Instance().SetSubtitlesPath(path);
                            }
                        }

                        ImGui::SameLine();

                        std::string subtitlesPath = Config::Instance().GetSubtitlesPath();
                        ImGui::Text("- %s", !subtitlesPath.empty() ? subtitlesPath.c_str() : std::string("").c_str());
                    }

                    ImGui::SeparatorText("Wwise Project & WwiseCLI##Setup");
                    {
                        ImGui::Text("Needed to convert .wav to .wem for replacement");
                        if (ImGui::Button("Select##WwiseProjectPath")) {
                            std::string path = Utils::OpenFileDialog("wproj");
                            if (!path.empty()) {
                                Config::Instance().SetWwiseProjectPath(path);
                            }
                        }

                        ImGui::SameLine();

                        std::string wwiseProjectPath = Config::Instance().GetWwiseProjectPath();
                        ImGui::Text("- %s", !wwiseProjectPath.empty() ? wwiseProjectPath.c_str() : std::string("").c_str());
                        if (ImGui::Button("Select##WwiseCLIPath")) {
                            std::string path = Utils::OpenFileDialog("exe");
                            if (!path.empty()) {
                                Config::Instance().SetWwiseCLIPath(path);
                            }
                        }

                        ImGui::SameLine();

                        std::string wwiseCLIPath = Config::Instance().GetWwiseCLIPath();
                        ImGui::Text("- %s", !wwiseCLIPath.empty() ? wwiseCLIPath.c_str() : std::string("").c_str());
                    }
                    ImGui::EndPopup();
                }

                AvailRegion = ImGui::GetContentRegionAvail();

                if (CurrentBnk && CurrentBnk->GetIsValid() && (!CurrentBnk->GetWems().empty() && !CurrentBnk->GetEntries().empty())) {

                    std::vector<BNK::WemFile> wems = CurrentBnk->GetWems();

                    if (last_search != search_buffer || filtered_wems.empty()) {
                        filtered_wems.clear();
                        std::vector<BNK::WemFile> wems = CurrentBnk->GetWems();
                        for (size_t i = 0; i < wems.size(); ++i) {
                            std::string id_str = std::to_string(wems[i].id);
                            std::string shortname = wems[i].shortname;
                            std::string subtitle = !wems[i].subtitles.empty() ? wems[i].subtitles[0] : "";

                            if (strlen(search_buffer) == 0 ||
                                id_str.find(search_buffer) != std::string::npos ||
                                shortname.find(search_buffer) != std::string::npos ||
                                subtitle.find(search_buffer) != std::string::npos)
                            {
                                filtered_wems.emplace_back(i, wems[i]);
                            }
                        }

                        last_search = search_buffer;
                    }

                    ImVec2 table_size = ImVec2(AvailRegion.x, AvailRegion.y * 0.75f);

                    ImGui::BeginChild("SoundListTableRegion", table_size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    ImGui::SeparatorText(std::string(reinterpret_cast<const char*>(std::filesystem::u8path(CurrentBnk->GetPath()).filename().u8string().c_str())).c_str());
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##search", "Search by ID, shortname or subtitles...", search_buffer, IM_ARRAYSIZE(search_buffer));
                    if (ImGui::BeginTable("SoundListTable", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoHostExtendY |
                        ImGuiTableFlags_Sortable))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
                        ImGui::TableSetupColumn("WEM ID", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn("Shortname", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn("Subtitles", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableHeadersRow();

                        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                            s_SortSpecs = sort_specs;
                            if (s_SortSpecs->SpecsDirty && s_SortSpecs->SpecsCount > 0) {
                                const ImGuiTableColumnSortSpecs& spec = s_SortSpecs->Specs[0];

                                std::sort(filtered_wems.begin(), filtered_wems.end(),
                                    [&](const std::pair<size_t, BNK::WemFile>& a, const std::pair<size_t, BNK::WemFile>& b) {
                                        const BNK::WemFile& wa = a.second;
                                        const BNK::WemFile& wb = b.second;

                                        switch (spec.ColumnIndex) {
                                        case 0:
                                            return (spec.SortDirection == ImGuiSortDirection_Ascending) ? a.first < b.first : a.first > b.first;
                                        case 1:
                                            return (spec.SortDirection == ImGuiSortDirection_Ascending) ? wa.id < wb.id : wa.id > wb.id;

                                        case 2: {
                                            std::string sa = wa.shortname;
                                            std::string sb = wb.shortname;
                                            return (spec.SortDirection == ImGuiSortDirection_Ascending) ? sa < sb : sa > sb;
                                        }

                                        case 3:
                                            return (spec.SortDirection == ImGuiSortDirection_Ascending) ? wa.duration < wb.duration : wa.duration > wb.duration;

                                        case 4: {
                                            std::string sa = !wa.subtitles.empty() ? wa.subtitles[0] : "";
                                            std::string sb = !wb.subtitles.empty() ? wb.subtitles[0] : "";
                                            return (spec.SortDirection == ImGuiSortDirection_Ascending) ? sa < sb : sa > sb;
                                        }
                                        default:
                                            return false;
                                        }
                                    });

                                s_SortSpecs->SpecsDirty = false;
                            }
                        }

                        int max_digits = std::to_string(wems.size()).size();

                        for (size_t display_idx = 0; display_idx < filtered_wems.size(); ++display_idx) {
                            size_t real_idx = filtered_wems[display_idx].first;
                            const BNK::WemFile& wem = filtered_wems[display_idx].second;

                            ImGui::TableNextRow();
                            ImGui::PushID((int)real_idx);

                            bool is_selected = std::find(selected_indices.begin(), selected_indices.end(), (int)real_idx) != selected_indices.end();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("#%*zu", max_digits, real_idx + 1);
                            ImGui::SameLine();

                            std::string play_label = AudioPlayer::Instance().IsPlaying() && AudioPlayer::Instance().GetCurrentFile() == std::to_string(wem.id) ? "[]##play" + std::to_string(real_idx) : "|>##play" + std::to_string(real_idx);
                            if (ImGui::Button(play_label.c_str())) {
                                if (AudioPlayer::Instance().IsPlaying() && AudioPlayer::Instance().GetCurrentFile() == std::to_string(wem.id)) {
                                    AudioPlayer::Instance().Stop();
                                }
                                else {
                                    std::string out_path = Utils::GetAppDirectory() + "\\growl_tmpfiles\\" + std::to_string(wem.id);
                                    std::filesystem::create_directories(Utils::GetAppDirectory() + "\\growl_tmpfiles");
                                    CurrentBnk->ExtractWem(real_idx, out_path + ".wem");
                                    std::filesystem::remove(out_path + ".wem");
                                    AudioPlayer::Instance().Play(out_path + ".ogg");
                                }
                            }
                            if (AudioPlayer::Instance().IsPlaying() && AudioPlayer::Instance().GetCurrentFile() == std::to_string(wem.id) && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text(Utils::FormatMillisecondsToTime(AudioPlayer::Instance().GetCurrentPositionMilliseconds()).c_str());
                                ImGui::EndTooltip();
                            }

                            ImGui::SameLine();
                            if (ImGui::Selectable("", is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 0))) {
                                if (io.KeyShift && last_selected_index >= 0) {
                                    int current_real_idx = (int)filtered_wems[display_idx].first;

                                    int start = -1, end = -1;
                                    for (size_t i = 0; i < filtered_wems.size(); ++i) {
                                        int idx = (int)filtered_wems[i].first;
                                        if (idx == last_selected_index) start = (int)i;
                                        if (idx == current_real_idx) end = (int)i;
                                    }

                                    if (start != -1 && end != -1) {
                                        if (start > end) std::swap(start, end);
                                        for (int i = start; i <= end; ++i) {
                                            int real = (int)filtered_wems[i].first;
                                            if (std::find(selected_indices.begin(), selected_indices.end(), real) == selected_indices.end())
                                                selected_indices.push_back(real);
                                        }
                                    }
                                }
                                else if (io.KeyCtrl) {
                                    if (is_selected)
                                        selected_indices.erase(std::remove(selected_indices.begin(), selected_indices.end(), (int)real_idx), selected_indices.end());
                                    else
                                        selected_indices.push_back((int)real_idx);
                                    last_selected_index = (int)real_idx;
                                }
                                else {
                                    selected_indices.clear();
                                    selected_indices.push_back((int)real_idx);
                                    last_selected_index = (int)real_idx;
                                }
                            }
                            if (ImGui::BeginPopupContextItem()) {
                                if (ImGui::MenuItem("Copy ID")) {
                                    ImGui::SetClipboardText(std::to_string(wem.id).c_str());
                                }
                                if (!wem.shortname.empty()) {
                                    if (ImGui::MenuItem("Copy Shortname")) {
                                        ImGui::SetClipboardText(wem.shortname.c_str());
                                    }
                                }
                                if (!wem.subtitles.empty()) {
                                    if (ImGui::MenuItem("Copy Subtitle")) {
                                        ImGui::SetClipboardText(wem.subtitles[0].c_str());
                                    }
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u", wem.id);

                            ImGui::TableSetColumnIndex(2);
                            std::string shortname = wem.shortname;
                            ImGui::TextUnformatted(shortname.c_str());

                            ImGui::TableSetColumnIndex(3);
                            ImGui::TextUnformatted(wem.duration_format.c_str());

                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextUnformatted(!wem.subtitles.empty() ? wem.subtitles[0].c_str() : "");

                            ImGui::PopID();
                        }

                        ImGui::EndTable();
                    }

                    ImGui::EndChild();

                    table_size = ImVec2(AvailRegion.x, AvailRegion.y * 0.20f);
                    ImGui::BeginChild("SoundSelectionRegion", table_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                    bool is_active_selection = selected_indices.size() > 0;
                    if (is_active_selection) ImGui::Text("Selected WEM #%d (ID: %u%s)", selected_indices[0]+1, wems[selected_indices[0]].id, selected_indices.size() > 1 ? std::string(" and " + std::to_string((int)selected_indices.size() - 1) + " other...").c_str() : "");
                    else ImGui::Text("No selection");

                    float itemWidth = (AvailRegion.x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 8.0f;

                    ImGui::PushItemWidth(itemWidth);

                    if (ImGui::Button("Extract...") && is_active_selection) {
                        std::string out_path = Utils::OpenFolderDialog();
                        if (!out_path.empty()) {
                            try {
                                std::filesystem::path p(std::filesystem::u8path(out_path));
                                int extractionname = Config::Instance().GetExtractionName();
                                for (int i : selected_indices) {
                                    std::string name;
                                    if (extractionname == Utils::ExtractionName::ExtractShortname && !wems[i].shortname.empty()) {
                                        name = wems[i].shortname;
                                    }
                                    else {
                                        name = std::to_string(wems[i].id);
                                    }
                                    CurrentBnk->ExtractWem(i, (p / name).replace_extension(".wem").string());
                                }
                                StatusMessage = selected_indices.size() > 1 ? "Extracted " + std::to_string((int)selected_indices.size()) + " WEMs in " + out_path : "Extracted WEM in " + out_path;
                            }
                            catch (const std::exception& ex) {
                                StatusMessage = ex.what();
                            }
                        }
                    }
                    ImGui::PopItemWidth();

                    ImGui::SameLine();

                    int extractiontype = Config::Instance().GetExtractionType();
                    ImGui::PushItemWidth(itemWidth);
                    if (ImGui::Combo("##Extraction Type", &extractiontype, "WEM only\0OGG only\0WEM and OGG\0")) {
                        Config::Instance().SetExtractionType(extractiontype);
                    }
                    ImGui::PopItemWidth();

                    ImGui::SameLine();

                    ImGui::Text("with name: ");

                    ImGui::SameLine();

                    int extractionname = Config::Instance().GetExtractionName();
                    ImGui::PushItemWidth(itemWidth);
                    if (ImGui::Combo("##Extraction Name", &extractionname, "ID\0Shortname")) {
                        Config::Instance().SetExtractionName(extractionname);
                    }
                    ImGui::PopItemWidth();

                    if (ImGui::Button("Replace...") && is_active_selection) {
                        std::vector<std::string> wem_paths = selected_indices.size() > 1 ? Utils::OpenMultipleFilesDialog("wem;wav") : std::vector<std::string>{ Utils::OpenFileDialog("wem;wav") };
                        if (!wem_paths.empty() && !wem_paths[0].empty()) {
                            std::vector<std::string> wav_paths;
                            for (size_t i = 0; i < wem_paths.size(); i++) {
                                std::string wem_path = wem_paths[i];
                                if (std::filesystem::u8path(wem_path).extension().string() == std::string(".wav")) {
                                    wav_paths.push_back(wem_path);
                                    std::filesystem::path projectPath = std::filesystem::u8path(Config::Instance().GetWwiseProjectPath());
                                    std::filesystem::create_directories(projectPath.parent_path() / "growl_convert" / "input");
                                    std::filesystem::create_directories(projectPath.parent_path() / "growl_convert" / "output" / "Windows");
                                    std::filesystem::path targetPath = std::filesystem::path(projectPath.parent_path() / "growl_convert" / "input" / std::filesystem::u8path(wem_path).filename());

                                    std::filesystem::copy_file(std::filesystem::u8path(wem_path), targetPath, std::filesystem::copy_options::overwrite_existing);
                                    wem_paths[i] = std::filesystem::path(projectPath.parent_path() / "growl_convert" / "output" / "Windows" / std::filesystem::u8path(wem_path).replace_extension(".wem").filename()).string();
                                }
                            }
                            if (!wav_paths.empty()) {
                                std::string cliPath = std::string(reinterpret_cast<const char*>(std::filesystem::u8path(Config::Instance().GetWwiseCLIPath()).u8string().c_str()));
                                std::string projectPath = std::string(reinterpret_cast<const char*>(std::filesystem::u8path(Config::Instance().GetWwiseProjectPath()).u8string().c_str()));
                                std::filesystem::path basePath = std::filesystem::u8path(projectPath).parent_path();
                                std::string wsourcesPath = std::string(reinterpret_cast<const char*>((basePath / "growl_convert" / "list.wsources").u8string().c_str()));
                                std::string outputPath = std::string(reinterpret_cast<const char*>((basePath / "growl_convert" / "output").u8string().c_str()));
                                std::string commandLine =
                                    "\"" + cliPath + "\" "
                                    + "\"" + projectPath + "\""
                                    + " -ConvertExternalSources \"" + wsourcesPath + "\""
                                    + " -ExternalSourcesOutput \"" + outputPath + "\""
                                    + " --platform \"Windows\""
                                    + " --language \"English(US)\""
                                    + " -GenerateSoundBanks";

                                Utils::CreateWwiseExternalSources(wav_paths, wsourcesPath);

                                STARTUPINFOA si = { sizeof(si) };
                                PROCESS_INFORMATION pi = {};
                                si.dwFlags = STARTF_USESHOWWINDOW;
                                si.wShowWindow = SW_HIDE;

                                char* mutableCmd = _strdup(commandLine.c_str());

                                BOOL success = CreateProcessA(
                                    NULL,
                                    mutableCmd,
                                    NULL, NULL,
                                    FALSE,
                                    CREATE_NO_WINDOW,
                                    NULL, NULL,
                                    &si, &pi
                                );

                                free(mutableCmd);
                                WaitForSingleObject(pi.hProcess, INFINITE);
                                CloseHandle(pi.hProcess);
                                CloseHandle(pi.hThread);
                            }

                            std::string out_path = Utils::SaveFileDialog("bnk");
                            if (!out_path.empty()) {
                                try {
                                    std::vector<std::pair<int, std::string>> replace_wem_data;
                                    for (std::string wem_path : wem_paths) {
                                        std::filesystem::path p = std::filesystem::u8path(wem_path);
                                        std::string stem_str = std::string(reinterpret_cast<const char*>(p.stem().u8string().c_str()));
                                        for (int i : selected_indices) {
                                            if (atoi(stem_str.c_str()) == wems[i].id || stem_str == wems[i].shortname) {
                                                replace_wem_data.emplace_back(i, std::string(reinterpret_cast<const char*>(p.u8string().c_str())));
                                            }
                                        }
                                    }
                                    CurrentBnk->ReplaceWems(replace_wem_data, out_path);
                                    delete CurrentBnk;
                                    CurrentBnk = new BNKManager(out_path);
                                    StatusMessage = "Replaced " + std::to_string(replace_wem_data.size()) + " WEMs and Saved BNK : " + out_path;
                                }
                                catch (const std::exception& ex) {
                                    StatusMessage = ex.what();
                                }
                            }
                            std::filesystem::path tempfolderpath = std::filesystem::u8path(Config::Instance().GetWwiseProjectPath()).parent_path() / "growl_convert";
                            if (std::filesystem::exists(tempfolderpath)) {
                                std::filesystem::remove_all(tempfolderpath);
                            }
                        }
                    }

                    ImGui::Separator();

                    float audiovolume = Config::Instance().GetVolume();
                    ImGui::PushItemWidth(itemWidth);
                    if (ImGui::SliderFloat("Volume", &audiovolume, 0, 1)) {
                        AudioPlayer::Instance().SetVolume(audiovolume);
                        Config::Instance().SetVolume(audiovolume);
                    }
                    ImGui::PopItemWidth();

                    ImGui::EndChild();

                }
                

                

                ImGui::EndTabItem();
            }


            if (ImGui::BeginTabItem("Searcher")) {

                if (ImGui::Button("Start")) {
                    try {
                        delete CurrentSearcher;
                        CurrentSearcher = new Searcher();
                    }
                    catch (const std::exception& ex) {
                        StatusMessage = ex.what();
                    }
                }

                ImGui::SameLine();
                
                if (ImGui::Button("Settings##Searcher")) {
                    ImGui::OpenPopup("SettingsPopup");
                }
                if (ImGui::BeginPopup("SettingsPopup")) {
                
                    ImGui::SeparatorText("SoundbanksInfo##Setup");
                    {
                        ImGui::Text("XML file for viewing shortname by ID");
                        if (ImGui::Button("Select##SoundbanksInfoPath")) {
                            std::string path = Utils::OpenFileDialog("xml");
                            if (!path.empty()) {
                                Config::Instance().SetSoundbanksInfoPath(path);
                            }
                        }
                
                        ImGui::SameLine();
                
                        std::string soundbanksInfoPath = Config::Instance().GetSoundbanksInfoPath();
                        ImGui::Text("- %s", !soundbanksInfoPath.empty() ? soundbanksInfoPath.c_str() : std::string("").c_str());
                    }
                
                    ImGui::SeparatorText("Subtitles##Setup");
                    {
                        ImGui::Text("INI file for viewing subtitles by shortname (requires SoundbanksInfo also)");
                        if (ImGui::Button("Select##SubtitlesPath")) {
                            std::string path = Utils::OpenFileDialog("");
                            if (!path.empty()) {
                                Config::Instance().SetSubtitlesPath(path);
                            }
                        }
                
                        ImGui::SameLine();
                
                        std::string subtitlesPath = Config::Instance().GetSubtitlesPath();
                        ImGui::Text("- %s", !subtitlesPath.empty() ? subtitlesPath.c_str() : std::string("").c_str());
                    }
                
                    ImGui::SeparatorText("Wwise Project & WwiseCLI##Setup");
                    {
                        ImGui::Text("Needed to convert .wav to .wem for replacement");
                        if (ImGui::Button("Select##WwiseProjectPath")) {
                            std::string path = Utils::OpenFileDialog("wproj");
                            if (!path.empty()) {
                                Config::Instance().SetWwiseProjectPath(path);
                            }
                        }
                
                        ImGui::SameLine();
                
                        std::string wwiseProjectPath = Config::Instance().GetWwiseProjectPath();
                        ImGui::Text("- %s", !wwiseProjectPath.empty() ? wwiseProjectPath.c_str() : std::string("").c_str());
                        if (ImGui::Button("Select##WwiseCLIPath")) {
                            std::string path = Utils::OpenFileDialog("exe");
                            if (!path.empty()) {
                                Config::Instance().SetWwiseCLIPath(path);
                            }
                        }
                
                        ImGui::SameLine();
                
                        std::string wwiseCLIPath = Config::Instance().GetWwiseCLIPath();
                        ImGui::Text("- %s", !wwiseCLIPath.empty() ? wwiseCLIPath.c_str() : std::string("").c_str());
                    }
                    ImGui::EndPopup();
                }

                

                if (CurrentSearcher && CurrentSearcher->GetIsValid() && CurrentSearcher->GetIsSoundbanksInfoValid()) {

                    std::vector<Utils::SoundBankInfo> ParsedList = CurrentSearcher->GetParsedList();
                    std::vector<BNK::SubtitleGroup> ParsedSubtitles = CurrentSearcher->GetParsedSubtitles();

                    static std::unordered_map<std::string, bool> bankOpenStates;
                    static std::unordered_map<std::string, bool> fileOpenStates;
                    static std::string lastSearch = "";

                    ImVec2 table_size = ImVec2(AvailRegion.x, AvailRegion.y * 0.75f);
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##searchSearcher", "Search by shortname or subtitles...", searcher_search_buffer, IM_ARRAYSIZE(searcher_search_buffer));
                    ImGui::BeginChild("SoundSelectionRegion", table_size, true);

                    

                    std::string searchTerm = searcher_search_buffer;
                    auto ToLower = [](const std::string& str) {
                        std::string result = str;
                        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
                        return result;
                        };

                    std::string searchLower = ToLower(searchTerm);
                    if (searchLower != lastSearch) {
                        lastSearch = searchLower;
                        bankOpenStates.clear();
                        fileOpenStates.clear();

                        if (!searchLower.empty()) {
                            for (size_t bankIndex = 0; bankIndex < ParsedList.size(); ++bankIndex) {
                                const auto& bank = ParsedList[bankIndex];

                                bool anyMatch = false;
                                std::vector<size_t> matchingFileIndices;

                                for (size_t fileIndex = 0; fileIndex < bank.files.size(); ++fileIndex) {
                                    const auto& file = bank.files[fileIndex];

                                    std::string shortLower = ToLower(file.shortName);
                                    if (shortLower.find(searchLower) != std::string::npos) {
                                        anyMatch = true;
                                        matchingFileIndices.push_back(fileIndex);
                                        continue;
                                    }

                                    auto subtitles = Utils::GetSubtitlesByShortName(ParsedSubtitles, file.shortName);
                                    for (const auto& subtitle : subtitles) {
                                        if (ToLower(subtitle).find(searchLower) != std::string::npos) {
                                            anyMatch = true;
                                            matchingFileIndices.push_back(fileIndex);
                                            break;
                                        }
                                    }
                                }

                                std::string bankKey = "bank_" + std::to_string(bankIndex);
                                if (anyMatch) {
                                    bankOpenStates[bankKey] = true;
                                    for (auto fileIndex : matchingFileIndices) {
                                        std::string fileKey = "file_" + std::to_string(bank.files[fileIndex].id);
                                        fileOpenStates[fileKey] = true;
                                    }
                                }
                            }
                        }
                    }

                    for (size_t bankIndex = 0; bankIndex < ParsedList.size(); ++bankIndex) {
                        const auto& bank = ParsedList[bankIndex];
                        std::string bankKey = "bank_" + std::to_string(bankIndex);

                        if (!searchLower.empty()) {
                            if (bankOpenStates.find(bankKey) == bankOpenStates.end()) continue;
                        }

                        ImGuiTreeNodeFlags bankFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                        if (bankOpenStates[bankKey]) {
                            bankFlags |= ImGuiTreeNodeFlags_DefaultOpen;
                        }

                        bool bankOpen = ImGui::TreeNodeEx(bankKey.c_str(), bankFlags, "%s", bank.name.c_str());

                        if (ImGui::BeginPopupContextItem(("##CopyPopup" + bankKey).c_str())) {
                            if (!bank.name.empty()) {
                                if (ImGui::MenuItem(("Copy Bankname##" + bankKey).c_str())) {
                                    ImGui::SetClipboardText(bank.name.c_str());
                                }
                            }
                            ImGui::EndPopup();
                        }

                        if (ImGui::IsItemClicked()) {
                            bankOpenStates[bankKey] = !bankOpenStates[bankKey];
                        }

                        if (bankOpen) {
                            for (size_t fileIndex = 0; fileIndex < bank.files.size(); ++fileIndex) {
                                const auto& file = bank.files[fileIndex];
                                std::string fileKey = "file_" + std::to_string(file.id);

                                if (!searchLower.empty()) {
                                    if (fileOpenStates.find(fileKey) == fileOpenStates.end()) continue;
                                }

                                ImGuiTreeNodeFlags fileFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                                auto subtitles = Utils::GetSubtitlesByShortName(ParsedSubtitles, file.shortName);
                                bool isLeaf = subtitles.empty();
                                if (isLeaf) fileFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                                if (fileOpenStates[fileKey]) {
                                    fileFlags |= ImGuiTreeNodeFlags_DefaultOpen;
                                }

                                if (isLeaf) {
                                    ImGui::TreeNodeEx(fileKey.c_str(), fileFlags, "%s", file.shortName.c_str());

                                    if (ImGui::BeginPopupContextItem(("##CopyPopup" + fileKey).c_str())) {
                                        if (!bank.name.empty()) {
                                            if (ImGui::MenuItem(("Copy Bankname##" + bankKey).c_str())) {
                                                ImGui::SetClipboardText(bank.name.c_str());
                                            }
                                        }
                                        if (!file.shortName.empty()) {
                                            if (ImGui::MenuItem(("Copy Shortname##" + fileKey).c_str())) {
                                                ImGui::SetClipboardText(file.shortName.c_str());
                                            }
                                        }
                                        if (!subtitles.empty()) {
                                            if (ImGui::MenuItem(("Copy Subtitle##" + fileKey).c_str())) {
                                                ImGui::SetClipboardText(subtitles[0].c_str());
                                            }
                                        }
                                        ImGui::EndPopup();
                                    }
                                }
                                else {
                                    bool fileOpen = ImGui::TreeNodeEx(fileKey.c_str(), fileFlags, "%s", file.shortName.c_str());

                                    if (ImGui::BeginPopupContextItem(("##CopyPopup" + fileKey).c_str())) {
                                        if (!bank.name.empty()) {
                                            if (ImGui::MenuItem(("Copy Bankname##" + bankKey).c_str())) {
                                                ImGui::SetClipboardText(bank.name.c_str());
                                            }
                                        }
                                        if (!file.shortName.empty()) {
                                            if (ImGui::MenuItem(("Copy Shortname##" + fileKey).c_str())) {
                                                ImGui::SetClipboardText(file.shortName.c_str());
                                            }
                                        }
                                        if (!subtitles.empty()) {
                                            if (ImGui::MenuItem(("Copy Subtitle##" + fileKey).c_str())) {
                                                ImGui::SetClipboardText(subtitles[0].c_str());
                                            }
                                        }
                                        ImGui::EndPopup();
                                    }

                                    if (ImGui::IsItemClicked()) {
                                        fileOpenStates[fileKey] = !fileOpenStates[fileKey];
                                    }

                                    if (fileOpen) {
                                        for (const auto& subtitle : subtitles) {
                                            ImGui::BulletText("%s", subtitle.c_str());
                                        }
                                        if (ImGui::BeginPopupContextItem(("##CopyPopup" + fileKey + "_sub").c_str())) {
                                            if (!bank.name.empty()) {
                                                if (ImGui::MenuItem(("Copy Bankname##" + bankKey).c_str())) {
                                                    ImGui::SetClipboardText(bank.name.c_str());
                                                }
                                            }
                                            if (!file.shortName.empty()) {
                                                if (ImGui::MenuItem(("Copy Shortname##" + fileKey).c_str())) {
                                                    ImGui::SetClipboardText(file.shortName.c_str());
                                                }
                                            }
                                            if (!subtitles.empty()) {
                                                if (ImGui::MenuItem(("Copy Subtitle##" + fileKey).c_str())) {
                                                    ImGui::SetClipboardText(subtitles[0].c_str());
                                                }
                                            }
                                            ImGui::EndPopup();
                                        }
                                        ImGui::TreePop();
                                    }
                                    
                                }
                                
                            }
                            ImGui::TreePop();

                        }
                    }

                    ImGui::EndChild();
                }
                else {
                    if (Config::Instance().GetSoundbanksInfoPath().empty()) {
                        ImGui::Text("Missing SoundbanksInfo for search... Please, select it.");
                    }
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        
        

        ImGui::SetNextWindowPos(ImVec2(0, windowHeight - 30));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 30));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
        ImGui::BeginChild("StatusBar", ImVec2(AvailRegion.x, AvailRegion.y), true,
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings);
        
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "%s", StatusMessage.c_str());
        
        float window_width = ImGui::GetWindowWidth();
        if (CurrentBnk && CurrentBnk->GetIsValid() && !CurrentBnk->GetWems().empty()) {
            std::string idx_count = std::to_string(selected_indices.size());
            float text_width = ImGui::CalcTextSize(("Sel: " + idx_count + "/" + std::to_string(CurrentBnk->GetWems().size())).c_str()).x;
            ImGui::SameLine(window_width - text_width - ImGui::GetStyle().WindowPadding.x);

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), ("Sel: " + idx_count + "/" + std::to_string(CurrentBnk->GetWems().size())).c_str());
        }
        
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        
	}
}