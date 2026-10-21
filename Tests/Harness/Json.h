#pragma once

#include <juce_core/juce_core.h>

namespace hpeq::test
{
// Tiny builder over juce::DynamicObject so report code reads as data.
class JsonObject
{
public:
    JsonObject() : object(new juce::DynamicObject()) {}

    JsonObject& set(const juce::Identifier& key, const juce::var& value)
    {
        object->setProperty(key, value);
        return *this;
    }

    operator juce::var() const { return juce::var(object.get()); }

private:
    juce::DynamicObject::Ptr object;
};

inline juce::var jsonArray() { return juce::var(juce::Array<juce::var>{}); }
} // namespace hpeq::test
