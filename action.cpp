#include <filesystem>
#include <iostream>
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace action {
static int l_copy(lua_State *L) {
  std::filesystem::path src = luaL_checkstring(L, 1);
  std::filesystem::path dest = luaL_checkstring(L, 2);

  if (std::filesystem::is_directory(dest)) {
    dest /= src.filename();
  }
  try {
    bool success = std::filesystem::copy_file(
        src, dest, std::filesystem::copy_options::overwrite_existing);
    lua_pushboolean(L, success);
    return 1;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Failed to copy " << src << " to " << dest << ": " << e.what()
              << "\n";
    lua_pushboolean(L, false);
    return 1;
  }
}

static int l_move(lua_State *L) {
  std::filesystem::path src = luaL_checkstring(L, 1);
  std::filesystem::path dest = luaL_checkstring(L, 2);

  if (std::filesystem::is_directory(dest)) {
    dest /= src.filename();
  }
  try {
    bool success = std::filesystem::copy_file(
        src, dest, std::filesystem::copy_options::overwrite_existing);
    success = std::filesystem::remove(src);
    lua_pushboolean(L, success);
    return 1;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Failed to move " << src << " to " << dest << ": " << e.what()
              << "\n";
    lua_pushboolean(L, false);
    return 1;
  }
}

static int l_delete(lua_State *L) {
  std::filesystem::path f = luaL_checkstring(L, 1);
  try {
    bool success = std::filesystem::remove(f);
    lua_pushboolean(L, success);
    return 1;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Failed to delete " << f << ": " << e.what() << "\n";
    lua_pushboolean(L, false);
    return 1;
  }
}

static const struct luaL_Reg lib[] = {
    {"copy", l_copy}, {"move", l_move}, {"delete", l_delete}, {NULL, NULL}};
} // namespace action
