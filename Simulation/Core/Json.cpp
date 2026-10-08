#include "Simulation/Core/Json.hpp"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace cannaville::json {

Value::Value() : storage_(nullptr) {}
Value::Value(std::nullptr_t) : storage_(nullptr) {}
Value::Value(bool value) : storage_(value) {}
Value::Value(double value) : storage_(value) {}
Value::Value(std::string value) : storage_(std::move(value)) {}
Value::Value(const char* value) : storage_(std::string(value)) {}
Value::Value(Object value) : storage_(std::move(value)) {}
Value::Value(Array value) : storage_(std::move(value)) {}

bool Value::is_null() const { return std::holds_alternative<std::nullptr_t>(storage_); }
bool Value::is_boolean() const { return std::holds_alternative<bool>(storage_); }
bool Value::is_number() const { return std::holds_alternative<double>(storage_); }
bool Value::is_string() const { return std::holds_alternative<std::string>(storage_); }
bool Value::is_object() const { return std::holds_alternative<Object>(storage_); }
bool Value::is_array() const { return std::holds_alternative<Array>(storage_); }

bool Value::as_boolean() const { return std::get<bool>(storage_); }
double Value::as_number() const { return std::get<double>(storage_); }
const std::string& Value::as_string() const { return std::get<std::string>(storage_); }
const Value::Object& Value::as_object() const { return std::get<Object>(storage_); }
const Value::Array& Value::as_array() const { return std::get<Array>(storage_); }

const Value* Value::find(std::string_view key) const {
    if (!is_object()) {
        return nullptr;
    }
    const auto& object = as_object();
    const auto it = object.find(std::string(key));
    return it == object.end() ? nullptr : &it->second;
}

const Value& Value::require(std::string_view key) const {
    const Value* value = find(key);
    if (value == nullptr) {
        throw ParseError("missing required JSON field: " + std::string(key));
    }
    return *value;
}

namespace {

class Parser {
public:
    explicit Parser(std::string_view input) : input_(input) {}

    Value run() {
        skip_whitespace();
        Value result = parse_value();
        skip_whitespace();
        if (position_ != input_.size()) {
            fail("trailing characters");
        }
        return result;
    }

private:
    [[noreturn]] void fail(const std::string& message) const {
        throw ParseError(message + " at byte " + std::to_string(position_));
    }

    void skip_whitespace() {
        while (position_ < input_.size()) {
            const char c = input_[position_];
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                break;
            }
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ < input_.size() && input_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(char expected) {
        if (!consume(expected)) {
            fail(std::string("expected '") + expected + "'");
        }
    }

    Value parse_value() {
        skip_whitespace();
        if (position_ >= input_.size()) {
            fail("expected value");
        }
        switch (input_[position_]) {
        case '{': return parse_object();
        case '[': return parse_array();
        case '"': return Value(parse_string());
        case 't': parse_literal("true"); return Value(true);
        case 'f': parse_literal("false"); return Value(false);
        case 'n': parse_literal("null"); return Value(nullptr);
        default: return parse_number();
        }
    }

    Value parse_object() {
        expect('{');
        Value::Object object;
        skip_whitespace();
        if (consume('}')) {
            return Value(std::move(object));
        }
        while (true) {
            skip_whitespace();
            if (position_ >= input_.size() || input_[position_] != '"') {
                fail("object key must be a string");
            }
            const std::string key = parse_string();
            skip_whitespace();
            expect(':');
            Value value = parse_value();
            if (!object.emplace(key, std::move(value)).second) {
                fail("duplicate object key: " + key);
            }
            skip_whitespace();
            if (consume('}')) {
                return Value(std::move(object));
            }
            expect(',');
        }
    }

    Value parse_array() {
        expect('[');
        Value::Array array;
        skip_whitespace();
        if (consume(']')) {
            return Value(std::move(array));
        }
        while (true) {
            array.push_back(parse_value());
            skip_whitespace();
            if (consume(']')) {
                return Value(std::move(array));
            }
            expect(',');
        }
    }

    void parse_literal(std::string_view literal) {
        if (input_.substr(position_, literal.size()) != literal) {
            fail("invalid literal");
        }
        position_ += literal.size();
    }

    std::string parse_string() {
        expect('"');
        std::string result;
        while (position_ < input_.size()) {
            const char c = input_[position_++];
            if (c == '"') {
                return result;
            }
            if (c == '\\') {
                if (position_ >= input_.size()) {
                    fail("unterminated escape");
                }
                const char escaped = input_[position_++];
                switch (escaped) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default: fail("unsupported JSON escape");
                }
            } else {
                if (static_cast<unsigned char>(c) < 0x20U) {
                    fail("control character in string");
                }
                result.push_back(c);
            }
        }
        fail("unterminated string");
    }

    Value parse_number() {
        const std::size_t start = position_;
        if (position_ < input_.size() && input_[position_] == '-') {
            ++position_;
        }
        if (position_ >= input_.size() || input_[position_] < '0' || input_[position_] > '9') {
            fail("invalid number");
        }
        if (input_[position_] == '0') {
            ++position_;
        } else {
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < input_.size() && input_[position_] == '.') {
            ++position_;
            const std::size_t fraction_start = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
            if (fraction_start == position_) {
                fail("fraction requires digits");
            }
        }
        if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) {
                ++position_;
            }
            const std::size_t exponent_start = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
            if (exponent_start == position_) {
                fail("exponent requires digits");
            }
        }
        const std::string token(input_.substr(start, position_ - start));
        try {
            const double value = std::stod(token);
            if (!std::isfinite(value)) {
                fail("number must be finite");
            }
            return Value(value);
        } catch (const std::exception&) {
            fail("invalid number");
        }
    }

    std::string_view input_;
    std::size_t position_{0};
};

void append_escaped(std::ostringstream& output, const std::string& value) {
    output << '"';
    for (const char c : value) {
        switch (c) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default: output << c; break;
        }
    }
    output << '"';
}

void append_value(std::ostringstream& output, const Value& value) {
    if (value.is_null()) {
        output << "null";
    } else if (value.is_boolean()) {
        output << (value.as_boolean() ? "true" : "false");
    } else if (value.is_number()) {
        output << std::setprecision(std::numeric_limits<double>::max_digits10) << value.as_number();
    } else if (value.is_string()) {
        append_escaped(output, value.as_string());
    } else if (value.is_array()) {
        output << '[';
        bool first = true;
        for (const Value& element : value.as_array()) {
            if (!first) output << ',';
            first = false;
            append_value(output, element);
        }
        output << ']';
    } else {
        output << '{';
        bool first = true;
        for (const auto& [key, element] : value.as_object()) {
            if (!first) output << ',';
            first = false;
            append_escaped(output, key);
            output << ':';
            append_value(output, element);
        }
        output << '}';
    }
}

} // namespace

Value parse(std::string_view input) {
    return Parser(input).run();
}

std::string stringify(const Value& value) {
    std::ostringstream output;
    append_value(output, value);
    return output.str();
}

} // namespace cannaville::json
