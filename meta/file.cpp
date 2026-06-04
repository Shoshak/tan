#include <chrono>
#include <cstdint>
#include <filesystem>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace file {
static int l_size(lua_State *L) {
  std::filesystem::path path = luaL_checkstring(L, 1);
  uintmax_t size = std::filesystem::file_size(path);
  lua_pushinteger(L, size);
  return 1;
}

static int l_modified(lua_State *L) {
  std::filesystem::path path = luaL_checkstring(L, 1);
  auto wt = std::filesystem::last_write_time(path);
  auto epoch = wt.time_since_epoch().count();
  lua_pushinteger(L, epoch);
  return 1;
}

static int l_permissions(lua_State *L) {
  std::filesystem::path path = luaL_checkstring(L, 1);
  auto perms = std::filesystem::status(path).permissions();
  auto bits = static_cast<std::uintmax_t>(perms);
  lua_pushinteger(L, bits);
  return 1;
}

static const struct luaL_Reg file_meta[] = {
  {"size", l_size},
  {"modified", l_modified},
  {"permissions", l_permissions},
  {NULL, NULL}
};
}
