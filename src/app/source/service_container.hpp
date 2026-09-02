#ifndef SERVICE_CONTAINER_HPP
#define SERVICE_CONTAINER_HPP

#include <QString>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>

class ServiceContainer
{
    using ErasedFactory = std::function<std::shared_ptr<void>(ServiceContainer &)>;

public:
    void clear()
    {
        m_singletons.clear();
        m_singletonFactories.clear();
        m_factories.clear();
    }

    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>(ServiceContainer &)> factory)
    {
        const auto typeId = std::type_index(typeid(T));
        m_factories[typeId] = [factory](ServiceContainer &container) -> std::shared_ptr<void> {
            return factory(container);
        };
        m_singletonFactories.erase(typeId);
        m_singletons.erase(typeId);
    }

    template<typename T>
    void registerSingleton(std::function<std::shared_ptr<T>(ServiceContainer &)> factory)
    {
        const auto typeId = std::type_index(typeid(T));
        m_singletonFactories[typeId] = [factory](ServiceContainer &container) -> std::shared_ptr<void> {
            return factory(container);
        };
        m_factories.erase(typeId);
        m_singletons.erase(typeId);
    }

    template<typename T>
    std::shared_ptr<T> resolve()
    {
        const auto typeId = std::type_index(typeid(T));

        if (auto it = m_singletons.find(typeId); it != m_singletons.end()) {
            return std::static_pointer_cast<T>(it->second);
        }

        if (auto it = m_singletonFactories.find(typeId); it != m_singletonFactories.end()) {
            std::shared_ptr<void> instance = it->second(*this);
            m_singletons[typeId] = instance;
            return std::static_pointer_cast<T>(instance);
        }

        if (auto it = m_factories.find(typeId); it != m_factories.end()) {
            return std::static_pointer_cast<T>(it->second(*this));
        }

        throw std::runtime_error(
            QString("ServiceContainer: no registration found for type '%1'")
                .arg(typeid(T).name())
                .toStdString()
            );
    }

private:
    std::unordered_map<std::type_index, ErasedFactory> m_factories;
    std::unordered_map<std::type_index, ErasedFactory> m_singletonFactories;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_singletons;
};

#endif // SERVICE_CONTAINER_HPP
