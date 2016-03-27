#include "nes_interface.hpp"
#include "drivers/sdl/config.h"
#include "drivers/sdl/sdl.h"
#include "driver.h"
#include "fceu.h"
#include "cheat.h"
#include "video.h"
#include <stdio.h>
#include <SDL/SDL.h>

// Global configuration info.
extern Config *g_config;
extern int noGui;
extern uint8_t *XBuf;

namespace nes {

void NESInterface::getRGB(
    unsigned char pixel,
    unsigned char &red,
    unsigned char &green,
    unsigned char &blue
) {

    static const int ntsc_tbl[] = {
        0x000000, 0, 0x4a4a4a, 0, 0x6f6f6f, 0, 0x8e8e8e, 0,
        0xaaaaaa, 0, 0xc0c0c0, 0, 0xd6d6d6, 0, 0xececec, 0,
        0x484800, 0, 0x69690f, 0, 0x86861d, 0, 0xa2a22a, 0,
        0xbbbb35, 0, 0xd2d240, 0, 0xe8e84a, 0, 0xfcfc54, 0,
        0x7c2c00, 0, 0x904811, 0, 0xa26221, 0, 0xb47a30, 0,
        0xc3903d, 0, 0xd2a44a, 0, 0xdfb755, 0, 0xecc860, 0,
        0x901c00, 0, 0xa33915, 0, 0xb55328, 0, 0xc66c3a, 0,
        0xd5824a, 0, 0xe39759, 0, 0xf0aa67, 0, 0xfcbc74, 0,
        0x940000, 0, 0xa71a1a, 0, 0xb83232, 0, 0xc84848, 0,
        0xd65c5c, 0, 0xe46f6f, 0, 0xf08080, 0, 0xfc9090, 0,
        0x840064, 0, 0x97197a, 0, 0xa8308f, 0, 0xb846a2, 0,
        0xc659b3, 0, 0xd46cc3, 0, 0xe07cd2, 0, 0xec8ce0, 0,
        0x500084, 0, 0x68199a, 0, 0x7d30ad, 0, 0x9246c0, 0,
        0xa459d0, 0, 0xb56ce0, 0, 0xc57cee, 0, 0xd48cfc, 0,
        0x140090, 0, 0x331aa3, 0, 0x4e32b5, 0, 0x6848c6, 0,
        0x7f5cd5, 0, 0x956fe3, 0, 0xa980f0, 0, 0xbc90fc, 0,
        0x000094, 0, 0x181aa7, 0, 0x2d32b8, 0, 0x4248c8, 0,
        0x545cd6, 0, 0x656fe4, 0, 0x7580f0, 0, 0x8490fc, 0,
        0x001c88, 0, 0x183b9d, 0, 0x2d57b0, 0, 0x4272c2, 0,
        0x548ad2, 0, 0x65a0e1, 0, 0x75b5ef, 0, 0x84c8fc, 0,
        0x003064, 0, 0x185080, 0, 0x2d6d98, 0, 0x4288b0, 0,
        0x54a0c5, 0, 0x65b7d9, 0, 0x75cceb, 0, 0x84e0fc, 0,
        0x004030, 0, 0x18624e, 0, 0x2d8169, 0, 0x429e82, 0,
        0x54b899, 0, 0x65d1ae, 0, 0x75e7c2, 0, 0x84fcd4, 0,
        0x004400, 0, 0x1a661a, 0, 0x328432, 0, 0x48a048, 0,
        0x5cba5c, 0, 0x6fd26f, 0, 0x80e880, 0, 0x90fc90, 0,
        0x143c00, 0, 0x355f18, 0, 0x527e2d, 0, 0x6e9c42, 0,
        0x87b754, 0, 0x9ed065, 0, 0xb4e775, 0, 0xc8fc84, 0,
        0x303800, 0, 0x505916, 0, 0x6d762b, 0, 0x88923e, 0,
        0xa0ab4f, 0, 0xb7c25f, 0, 0xccd86e, 0, 0xe0ec7c, 0,
        0x482c00, 0, 0x694d14, 0, 0x866a26, 0, 0xa28638, 0,
        0xbb9f47, 0, 0xd2b656, 0, 0xe8cc63, 0, 0xfce070, 0
    };

    int c = ntsc_tbl[pixel];
    red   = c >> 16;
    green = (c >> 8) & 0xFF;
    blue  = c & 0xFF;
}


class NESInterface::Impl {

    public:

        // create an NESInterface. This routine is not threadsafe!
        Impl(const std::string &rom_file);
        ~Impl();

        // Resets the game
        void reset_game();

        // Indicates if the game has ended
        bool game_over();

        // Applies an action to the game and returns the reward. It is the user's responsibility
        // to check if the game has ended and reset when necessary - this method will keep pressing
        // buttons on the game over screen.
        reward_t act(Action action);

        // Returns the number of legal actions.
        int getNumLegalActions();

        // Returns the vector of legal actions.
        ActionVect getLegalActionSet();

        // Minimum possible instantaneous reward.
        reward_t minReward() const;

        // Maximum possible instantaneous reward.
        reward_t maxReward() const;

        // The remaining number of lives.
        int lives() const;

        // Returns the frame number since the loading of the ROM
        int getFrameNumber() const;

        // Returns the frame number since the start of the current episode
        int getEpisodeFrameNumber() const;

        // Sets the episodic frame limit
        void setMaxNumFrames(int newMax);

        // Returns the current game screen
        const uint8_t *getScreen() const;

        // Return screen height.
        const int getScreenHeight() const;

        // Return screen width.
        const int getScreenWidth() const;

        // Saves the state of the system
        void saveState();

        // Loads the state of the system
        bool loadState();

        // Gets current state as string
        std::string getSnapshot() const;

        // restores state from a string
        void restoreSnapshot(const std::string& snapshot);

    private:

        reward_t m_episode_score; // Score accumulated throughout the course of an episode
        bool m_display_active;    // Should the screen be displayed or not
        int m_max_num_frames;     // Maximum number of frames for each episode
        int nes_input; // Input to the emulator.
        int current_game_score;
        int remaining_lives;
        int game_state;
        int episode_frame_number;
};


NESInterface::Impl::~Impl() {

	CloseGame();
	FCEUI_Kill();
	SDL_Quit();
}

bool NESInterface::Impl::loadState() {

	// TODO implement.
	printf("NOT IMPLEMENTED.\n");

    // FCEUD_LoadStateFrom ();
	return false;
}


bool NESInterface::Impl::game_over() {

	// Update game state.
	game_state = FCEU_CheatGetByte(0x0770);

	if (game_state == 1) return false;
	return true;
}


void NESInterface::Impl::reset_game() {

	// Pretty simple...
	ResetNES();

	// Initialize the score and frame counter.
	current_game_score = 0;
	episode_frame_number = 0;

	// Run a few frames first to get to the startup screen.
	for (int i = 0; i<60; i++) {
		NESInterface::Impl::act((Action) NOOP);
	}

	// Hit the start button...
	for (int i = 0; i<10; i++) {
		NESInterface::Impl::act((Action) SELECT);
	}
}


void NESInterface::Impl::saveState() {

	// TODO implement
	printf("NOT IMPLEMENTED.\n");
	//		FCEUD_SaveStateAs ();
}

std::string NESInterface::Impl::getSnapshot() const {

	// TODO implement
	printf("NOT IMPLEMENTED.\n");
	/*
    const ALEState* state = m_emu->environment->cloneState();
    std::string snapshot = state->getStateAsString();
    m_emu->environment->destroyState(state);
    return snapshot;
    */
	return "";
}

void NESInterface::Impl::restoreSnapshot(const std::string &snapshot) {

	// TODO implement
	printf("NOT IMPLEMENTED.\n");
    //ALEState state(snapshot);
    //m_emu->environment->restoreState(state);
}

const uint8_t *NESInterface::Impl::getScreen() const {

	// Look at GetScreenPixel in video.cpp?
	return XBuf;
}

const int NESInterface::Impl::getScreenHeight() const {
	return FSettings.LastSLine - FSettings.FirstSLine+1;
}

const int NESInterface::Impl::getScreenWidth() const {
	return NES_SCREEN_WIDTH;
}


void NESInterface::Impl::setMaxNumFrames(int newMax) {
    m_max_num_frames = newMax;
}


int NESInterface::Impl::getEpisodeFrameNumber() const {
	return episode_frame_number;
}


int NESInterface::Impl::getFrameNumber() const {

	// TODO implement
	printf("NOT IMPLEMENTED.\n");
    //return m_emu->environment->getFrameNumber();
	return 0;
}

int NESInterface::Impl::getNumLegalActions() {
	return NUM_NES_LEGAL_ACTIONS;
}

ActionVect NESInterface::Impl::getLegalActionSet() {
    
	// Since we're only working with Super Mario Bros. here...
	ActionVect legal_actions;

	// All the things you can do in SMB...
	legal_actions.push_back((Action) NOOP);
	legal_actions.push_back((Action) A);
	legal_actions.push_back((Action) B);
	legal_actions.push_back((Action) UP);
	legal_actions.push_back((Action) RIGHT);
	legal_actions.push_back((Action) LEFT);
	legal_actions.push_back((Action) DOWN);
	legal_actions.push_back((Action) A_UP);
	legal_actions.push_back((Action) A_RIGHT);
	legal_actions.push_back((Action) A_LEFT);
	legal_actions.push_back((Action) A_DOWN);
	legal_actions.push_back((Action) B_UP);
	legal_actions.push_back((Action) B_RIGHT);
	legal_actions.push_back((Action) B_LEFT);
	legal_actions.push_back((Action) B_UP);
	legal_actions.push_back((Action) B_DOWN);

	return legal_actions;
}


reward_t NESInterface::Impl::minReward() const {

	// TODO implement.
	printf("NOT IMPLEMENTED.\n");
    //return m_rom_settings->minReward();
	return 0;
}


reward_t NESInterface::Impl::maxReward() const {

	// TODO implement.
	printf("NOT IMPLEMENTED.\n");
    //return m_rom_settings->maxReward();
	return 0;
}


int NESInterface::Impl::lives() const {
	return remaining_lives;
}


reward_t NESInterface::Impl::act(Action action) {

	// Calculate lives.
	remaining_lives = FCEU_CheatGetByte(0x075a);

	// Update game state.
	game_state = FCEU_CheatGetByte(0x0770);

	// Set the action. No idea whether this will work with other input configurations!
	switch (action) {

		case (Action) NOOP:
			nes_input = 0;
			break;

		case (Action) A:
			nes_input = 1;
			break;

		case (Action) B:
			nes_input = 2;
			break;

		case (Action) UP:
			nes_input = 16;
			break;

		case (Action) RIGHT:
			nes_input = 128;
			break;

		case (Action) LEFT:
			nes_input = 64;
			break;

		case (Action) DOWN:
			nes_input = 32;
			break;

		case (Action) A_UP:
			nes_input = 17;
			break;

		case (Action) A_RIGHT:
			nes_input = 129;
			break;

		case (Action) A_LEFT:
			nes_input = 65;
			break;

		case (Action) A_DOWN:
			nes_input = 33;
			break;

		case (Action) B_UP:
			nes_input = 18;
			break;

		case (Action) B_RIGHT:
			nes_input = 130;
			break;

		case (Action) B_LEFT:
			nes_input = 66;
			break;

		case (Action) B_DOWN:
			nes_input = 34;
			break;

		case (Action) SELECT:
			nes_input = 8;
			break;

		default:
			nes_input = 0;
			printf("ERROR: Undefined action sent to nes_act!");
			break;
	}

	uint8 *gfx;
	int32 *sound;
	int32 ssize;
	static int fskipc = 0;

	// Main loop.
	episode_frame_number++;
	FCEUI_Emulate(&gfx, &sound, &ssize, fskipc);
	FCEUD_Update(gfx, sound, ssize);

	// Get score...
	int new_score = (FCEU_CheatGetByte(0x07d7) * 100000) +
			(FCEU_CheatGetByte(0x07d8) * 100000) +
			(FCEU_CheatGetByte(0x07d9) * 10000) +
			(FCEU_CheatGetByte(0x07da) * 1000) +
			(FCEU_CheatGetByte(0x07db) * 100) +
			(FCEU_CheatGetByte(0x07dc) * 10);

	// Calculate the reward.
	reward_t reward = new_score - current_game_score;
	current_game_score = new_score;

	/*
	while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
	*/

	return reward;
}

NESInterface::Impl::Impl(const std::string &rom_file) :
    m_episode_score(0),
    m_display_active(false),
	nes_input(0),
	current_game_score(0),
	remaining_lives(0),
	game_state(0),
	episode_frame_number(0)
{

	// Initialize some configuration variables.
	static int inited = 0;
	noGui = 1;

	if(SDL_Init(SDL_INIT_VIDEO)) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
	}
	SDL_GL_LoadLibrary(0);

	// Initialize the configuration system
	g_config = InitConfig();

	if(!g_config) {
		SDL_Quit();
		printf("Could not initialize configuration.\n");
	}

	int error = FCEUI_Initialize();
	if(error != 1) {
		SDL_Quit();
		printf("Error initializing FCEUI.\n");
	}

	// Specify some arguments - may want to do this dynamically at some point...
	int argc = 2;
	char** argv = new char*[argc];
        for (int i=0; i < argc; i++) {
                argv[i] = new char[200+rom_file.length()];
        }
        strcpy(argv[0], "./fceux");
        strcpy(argv[1], rom_file.c_str());

    // Parse the args.
	int romIndex = g_config->parse(argc, argv);
	if(romIndex < 1) {
		printf("ERROR: romIndex less than 1.\n");
	}

	// Now set some configuration options.
	int ret = g_config->setOption("SDL.NoConfig", 1);
	if (ret != 0) {
		printf("Error setting --no-config option.");
	}

	// Update input devices.
	UpdateInput(g_config);

    // Update the emu core with the config options.
	UpdateEMUCore(g_config);

	/*
	// Set up the GUI.
	gtk_init(&argc, &argv);
	InitGTKSubsystem(argc, argv);
	while(gtk_events_pending())
		gtk_main_iteration_do(FALSE);
		*/

	// load the specified game.
	error = LoadGame(argv[romIndex]);

	// If something went wrong, kill the driver stuff.
	if(error != 1) {
		if(inited&2)
			KillJoysticks();
		if(inited&4)
			KillVideo();
		if(inited&1)
			KillSound();
		inited=0;
		printf("Error loading ROM.\n");
		SDL_Quit();
	}

	// Set the emulator to read from nes_input instead of the Gamepad :)
	FCEUI_SetInput(0, (ESI) SI_GAMEPAD, &nes_input, 0);
	FCEUI_SetInput(1, (ESI) SI_GAMEPAD, &nes_input, 0);
}

/* --------------------------------------------------------------------------------------------------*/

/* begin PIMPL wrapper */

bool NESInterface::loadState() {
    return m_pimpl->loadState();
}


bool NESInterface::gameOver() {
    return m_pimpl->game_over();
}


void NESInterface::resetGame() {
    m_pimpl->reset_game();
}


void NESInterface::saveState() {
    m_pimpl->saveState();
}


std::string NESInterface::getSnapshot() const {
    return m_pimpl->getSnapshot();
}


void NESInterface::restoreSnapshot(const std::string& snapshot) {
    m_pimpl->restoreSnapshot(snapshot);
}

const uint8_t *NESInterface::getScreen() const {
    return m_pimpl->getScreen();
}

const int NESInterface::getScreenHeight() const {
	return m_pimpl->getScreenHeight();
}

const int NESInterface::getScreenWidth() const {
	return m_pimpl->getScreenWidth();
}


void NESInterface::setMaxNumFrames(int newMax) {
    m_pimpl->setMaxNumFrames(newMax);
}


int NESInterface::getEpisodeFrameNumber() const {
    return m_pimpl->getEpisodeFrameNumber();
}


int NESInterface::getFrameNumber() const {
    return m_pimpl->getFrameNumber();
}

int NESInterface::getNumLegalActions() {
	return m_pimpl->getNumLegalActions();
}

ActionVect NESInterface::getLegalActionSet() {
    return m_pimpl->getLegalActionSet();
}


reward_t NESInterface::minReward() const {
    return m_pimpl->minReward();
}


reward_t NESInterface::maxReward() const {
    return m_pimpl->maxReward();
}


int NESInterface::lives() const {
    return m_pimpl->lives();
}


reward_t NESInterface::act(Action action) {
    return m_pimpl->act(action);
}

NESInterface::NESInterface(const std::string &rom_file) :
    m_pimpl(new NESInterface::Impl(rom_file)) {

}

NESInterface::~NESInterface() {
    delete m_pimpl;
}

} // namespace nes

