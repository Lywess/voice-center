#include "SdlPlayer.hpp"
#include "core/Logger.hpp"
#include <SDL.h>
#include <filesystem>

namespace vc {

SdlPlayer::SdlPlayer() {
  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    log_warn(std::string("SDL_Init failed: ") + SDL_GetError());
    initialized_ = false;
    return;
  }
  initialized_ = true;
}

SdlPlayer::~SdlPlayer() {
  if (initialized_) {
    SDL_Quit();
    initialized_ = false;
  }
}

bool SdlPlayer::play(const std::string& path) {
  if (!initialized_) {
    return false;
  }
  if (!std::filesystem::exists(path)) {
    return false;
  }

  SDL_AudioSpec wav_spec{};
  Uint8* buf = nullptr;
  Uint32 len = 0;
  if (!SDL_LoadWAV(path.c_str(), &wav_spec, &buf, &len)) {
    log_warn(std::string("SDL_LoadWAV failed: ") + SDL_GetError());
    return false;
  }

  SDL_AudioSpec have{};
  SDL_AudioDeviceID dev =
      SDL_OpenAudioDevice(nullptr, 0, &wav_spec, &have, 0);
  if (!dev) {
    log_warn(std::string("SDL_OpenAudioDevice failed: ") + SDL_GetError());
    SDL_FreeWAV(buf);
    return false;
  }

  if (SDL_QueueAudio(dev, buf, len) != 0) {
    log_warn(std::string("SDL_QueueAudio failed: ") + SDL_GetError());
    SDL_CloseAudioDevice(dev);
    SDL_FreeWAV(buf);
    return false;
  }

  SDL_PauseAudioDevice(dev, 0);
  while (SDL_GetQueuedAudioSize(dev) > 0) {
    SDL_Delay(10);
  }

  SDL_CloseAudioDevice(dev);
  SDL_FreeWAV(buf);
  return true;
}

}  // namespace vc
