#pragma once
#include "Engine.h"
#include <atomic>
#include <memory>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#endif
namespace small_stack_test {
using namespace logisth;
struct Context {
    Engine engine;
    Params params;
    std::shared_ptr<const SourceData> source = SourceData::factory();
    std::shared_ptr<SpectralBank> spectral = std::make_shared<SpectralBank>(
        48000, 1, std::array<int, 4>{1, 1, 1, 1}, std::shared_ptr<SpectralBank>{});
    std::atomic<bool> passed{false};
    Context() {
        engine.prepare(48000);
        params.spectralBank = spectral.get();
        for (auto &osc : params.osc) {
            osc.level = 0;
            osc.unison = 1;
            osc.data = source.get();
        }
        params.osc[0].level = .2f;
        params.osc[0].stretchMode = 6;
        for (auto &part : params.part)
            part.amp = {0, 0, 1, .01f};
    }
};
inline void exercise(Context &context) {
    auto &engine = context.engine;
    auto &params = context.params;
    float left[64]{}, right[64]{};
    for (int mode = 0; mode < 4; ++mode) {
        params.osc[0].engine = mode;
        params.monoLegato = 0;
        params.polyphony = 16;
        engine.reset();
        for (int note = 48; note < 72; ++note) {
            engine.noteOn(note, .8f, 1, params); // Also steals voices when full.
            engine.render(left, right, 64, params);
        }
        params.polyphony = 1;
        engine.render(left, right, 64, params); // Discards voices above the limit.
        params.monoLegato = 1;
        engine.noteOn(72, .8f, 1, params);
        engine.noteOn(74, .8f, 1, params);
        engine.controller(1, 120, 0, params); // MIDI All Sound Off.
        if (engine.activeVoices() != 0)
            return;
        for (float value : left)
            if (!std::isfinite(value))
                return;
    }
    engine.reset();
    context.passed = engine.activeVoices() == 0;
}
#if defined(_WIN32)
inline unsigned __stdcall run(void *data) {
#else
inline void *run(void *data) {
#endif
    try {
        exercise(*static_cast<Context *>(data));
    } catch (...) {
        static_cast<Context *>(data)->passed = false;
    }
    return 0;
}
inline bool check() {
    // All owning storage is prepared on the caller, before the audio-sized stack.
    auto context = std::make_unique<Context>();
    constexpr size_t stackBytes = 128 * 1024;
#if defined(_WIN32)
    auto thread =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, unsigned(stackBytes), run, context.get(),
                                                STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr));
    if (!thread)
        return false;
    const bool joined = WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0;
    CloseHandle(thread);
    return joined && context->passed.load();
#else
    pthread_attr_t attributes;
    if (pthread_attr_init(&attributes) != 0)
        return false;
    pthread_t thread;
    int result = pthread_attr_setstacksize(&attributes, stackBytes);
    if (result == 0)
        result = pthread_create(&thread, &attributes, run, context.get());
    pthread_attr_destroy(&attributes);
    return result == 0 && pthread_join(thread, nullptr) == 0 && context->passed.load();
#endif
}
} // namespace small_stack_test
