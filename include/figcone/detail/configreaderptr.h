#ifndef FIGCONE_CONFIGREADERPTR_H
#define FIGCONE_CONFIGREADERPTR_H

namespace figcone {
class ConfigReader;
}

namespace figcone::detail {

class ConfigReaderPtr {
    friend class figcone::ConfigReader;

private:
    ConfigReaderPtr(ConfigReader* reader)
        : reader_{reader}
    {
    }

public:
    ConfigReaderPtr() = default;

    const ConfigReader* operator->() const
    {
        return reader_;
    }

    ConfigReader* operator->()
    {
        return reader_;
    }

    const ConfigReader& operator*() const
    {
        return *reader_;
    }

    ConfigReader& operator*()
    {
        return *reader_;
    }

    operator bool() const
    {
        return reader_ != nullptr;
    }

private:
    ConfigReader* reader_ = nullptr;
};

} //namespace figcone::detail

#endif //FIGCONE_CONFIGREADERPTR_H
