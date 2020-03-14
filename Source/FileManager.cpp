#include "FileManager.h"
#include "Defs.h"
#include "Log.h"
#include "SDL/include/SDL.h"

#include "PhysFS/include/physfs.h"

#pragma comment( lib, "PhysFS/libx86/physfs.lib" )

FileManager::FileManager()
{}

FileManager::~FileManager()
{}

bool FileManager::Init()
{
	char* path = SDL_GetBasePath();

	bool ret = (PHYSFS_init(path) != 0);

	if (ret)
	{
		base_path = path;
		ret = AddDirectory(path, NULL);
	}
	else
		LOG("Error Initializing PhysFS: %s", PHYSFS_getLastError());

	SDL_free(path);

	return ret;
}

bool FileManager::CleanUp()
{
	return (PHYSFS_deinit() != 0);
}

bool FileManager::AddDirectory(const char* path, const char* mount_point)
{
	bool ret = (PHYSFS_mount(path, mount_point, 1) != 0);

	if (!ret)
		LOG("PhysFS error while adding a path (%s): %s\n", path, PHYSFS_getLastError());

	return ret;
}

bool FileManager::LoadConfig(pugi::xml_node& node)
{
	bool ret = LoadXML("config.xml", config);

	if (ret)
		node = config.first_child();

	return ret;
}

bool FileManager::LoadXML(const char* file, pugi::xml_document& doc)
{
	bool ret = false;

	char* buffer;
	unsigned int size = Load(file, &buffer);

	if (size > 0u)
	{
		pugi::xml_parse_result result = doc.load_buffer(buffer, size);

		if (!(ret = result))
			LOG("Could not load config.xml - pugi error: %s", result.description());
	}

	return ret;

	/*const char* org = "Juicy Code Games";
	const char* app = "Square Up";
	pref_dir = SDL_GetPrefPath(org, app);*/
}

const char* FileManager::GetBasePath()
{
	return base_path.c_str();
}

unsigned int FileManager::Load(const char* file, char** buffer) const
{
	unsigned int ret = 0;

	PHYSFS_file* fs_file = PHYSFS_openRead(file);

	if (fs_file != NULL)
	{
		PHYSFS_sint64 size = PHYSFS_fileLength(fs_file);

		if (size > 0)
		{
			*buffer = new char[(unsigned int)size];
			PHYSFS_sint64 read = PHYSFS_read(fs_file, *buffer, 1, (PHYSFS_sint32)size);
			if (read != size)
			{
				LOG("File System error while reading from file %s: %s\n", file, PHYSFS_getLastError());
				DEL_ARRAY(buffer);
			}
			else
				ret = (unsigned int)read;
		}

		if (PHYSFS_close(fs_file) == 0)
			LOG("File System error while closing file %s: %s\n", file, PHYSFS_getLastError());
	}
	else
		LOG("File System error while opening file %s: %s\n", file, PHYSFS_getLastError());

	return ret;
}

SDL_RWops* FileManager::Load(const char* file) const
{
	char* buffer;
	int size = Load(file, &buffer);

	if (size > 0)
	{
		SDL_RWops* r = SDL_RWFromConstMem(buffer, size);
		if (r != NULL)
			r->close = close_sdl_rwops;

		return r;
	}
	else
		return NULL;
}

int close_sdl_rwops(SDL_RWops* rw)
{
	DEL_ARRAY(rw->hidden.mem.base);
	SDL_FreeRW(rw);
	return 0;
}