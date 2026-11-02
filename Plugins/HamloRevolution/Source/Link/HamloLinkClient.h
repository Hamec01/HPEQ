#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "Link/HamloLinkProtocol.h"

namespace hamlo::link
{
// HPDG Link, the plug-in side (docs/HPDG_LINK.md): CONNECT TO HPDG. A background thread polls the chosen HPDG slot at
// 10 Hz (read-only, never blocking anything); the message thread takes a copy when HPDG published something new. If
// HPDG closes, the state turns LOST and the last snapshot is kept; when that HPDG comes back (the project reopened:
// the same instance id) the link resumes by itself. Never touched by the audio thread.
class Client final : private juce::Thread
{
public:
    enum class State
    {
        Off,       // not connected
        Searching, // connected, no live HPDG (yet)
        Linked,    // a live HPDG, snapshots arriving
        Lost,      // was linked; HPDG gone (last snapshot kept)
        Update     // the HPDG publishes a newer protocol: refused
    };

    Client();
    ~Client() override;

    // instanceId 0 = the first live HPDG found (then that one). Starts the polling thread (tests: poll by hand).
    void connect(std::uint64_t instanceId, bool startPolling = true);
    void disconnect();
    bool isConnected() const noexcept { return wanted.load(); }

    State getState() const noexcept { return state.load(); }
    std::uint64_t getInstanceId() const noexcept { return linkedId.load(); } // the HPDG followed (0 = none yet)
    static const char* stateName(State) noexcept;

    // Message thread: the newest snapshot if it changed since the last take (true), else false. `out` untouched then.
    bool takeIfNew(Snapshot& out);
    // The last snapshot received (also after LOST); false when none yet.
    bool last(Snapshot& out) const;

    // Test hook: poll once on the caller's thread (the thread is not needed then).
    void pollOnce(juce::int64 nowMs);

private:
    void run() override;

    std::atomic<bool> wanted { false };
    std::atomic<State> state { State::Off };
    std::atomic<std::uint64_t> requestedId { 0 }, linkedId { 0 };

    juce::CriticalSection pollLock;          // the slot and the scratch: the poller only
    std::unique_ptr<SharedSlot> slot;
    std::unique_ptr<Snapshot> scratch;
    std::uint64_t openedId = 0;
    bool everLinked = false;

    mutable juce::CriticalSection latestLock; // the poller writes, the message thread reads
    std::unique_ptr<Snapshot> latest;
    bool haveLatest = false, fresh = false;
};
} // namespace hamlo::link
