#include "Application.h"
#include "Defs.h"
#include "Log.h"
#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "Textures.h"
#include "Audio.h"
#include "Scene.h"
#include "Map.h"
#include "FadeToBlack.h"
#include "Collisions.h"
#include "TimeManager.h"
#include "Optick/include/optick.h"

Application::Application(int argc, char* args[]) : argc(argc), args(args)
{
	want_to_save = want_to_load = false;

	// Independent Managers
	time = new TimeManager();
	tex = new Textures();

	// Modules
	AddModule(input = new Input());
	AddModule(win = new Window());
	AddModule(audio = new Audio());
	AddModule(map = new Map());
	AddModule(scene = new Scene());
	AddModule(collisions = new Collisions());
	AddModule(fade = new FadeToBlack());
	AddModule(render = new Render()); // render last to swap buffer
}

Application::~Application()
{
	for (std::list<Module*>::reverse_iterator it = modules.rbegin(); it != modules.rend(); ++it)
		DEL(*it);

	modules.clear();

	DEL(time);
	DEL(tex);
}

void Application::AddModule(Module* module)
{
	module->Init();
	modules.push_back(module);
}

// Called before the first frame
bool Application::Init()
{
	bool ret = true;

	pugi::xml_document config_file;
	pugi::xml_parse_result result = config_file.load_file("config.xml");

	if (result)
	{
		pugi::xml_node config = config_file.child("config");

		if (!config.empty())
		{
			pugi::xml_node app_config = config.child("app");

			title = app_config.child("title").child_value();
			organization = app_config.child("organization").child_value();

			for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && ret; ++it)
				ret = (*it)->Awake(config.child((*it)->name));
		}
		else
		{
			LOG("Empty config file.");

			title = "unknown";
			organization = "unknown";

			for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && ret; ++it)
				ret = (*it)->Awake(config.append_child((*it)->name));
		}

		// Setup module inter-dependecies
		for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && ret; ++it)
			ret = (*it)->Start();

		// Run test content
		map->Load("level1.tmx");
		audio->PlayMusic("audio/music/lvl1bgm.ogg");
	}
	else
	{
		LOG("Could not load map xml file config.xml. pugi error: %s", result.description());
	}

	return ret;
}

// Called each loop iteration
int Application::Update()
{
	int ret = 1;

	OPTICK_CATEGORY("Update Application", Optick::Category::GameLogic);

	PrepareUpdate();

	if (input->GetWindowEvent(WE_QUIT))
	{
		LOG("WINDOW EVENT TRIGGERED SHUTDOWN");
		ret = 0; // closing app
	}
	else
	{
		static std::list<Module*>::iterator it;
		static bool no_error = true;

		OPTICK_CATEGORY("PreUpdate Application", Optick::Category::GameLogic);
		for (it = modules.begin(); it != modules.end() && no_error; ++it)
			no_error = (*it)->PreUpdate();

		OPTICK_CATEGORY("Update Application", Optick::Category::GameLogic);
		for (it = modules.begin(); it != modules.end() && no_error; ++it)
			no_error = (*it)->Update();

		OPTICK_CATEGORY("PostUpdate Application", Optick::Category::GameLogic);
		for (it = modules.begin(); it != modules.end() && no_error; ++it)
			no_error = (*it)->PostUpdate();

		if (no_error)
			FinishUpdate();
		else
			ret = -1; // error
	}

	return ret;
}

void Application::PrepareUpdate()
{
	time->UpdateDeltaTime();
}

void Application::FinishUpdate()
{
	if(want_to_save == true)
		SavegameNow();

	if(want_to_load == true)
		LoadGameNow();

	// Delay capped FPS
	unsigned int extra_ms = time->ManageFrameTimers();
	if (extra_ms > 0)
		time->Delay(extra_ms);
}

// Called before quitting
bool Application::CleanUp()
{
	bool ret = true;

	for (std::list<Module*>::reverse_iterator it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->CleanUp();

	tex->CleanUp();

	return ret;
}

// ---------------------------------------
int Application::GetArgc() const
{
	return argc;
}

// ---------------------------------------
const char* Application::GetArgv(int index) const
{
	return index < argc ? args[index] : nullptr;
}

// ---------------------------------------
const char* Application::GetTitle() const
{
	return title;
}

// ---------------------------------------
const char* Application::GetOrganization() const
{
	return organization;
}

// Load / Save
void Application::LoadGame(const char* file)
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list
	want_to_load = true;
	//load_game.create("%s%s", fs->GetSaveDirectory(), file);
}

// ---------------------------------------
void Application::SaveGame(const char* file) const
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list ... should we overwrite ?

	want_to_save = true;
	//save_game.create(file);
}

// ---------------------------------------
void Application::GetSaveGames(std::list<std::string>& list_to_fill) const
{
	// need to add functionality to file_system module for this to work
}

bool Application::LoadGameNow()
{
	bool ret = true;

	pugi::xml_document data;
	pugi::xml_node root;

	pugi::xml_parse_result result = data.load_file(load_game.c_str());

	if(result)
	{
		LOG("Loading new Game State from %s...", load_game);

		root = data.child("game_state");

		std::list<Module*>::iterator it;
		for (it = modules.begin(); it != modules.end() && ret; ++it)
			ret = (*it)->Load(root.child((*it)->name));

		want_to_load = false;
		data.reset();

		if(ret) LOG("...finished loading");
		else LOG("...loading process interrupted with error on module %s", (*it)->name);
	}
	else
	{
		LOG("Could not parse game state xml file %s. pugi error: %s", load_game, result.description());
		ret = false;
	}

	return ret;
}

bool Application::SavegameNow() const
{
	bool ret = true;

	LOG("Saving Game State to %s...", save_game);

	// xml object were we will store all data
	pugi::xml_document data;
	pugi::xml_node root;
	
	root = data.append_child("game_state");

	std::list<Module*>::const_iterator it;
	for (it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->Save(root.append_child((*it)->name));

	if (ret)
	{
		data.save_file(save_game.c_str());
		data.reset();
		want_to_save = false;

		LOG("... finished saving", save_game);
	}
	else
		LOG("Save process halted from an error in module %s", (*it)->name);

	return ret;
}