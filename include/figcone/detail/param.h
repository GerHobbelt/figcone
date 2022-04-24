#pragma once
#include "iparam.h"
#include "iconfigentity.h"
#include "utils.h"
#include <figcone_tree/tree.h>
#include <figcone/errors.h>
#include <figcone/stringconverter.h>
#include <string>
#include <sstream>
#include <algorithm>

namespace figcone::detail {

template<typename T>
class ConfigParam : public IConfigParam{
public:
    ConfigParam(std::string name, T& paramValue)
        : name_{std::move(name)}
        , paramValue_{paramValue}
    {
    }

    void markValueIsSet()
    {
        hasValue_ = true;
    }

private:
    void load(const figcone::TreeParam& param) override
    {
        auto paramVal = detail::convertFromString<T>(param.value());
        if (!paramVal)
            throw ConfigError{"Couldn't set parameter '" + name_ + "' value from '" + param.value() + "'", param.position()};
        paramValue_ = *paramVal;
        hasValue_ = true;
        position_ = param.position();
    }

    bool hasValue() const override
    {
        if constexpr (detail::is_optional<T>::value)
            return true;
        return hasValue_;
    }

    StreamPosition position() override
    {
        return position_;
    }

    std::string description() override
    {
        return "Parameter '" + name_ + "'";
    }

private:
    std::string name_;
    T& paramValue_;
    bool hasValue_ = false;
    StreamPosition position_;
};

}