#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace cannaville::json {

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Value {
public:
    using Object = std::map<std::string, Value>;
    using Array = std::vector<Value>;

    Value();
    Value(std::nullptr_t);
    Value(bool value);
    Value(double value);
    Value(std::string value);
    Value(const char* value);
    Value(Object value);
    Value(Array value);

    bool is_null() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_string() const;
    bool is_object() const;
    bool is_array() const;

    bool as_boolean() const;
    double as_number() const;
    const std::string& as_string() const;
    const Object& as_object() const;
    const Array& as_array() const;

    const Value* find(std::string_view key) const;
    const Value& require(std::string_view key) const;

private:
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Object, Array>;
    Storage storage_;
};

Value parse(std::string_view input);
std::string stringify(const Value& value);

} // namespace cannaville::json
