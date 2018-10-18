#pragma once

#include "../audio/AudioBuffer.h"
#include "Component.h"
#include "../engine/Resource.h"

namespace L {
  class ScriptComponent;
  class Transform;
  class AudioSourceComponent : public Component {
    L_COMPONENT(AudioSourceComponent)
      L_COMPONENT_HAS_AUDIO_RENDER(AudioSourceComponent)
  protected:
    Transform* _transform;
    ScriptComponent* _script;
    Resource<AudioStream> _stream;
    float _volume;
    uint32_t _current_frame;
    bool _playing, _looping;
  public:
    inline AudioSourceComponent() : _volume(1.f), _playing(false), _looping(false) {}
    void audio_render(void* frames, uint32_t frame_count);

    void update_components() override;
    virtual Map<Symbol, Var> pack() const override;
    virtual void unpack(const Map<Symbol, Var>&) override;
    static void script_registration();

    inline void stream(const char* filepath) { _stream = filepath; }
    inline void looping(bool should_loop) { _looping = should_loop; }
    inline void volume(float v) { _volume = v; }
    inline void play() { _current_frame = 0; _playing = true; }
    inline void stop() { _playing = false; }
    inline bool playing() const { return _playing; }
  };
}
