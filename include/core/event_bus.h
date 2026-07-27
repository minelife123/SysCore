#ifndef SYSCORE_EVENT_BUS_H
#define SYSCORE_EVENT_BUS_H

#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <memory>

namespace SysCore::Events {

    using SubscriptionId = uint64_t;

    class IEventSubscription {
    public:
        virtual ~IEventSubscription() = default;
    };

    template <typename EventType>
    class EventSubscription : public IEventSubscription {
    public:
        SubscriptionId id;
        std::function<void(const EventType&)> callback;

        EventSubscription(SubscriptionId subId, std::function<void(const EventType&)> cb)
            : id(subId), callback(std::move(cb)) {}
    };

    class EventBus {
    private:
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<IEventSubscription>>> m_subscribers;
        mutable std::shared_mutex m_mutex;
        std::atomic<SubscriptionId> m_nextId{1};

    public:
        EventBus() = default;
        ~EventBus() = default;

        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

        // Subscribe to a specific event type
        template <typename EventType>
        SubscriptionId Subscribe(std::function<void(const EventType&)> callback) {
            std::unique_lock lock(m_mutex);
            SubscriptionId subId = m_nextId.fetch_add(1, std::memory_order_relaxed);
            auto typeIdx = std::type_index(typeid(EventType));

            auto sub = std::make_shared<EventSubscription<EventType>>(subId, std::move(callback));
            m_subscribers[typeIdx].push_back(sub);

            return subId;
        }

        // Publish event to all registered subscribers
        template <typename EventType>
        void Publish(const EventType& event) const {
            std::shared_lock lock(m_mutex);
            auto typeIdx = std::type_index(typeid(EventType));

            auto it = m_subscribers.find(typeIdx);
            if (it != m_subscribers.end()) {
                for (const auto& baseSub : it->second) {
                    auto typedSub = std::static_pointer_cast<EventSubscription<EventType>>(baseSub);
                    if (typedSub && typedSub->callback) {
                        typedSub->callback(event);
                    }
                }
            }
        }

        // Clear all subscriptions
        void Clear() {
            std::unique_lock lock(m_mutex);
            m_subscribers.clear();
        }
    };

} // namespace SysCore::Events

#endif // SYSCORE_EVENT_BUS_H
