// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// One player-facing launcher for installation, updates and Play. The prepared
// private executable replaces this process only after Vulkan has shut down.
#include "vulkanexamplebase.h"
#include "ps5_samples.h"
#include "installer_pipeline.h"
#include "installer_ui_assets.h"
#include "installer_music.h"
#include "installer_updates.h"
#include "launcher_preferences.h"

static Sims2Installer installer;
class VulkanExample : public VulkanExampleBase {
    bool seenReady=false,autoTest=false;
    Sims2LauncherNavigation navigation;
    Sims2LauncherStartup startup;
    Sims2LauncherTransition transition;
    Sims2LauncherTransition previewTransition{Sims2LauncherSection::Play};
    Sims2SelectionMotion updateFocus;
    Sims2LauncherStick leftStick;
    Sims2LaunchFade playFade;
    bool blackFrameSubmitted=false;
    double heartbeat=0,feedbackUntil=0;
    Sims2InstallerCarousel carousel;
    Sims2InstallerMusic music;
    bool musicPlaying=false,strongFeedback=false;
    static constexpr const char *preferences="/app0/.sims2-launcher-prefs";
    void stopMusic() {if(musicPlaying){audio_stop();musicPlaying=false;}}
    void stopFeedback() {if(feedbackUntil>0){pad_vibrate(0,0);feedbackUntil=0;strongFeedback=false;}}
    void feedback(bool moved){
        if(moved && strongFeedback && now_seconds()<feedbackUntil)return;
        pad_vibrate(moved?.06f:.65f,moved?.12f:.90f);
        strongFeedback=!moved;feedbackUntil=now_seconds()+(moved?.040:.055);
    }
    void moveTab(int direction,bool ready,bool locked=false) {
        const bool moved=navigation.Move(direction,ready,now_seconds(),locked);
        // A rejected move nudges the selection and gives one sharp 55 ms pulse.
        feedback(moved);
        say("[SIMS2][INSTALLER][INFO] event=section_%s direction=%d section=%d",moved?"changed":"locked",direction,int(navigation.section));
    }
    void drawLauncherFrame(bool black=false){
        prepareFrame();auto cmd=drawCmdBuffers[currentBuffer];auto begin=vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd,&begin));beginDynamicRendering(cmd);
        if(!black)drawUI(cmd); // The final frame is an unconditional black clear.
        endDynamicRendering(cmd);VK_CHECK_RESULT(vkEndCommandBuffer(cmd));submitFrame();
    }
public:
    ~VulkanExample() {stopFeedback();stopMusic();}
    VulkanExample() {
        title="The Sims 2";apiVersion=VK_API_VERSION_1_3;useDynamicRendering=true;
        requiresStencil=true;deviceCreatepNextChain=&baseDynamicRenderingFeatures;
        ps5.ownOverlay=true;settings.overlay=true;defaultClearColor={{0,0,0,1}};
    }
    void prepare() override {
        if(Sims2IsUpdateHelper())startup.pending=false;
        else if(!installer.Start(true))throw std::runtime_error("Cannot start installation check");
        VulkanExampleBase::prepare();prepared=true;
        std::ifstream test("/app0/installer-autotest.txt",std::ios::binary);
        if(test){std::string marker((std::istreambuf_iterator<char>(test)),{});test.close();
            if(marker!="SIMS2-INSTALLER-AUTOTEST-1\n")throw std::runtime_error("Invalid installer test hook");
            if(std::remove("/app0/installer-autotest.txt"))throw std::runtime_error("Cannot consume installer test hook");autoTest=true;}
        say("[SIMS2][INSTALLER][INFO] build=%s event=launcher_ready width=%u height=%u",SIMS2_INSTALLER_BUILD_ID,width,height);
        bool musicEnabled=true;
        if(!Sims2LoadMusicPreference(preferences,musicEnabled))say("[SIMS2][AUDIO][WARN] event=music_preference_read_failed");
        music.SetEnabled(musicEnabled);
        if(music.Load("/app0/public-resources/launcher-music.mp3")) {
            musicPlaying=audio_start(Sims2InstallerMusic::Callback,&music);
            say("[SIMS2][AUDIO][%s] event=launcher_music frames=%zu source_rate=%d output_rate=48000",
                musicPlaying?"INFO":"WARN",music.Frames(),music.Rate());
        }else say("[SIMS2][AUDIO][WARN] event=launcher_music_load_failed");
        if(Sims2IsUpdateHelper()){
            navigation.section=Sims2LauncherSection::Update;Sims2StartApplyUpdate();
        }else{
            Sims2StartUpdateCheck(SIMS2_INSTALLER_VERSION);
        }
    }
    void render() override {
        if(!prepared)return;
        if(feedbackUntil>0 && now_seconds()>=feedbackUntil)stopFeedback();
        if(playFade.active){
            if(blackFrameSubmitted){stopFeedback();stopMusic();
                say("[SIMS2][INSTALLER][INFO] event=play_handoff black_frame_submitted=1");quit=true;return;}
            if(ps5.frameBudget && ps5.framesDrawn+2>=ps5.frameBudget)ps5.frameBudget+=30;
            const bool black=playFade.Black(now_seconds());drawLauncherFrame(black);blackFrameSubmitted=black;return;
        }
        if(installer.done.load(std::memory_order_acquire))installer.Join();
        auto phase=installer.phase.load();const auto pressed=ps5_pad().pressed;
        if(startup.pending){
            if(startup.Resolve(!installer.started)){
                navigation.section=Sims2LauncherStartup::Section(phase);
                transition=Sims2LauncherTransition(navigation.section);seenReady=phase==Sims2InstallPhase::Ready;
                say("[SIMS2][INSTALLER][INFO] event=initial_check_complete phase=%d section=%d",int(phase),int(navigation.section));
            }
            drawLauncherFrame();return;
        }
        const auto update=Sims2GetUpdateStatus();
        const bool updateBusy=Sims2UpdateBusy(update.state);
        if(update.state==Sims2UpdateState::ReadyToApply || update.state==Sims2UpdateState::RestartReady){
            stopFeedback();stopMusic();Sims2SetUpdateHandoff(update.state==Sims2UpdateState::ReadyToApply);
            say("[SIMS2][UPDATES][INFO] event=clean_handoff helper=%d",int(update.state==Sims2UpdateState::ReadyToApply));quit=true;return;
        }
        if(pressed&PAD_TOUCH_PAD){
            music.SetEnabled(!music.Enabled());
            const bool saved=Sims2SaveMusicPreference(preferences,music.Enabled());
            say("[SIMS2][AUDIO][%s] event=music_toggled enabled=%d saved=%d volume_percent=10",saved?"INFO":"WARN",int(music.Enabled()),int(saved));
        }
        const bool installingGame=installer.started;
        if(installingGame)navigation.section=Sims2LauncherSection::Install;
        if(navigation.section!=Sims2LauncherSection::Play && carousel.Move(ps5_pad().right_x,now_seconds())){
            previewTransition.Select(Sims2LauncherSection(carousel.selected),now_seconds());feedback(true);
            say("[SIMS2][INSTALLER][INFO] event=preview_changed image=%d",carousel.selected+1);
        }else if(carousel.Auto(installingGame,now_seconds())){
            previewTransition.Select(Sims2LauncherSection(carousel.selected),now_seconds());
            say("[SIMS2][INSTALLER][INFO] event=preview_auto image=%d",carousel.selected+1);
        }
        const bool ready=phase==Sims2InstallPhase::Ready;
        if(ready && !seenReady){seenReady=true;if(navigation.section==Sims2LauncherSection::Install)navigation.section=Sims2LauncherSection::Play;}
        const int stick=leftStick.Move(ps5_pad().left_x,ps5_pad().left_y,now_seconds());
        if((pressed&(PAD_L1|PAD_LEFT)) || stick==-1)moveTab(-1,ready,installingGame || updateBusy);
        else if((pressed&(PAD_R1|PAD_RIGHT)) || stick==1)moveTab(1,ready,installingGame || updateBusy);
        transition.Select(navigation.section,now_seconds());
        const bool canUpdateGame=installer.has_game.load() && installer.game_update_available.load() && !installer.restart_required;
        if(!installer.started && !canUpdateGame)updateFocus.Select(0,now_seconds());
        if(navigation.section==Sims2LauncherSection::Update && transition.Settled(now_seconds())){
            int direction=0;if((pressed&PAD_UP) || stick==-2)direction=-1;
            else if((pressed&PAD_DOWN) || stick==2)direction=1;
            if(direction && !installer.started && !updateBusy){
                const bool moved=updateFocus.Move(direction,canUpdateGame,now_seconds());feedback(moved);
                say("[SIMS2][UPDATES][INFO] event=focus_%s direction=%d choice=%d",moved?"changed":"locked",direction,updateFocus.selected);
            }
        }
        const bool activate=(pressed&PAD_CROSS) && !updateBusy && !Sims2IsUpdateHelper() && transition.Settled(now_seconds()) &&
            (navigation.section!=Sims2LauncherSection::Update || updateFocus.Settled(now_seconds()));
        bool keptIso=false;
        if(!installer.started){
            if(navigation.section==Sims2LauncherSection::Update && activate){
                if(updateFocus.selected==0)Sims2StartLauncherUpdate(SIMS2_INSTALLER_VERSION);
                else if(installer.has_game.load() && installer.game_update_available.load()){
                    if(!installer.Start(false))throw std::runtime_error("Cannot start game update; close and reopen the launcher");
                    say("[SIMS2][UPDATES][INFO] event=game_update_begin saves_preserved=1");
                }else{updateFocus.Move(1,false,now_seconds());feedback(false);}
            }else if(phase==Sims2InstallPhase::DeleteQuestion){
                if(autoTest || (pressed&PAD_CIRCLE))keptIso=installer.ResolveSource(false);
                else if(activate)installer.ResolveSource(true);
            }else if(phase==Sims2InstallPhase::Ready && (activate || autoTest)){
                if(navigation.section==Sims2LauncherSection::Install && !autoTest){navigation.section=Sims2LauncherSection::Play;feedback(true);}
                else{stopFeedback();Sims2SetNextExecutable(installer.Executable());music.SetEnabled(false);
                    playFade.Start(now_seconds());say("[SIMS2][INSTALLER][INFO] event=play_requested fade_ms=180");}
            }else if((phase==Sims2InstallPhase::Missing || phase==Sims2InstallPhase::Error || phase==Sims2InstallPhase::Cancelled) &&
                    (activate || (autoTest && phase==Sims2InstallPhase::Missing))){
                if(!installer.Start(false))throw std::runtime_error("Preparation cannot restart; close and reopen the application");
            }
            if(!playFade.active && !updateBusy && ((pressed&PAD_OPTIONS) || ((pressed&PAD_CIRCLE) && !keptIso))){stopFeedback();quit=true;return;}
        }else if(pressed&(PAD_CIRCLE|PAD_OPTIONS))installer.cancel.store(true);
        if(installer.started && now_seconds()-heartbeat>=2){heartbeat=now_seconds();
            say("[SIMS2][INSTALLER][INFO] build=%s event=heartbeat phase=%d completed=%u total=%u elapsed=%.3f",SIMS2_INSTALLER_BUILD_ID,
                int(phase),installer.completed.load(),installer.total.load(),now_seconds()-installer.began.load());}
        if(installer.started && (now_seconds()-installer.began.load()>1800 ||
                (phase==Sims2InstallPhase::Compiling && now_seconds()-installer.operation_began.load()>120))){
            say("[SIMS2][INSTALLER][ERROR] event=watchdog_timeout");vks::tools::exitFatal("Preparation timed out; preserve klog",1);}
        if(autoTest && (phase==Sims2InstallPhase::Error || phase==Sims2InstallPhase::Cancelled))
            vks::tools::exitFatal(installer.Error(),1);
        if(ps5.frameBudget && (installer.started || updateBusy) && ps5.framesDrawn+2>=ps5.frameBudget)ps5.frameBudget+=120;
        drawLauncherFrame();
    }
    void OnUpdateUIOverlay(vks::UIOverlay*) override {
        Sims2InstallerView view;
        view.initial_check=startup.pending;view.time=now_seconds();
        transition.Select(navigation.section,now_seconds());
        view.phase=installer.phase.load();view.section=transition.Displayed(now_seconds());
        view.selection_position=transition.Position(now_seconds());view.content_alpha=transition.Alpha(now_seconds());
        view.launch_alpha=playFade.Alpha(now_seconds());
        view.selection_offset=navigation.Offset(now_seconds());view.update=Sims2GetUpdateStatus();
        view.update_choice=updateFocus.selected;view.update_position=updateFocus.Position(now_seconds());
        view.update_offset=updateFocus.Offset(now_seconds());view.music_enabled=music.Enabled();
        view.has_game=installer.has_game.load();view.game_update_available=installer.game_update_available.load();
        view.game_launcher_version=installer.GameLauncherVersion();
        view.busy=installer.started;view.cancelling=installer.cancel.load();
        view.restart_required=installer.restart_required || Sims2IsUpdateHelper();
        view.completed=installer.completed.load();view.total=installer.total.load();
        view.extraction=installer.percent.load();view.preview=int(previewTransition.Displayed(now_seconds()));
        view.preview_position=previewTransition.Position(now_seconds());view.preview_alpha=previewTransition.Alpha(now_seconds());
        view.error=installer.Error();view.version=SIMS2_INSTALLER_VERSION;
        view.title_id=SIMS2_INSTALLER_TITLE_ID;
        Sims2DrawInstallerView(view);
    }
};
VULKAN_EXAMPLE_MAIN()
