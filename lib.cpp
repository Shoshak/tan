#include <chrono>
#include <condition_variable>
#include <efsw/efsw.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <sys/file.h>
#include <unistd.h>
#endif

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace tan {
static int l_get(lua_State *L) {
  std::string path = luaL_checkstring(L, 1);

  std::vector<std::filesystem::path> paths;
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      paths.push_back(entry.path());
    }
  }

  lua_createtable(L, paths.size(), 0);
  for (int i = 0; i < paths.size(); i++) {
    lua_pushstring(L, paths[i].c_str());
    lua_rawseti(L, -2, i + 1);
  }

  return 1;
}

static int l_getrec(lua_State *L) {
  std::string path = luaL_checkstring(L, 1);

  std::vector<std::filesystem::path> paths;
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(path)) {
    if (entry.is_regular_file()) {
      paths.push_back(entry.path());
    }
  }

  lua_createtable(L, paths.size(), 0);
  for (int i = 0; i < paths.size(); i++) {
    lua_pushstring(L, paths[i].c_str());
    lua_rawseti(L, -2, i + 1);
  }

  return 1;
}

efsw::FileWatcher *fileWatcher = new efsw::FileWatcher();

class UpdateListener : public efsw::FileWatchListener {
private:
  std::unordered_map<std::filesystem::path,
                     std::chrono::steady_clock::time_point>
      debouncer;
  std::queue<std::pair<efsw::WatchID, std::filesystem::path>> updates;
  std::mutex mx;
  std::condition_variable cv;

public:
  efsw::WatchID addEntry(std::string target) {
    std::lock_guard<std::mutex> lock(mx);
    std::string original_target = target;

    bool recursive =
        target.ends_with(std::filesystem::path::preferred_separator);
    if (recursive) {
      target.pop_back();
    }

    efsw::WatchID id = fileWatcher->addWatch(target, this, recursive);
    return id;
  }

  void removeEntry(efsw::WatchID id) {
    std::lock_guard<std::mutex> lock(mx);
    fileWatcher->removeWatch(id);
  }

  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        const std::string &oldFilename) override {
    if (action == efsw::Actions::Add) {
      std::filesystem::path p = std::filesystem::path(dir) / filename;
      if (debouncer.contains(p)) {
        auto last = debouncer[p];
        auto difference = std::chrono::steady_clock::now() - last;
        auto milis =
            std::chrono::duration_cast<std::chrono::milliseconds>(difference)
                .count();
        if (milis < 100) {
          return;
        } else {
          debouncer.erase(p);
        }
      }
      debouncer[p] = std::chrono::steady_clock::now();
#ifndef _WIN32
      int fd = open(p.c_str(), O_WRONLY);
      while (true) {
        if (flock(fd, LOCK_EX) == -1) {
          if (errno != EWOULDBLOCK) {
            std::cerr << "flock failed\n" << errno << "\n";
            return;
          }
        }
        break;
      }
      close(fd);
#else
      while (true) {
        std::ifstream file(f);
        if (file.is_open()) {
          break;
        } else {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }
#endif
      {
        std::lock_guard<std::mutex> lock(mx);
        updates.push(std::make_pair(watchid, p));
      }
      cv.notify_one();
    }
  }

  std::pair<efsw::WatchID, std::filesystem::path> listen() {
    std::unique_lock<std::mutex> lock(mx);
    cv.wait(lock, [this]() { return !updates.empty(); });
    auto p = updates.front();
    updates.pop();

    return std::make_pair(p.first, p.second);
  }
};

UpdateListener *listener = new UpdateListener();

static int l_register(lua_State *L) {
  std::string path = luaL_checkstring(L, 1);
  int id = listener->addEntry(path);
  lua_pushinteger(L, id);
  return 1;
}

static int l_unregister(lua_State *L) {
  int id = luaL_checkinteger(L, 1);
  listener->removeEntry(id);
  return 0;
}

static int l_listen(lua_State *L) {
  auto p = listener->listen();

  lua_pushinteger(L, p.first);
  lua_pushstring(L, p.second.c_str());
  return 2;
}

static const struct luaL_Reg lib[] = {
  {"get", l_get},
  {"getrec", l_getrec},
  {"listen", l_listen},
  {"register", l_register},
  {"unregister", l_unregister},
  {NULL, NULL}
};
} // namespace tan
