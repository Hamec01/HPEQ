#include "Link/HamloLinkClient.h"

#include <cstring>

namespace hamlo::link
{
Client::Client()
    : juce::Thread("Hamlo HPDG Link"), scratch(std::make_unique<Snapshot>()), latest(std::make_unique<Snapshot>())
{
}

Client::~Client()
{
    disconnect();
}

const char* Client::stateName(State s) noexcept
{
    switch (s)
    {
        case State::Off: return "OFF";
        case State::Searching: return "SEARCHING";
        case State::Linked: return "LINKED";
        case State::Lost: return "LOST";
        case State::Update: return "UPDATE HPDG";
    }
    return "";
}

void Client::connect(std::uint64_t instanceId, bool startPolling)
{
    stopThread(2000);
    {
        const juce::ScopedLock lock(pollLock);
        slot.reset();
        openedId = 0;
        everLinked = false;
    }
    requestedId = instanceId;
    linkedId = instanceId;
    wanted = true;
    state = State::Searching;
    if (startPolling)
        startThread(juce::Thread::Priority::low);
}

void Client::disconnect()
{
    wanted = false;
    stopThread(2000);
    const juce::ScopedLock lock(pollLock);
    slot.reset();
    openedId = 0;
    state = State::Off;
}

bool Client::takeIfNew(Snapshot& out)
{
    const juce::ScopedLock lock(latestLock);
    if (!fresh)
        return false;
    std::memcpy(&out, latest.get(), sizeof(Snapshot));
    fresh = false;
    return true;
}

bool Client::last(Snapshot& out) const
{
    const juce::ScopedLock lock(latestLock);
    if (!haveLatest)
        return false;
    std::memcpy(&out, latest.get(), sizeof(Snapshot));
    return true;
}

void Client::run()
{
    while (!threadShouldExit())
    {
        pollOnce(juce::Time::currentTimeMillis());
        wait(100); // 10 Hz
    }
}

void Client::pollOnce(juce::int64 nowMs)
{
    if (!wanted)
        return;
    const juce::ScopedLock lock(pollLock);
    const auto lostOrSearching = [this] { state = everLinked ? State::Lost : State::Searching; };

    // which HPDG: the requested one, or (auto) the first live one, then kept
    std::uint64_t id = linkedId.load();
    if (id == 0)
    {
        for (const auto& l : listInstances(nowMs))
            if (l.status == Status::Ok)
            {
                id = l.instanceId;
                break;
            }
        if (id == 0)
        {
            lostOrSearching();
            return;
        }
        linkedId = id;
    }
    if (slot == nullptr || openedId != id)
    {
        slot = SharedSlot::openForReading(slotFile(id));
        openedId = slot != nullptr ? id : 0;
        if (slot == nullptr)
        {
            lostOrSearching();
            return;
        }
    }

    auto status = slot->read(*scratch);
    if (status == Status::Busy)
        return; // try again next poll
    if (status == Status::Ok)
        status = check(*scratch, nowMs);
    switch (status)
    {
        case Status::Ok:
        {
            everLinked = true;
            state = State::Linked;
            const juce::ScopedLock latestGuard(latestLock);
            if (!haveLatest || !samePayload(*scratch, *latest))
            {
                std::memcpy(latest.get(), scratch.get(), sizeof(Snapshot));
                haveLatest = true;
                fresh = true;
            }
            return;
        }
        case Status::NewerMajor:
            state = State::Update;
            return;
        case Status::Stale:
        case Status::Missing:
        case Status::Busy:
            // HPDG closed or crashed: drop the mapping (its file goes away); reopen when it returns
            slot.reset();
            openedId = 0;
            if (requestedId.load() == 0 && !everLinked)
                linkedId = 0; // auto and never linked: look for any live HPDG again
            lostOrSearching();
            return;
    }
}
} // namespace hamlo::link
