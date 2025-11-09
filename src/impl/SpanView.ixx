module;

#include <span>
#include <generator>
#include <variant>

export module SpanView;

export class SpanView {
private:
    std::variant<std::span<const int>, std::generator<const int>> view_variant;

public:
    SpanView(std::span<const int> s) : view_variant(s) {}
    SpanView(std::generator<const int>&& g) : view_variant(std::move(g)) {}

    // move-only semantics
    SpanView(const SpanView&) = delete;
    SpanView(SpanView&&) = default;
    SpanView& operator=(const SpanView&) = delete;
    SpanView& operator=(SpanView&&) = default;

    // consuming for_each, requires rvalue
    template<typename F>
    void for_each(F func) && {
        std::visit([&func](auto&& range) {
            for (const auto& val : range) {
                func(val);
            }
        }, view_variant);
    }
};