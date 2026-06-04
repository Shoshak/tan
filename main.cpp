extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "action.cpp"
#include "lib.cpp"

#ifdef ENABLE_VIDEO
#include "meta/video.cpp"
#endif
#include "meta/file.cpp"

int main() {
  std::filesystem::path config_path;
  const char *xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  if (!xdg_config_home) {
    const char *home_dir = std::getenv("HOME");
    if (!home_dir) {
      throw std::runtime_error("No config or home directory found.");
    }
    config_path = std::filesystem::path(home_dir) / ".config";
  } else {
    config_path = xdg_config_home;
  }

  std::filesystem::path lua_init = config_path / "tan" / "init.lua";
  if (!std::filesystem::exists(lua_init)) {
    std::cerr << "init.lua does not exist.\n";
    return 1;
  }

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  luaL_newlib(L, tan::lib);
  lua_setglobal(L, "tan");

  luaL_newlib(L, action::lib);
  lua_setglobal(L, "action");

#ifdef ENABLE_VIDEO
  luaL_newlib(L, video::video_meta);
  lua_setglobal(L, "video");
#endif

  luaL_newlib(L, file::file_meta);
  lua_setglobal(L, "file");

  tan::fileWatcher->watch();

  if (luaL_dofile(L, lua_init.c_str()) != LUA_OK) {
    std::cerr << lua_tostring(L, -1) << "\n";
    lua_pop(L, 1);
    goto cleanup;
  }

cleanup:
  delete tan::fileWatcher;
  lua_close(L);
}
