#include <iostream>
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <libavformat/avformat.h>
}

namespace video {
static int l_duration(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);

  AVFormatContext* pFormatCtx = NULL;
  if (avformat_open_input(&pFormatCtx, path, NULL, NULL) < 0) {
    std::cerr << "Avformat open input error\n";
    return 0;
  }

  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    std::cerr << "Avformat find stream info error\n";
    goto cleanup;
  }

  if (pFormatCtx->duration != AV_NOPTS_VALUE) {
    double total_seconds = (double) pFormatCtx->duration / AV_TIME_BASE;
    lua_pushnumber(L, total_seconds);
    return 1;
  } else {
    std::cout << "Unknown duration for: " << path << "\n";
    goto cleanup;
  }

  cleanup:
  avformat_close_input(&pFormatCtx);
  return 0;
}

static const struct luaL_Reg video_meta[] = {
  {"duration", l_duration},
  {NULL, NULL}
};
}
