module;

# include  <variant>
# include  <vector>
# include  <string>
# include  <iostream>
# include <utility>
#include <concepts>
#include <iostream>
#include <vector>
#include <utility>
#include <variant>
#include <ranges>

export module AlgorithmResult;

export using AlgoResultVariant = std::variant<int, std::vector<int>, std::string, std::vector<std::pair<int, int>>>;

template <typename stream_type>
concept OutputStreamable = requires(stream_type& os, const std::string value) {
    { os << value } -> std::same_as<stream_type&>;
};

export
template <typename OS = std::ostream>
requires OutputStreamable<OS>
void printResult(const AlgoResultVariant& result, OS& os = std::cout) {
    os << "output: ";
    if (std::holds_alternative<int>(result)) {
        os << std::get<int>(result) << std::endl;
    } else if (std::holds_alternative<std::vector<int>>(result)) {
        const auto& vec = std::get<std::vector<int>>(result);
        os << vec.size() << "\n";
    } else if (std::holds_alternative<std::vector<std::pair<int, int>>>(result)) {
        const auto& vec = std::get<std::vector<std::pair<int, int>>>(result);
        os << "\n";
        for (const auto& [key, value] : vec) {
            os << key << " " << value << "\n";
        }
    }
    else if (std::holds_alternative<std::string>(result)) {
        os << std::get<std::string>(result) << std::endl;
    }
    else {
        os << "unexpected type, cannot print" << std::endl;
    }
}