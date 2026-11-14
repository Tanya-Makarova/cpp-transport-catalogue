#include "json.h"

#include <cmath>
#include <cstdlib>
#include <charconv>
#include <iterator>
#include <string_view>
#include <utility>

namespace json {

namespace {

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {
    }

    Node ParseDocument() {
        Node root = ParseValue();
        SkipSpace();
        if (position_ != text_.size()) {
            Fail();
        }
        return root;
    }

private:
    [[noreturn]] void Fail() const {
        throw ParsingError("Invalid JSON at position " + std::to_string(position_));
    }

    char Peek() const {
        return position_ < text_.size() ? text_[position_] : '\0';
    }

    char Take() {
        if (position_ == text_.size()) {
            Fail();
        }
        return text_[position_++];
    }

    void Expect(char expected) {
        if (Take() != expected) {
            Fail();
        }
    }

    void SkipSpace() {
        while (position_ < text_.size()) {
            const char ch = Peek();
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                break;
            }
            ++position_;
        }
    }

    Node ParseValue() {
        SkipSpace();
        switch (Peek()) {
            case '[': return ParseArray();
            case '{': return ParseObject();
            case '"': return Node{ParseString()};
            case 't': return ParseLiteral("true", true);
            case 'f': return ParseLiteral("false", false);
            case 'n': return ParseLiteral("null", nullptr);
            default:
                if (Peek() == '-' || (Peek() >= '0' && Peek() <= '9')) {
                    return ParseNumber();
                }
                Fail();
        }
    }

    Node ParseLiteral(std::string_view word, Node value) {
        for (char expected : word) {
            Expect(expected);
        }
        return value;
    }

    Node ParseArray() {
        Expect('[');
        Array array;
        SkipSpace();
        if (Peek() == ']') {
            Take();
            return array;
        }
        while (true) {
            array.push_back(ParseValue());
            SkipSpace();
            if (Peek() == ']') {
                Take();
                return array;
            }
            Expect(',');
        }
    }

    Node ParseObject() {
        Expect('{');
        Dict dict;
        SkipSpace();
        if (Peek() == '}') {
            Take();
            return dict;
        }
        while (true) {
            SkipSpace();
            if (Peek() != '"') {
                Fail();
            }
            std::string key = ParseString();
            SkipSpace();
            Expect(':');
            dict.insert_or_assign(std::move(key), ParseValue());
            SkipSpace();
            if (Peek() == '}') {
                Take();
                return dict;
            }
            Expect(',');
        }
    }

    int ParseHex4() {
        int value = 0;
        for (int i = 0; i < 4; ++i) {
            const char ch = Take();
            value <<= 4;
            if (ch >= '0' && ch <= '9') {
                value += ch - '0';
            } else if (ch >= 'a' && ch <= 'f') {
                value += ch - 'a' + 10;
            } else if (ch >= 'A' && ch <= 'F') {
                value += ch - 'A' + 10;
            } else {
                Fail();
            }
        }
        return value;
    }

    static void AppendUtf8(std::string& result, unsigned code_point) {
        if (code_point <= 0x7F) {
            result.push_back(static_cast<char>(code_point));
        } else if (code_point <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
            result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else if (code_point <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
            result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
            result.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
    }

    std::string ParseString() {
        Expect('"');
        std::string result;
        while (position_ < text_.size()) {
            const unsigned char ch = static_cast<unsigned char>(Take());
            if (ch == '"') {
                return result;
            }
            if (ch < 0x20) {
                Fail();
            }
            if (ch != '\\') {
                result.push_back(static_cast<char>(ch));
                continue;
            }
            switch (Take()) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': {
                    unsigned code_point = static_cast<unsigned>(ParseHex4());
                    if (code_point >= 0xD800 && code_point <= 0xDBFF) {
                        Expect('\\');
                        Expect('u');
                        const unsigned low = static_cast<unsigned>(ParseHex4());
                        if (low < 0xDC00 || low > 0xDFFF) {
                            Fail();
                        }
                        code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low - 0xDC00);
                    } else if (code_point >= 0xDC00 && code_point <= 0xDFFF) {
                        Fail();
                    }
                    AppendUtf8(result, code_point);
                    break;
                }
                default: Fail();
            }
        }
        Fail();
    }

    Node ParseNumber() {
        const size_t start = position_;
        if (Peek() == '-') {
            Take();
        }
        if (Peek() == '0') {
            Take();
        } else {
            if (Peek() < '1' || Peek() > '9') {
                Fail();
            }
            do {
                Take();
            } while (Peek() >= '0' && Peek() <= '9');
        }

        bool floating = false;
        if (Peek() == '.') {
            floating = true;
            Take();
            if (Peek() < '0' || Peek() > '9') {
                Fail();
            }
            do {
                Take();
            } while (Peek() >= '0' && Peek() <= '9');
        }
        if (Peek() == 'e' || Peek() == 'E') {
            floating = true;
            Take();
            if (Peek() == '+' || Peek() == '-') {
                Take();
            }
            if (Peek() < '0' || Peek() > '9') {
                Fail();
            }
            do {
                Take();
            } while (Peek() >= '0' && Peek() <= '9');
        }

        const std::string_view token = text_.substr(start, position_ - start);
        if (!floating) {
            int integer = 0;
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), integer);
            if (parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size()) {
                return integer;
            }
        }
        const std::string number(token);
        char* end = nullptr;
        const double value = std::strtod(number.c_str(), &end);
        if (end != number.c_str() + number.size() || !std::isfinite(value)) {
            Fail();
        }
        return value;
    }

    std::string_view text_;
    size_t position_ = 0;
};

void PrintString(const std::string& value, std::ostream& output) {
    constexpr char digits[] = "0123456789abcdef";
    output.put('"');
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            default:
                if (ch < 0x20) {
                    output << "\\u00" << digits[ch >> 4] << digits[ch & 0x0F];
                } else {
                    output.put(static_cast<char>(ch));
                }
        }
    }
    output.put('"');
}

void PrintNode(const Node& node, std::ostream& output) {
    if (node.IsNull()) {
        output << "null";
    } else if (node.IsBool()) {
        output << (node.AsBool() ? "true" : "false");
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsPureDouble()) {
        if (!std::isfinite(node.AsDouble())) {
            throw std::logic_error("Non-finite double is not valid JSON");
        }
        output << node.AsDouble();
    } else if (node.IsString()) {
        PrintString(node.AsString(), output);
    } else if (node.IsArray()) {
        output.put('[');
        bool first = true;
        for (const Node& element : node.AsArray()) {
            if (!first) {
                output.put(',');
            }
            first = false;
            PrintNode(element, output);
        }
        output.put(']');
    } else {
        output.put('{');
        bool first = true;
        for (const auto& [key, value] : node.AsMap()) {
            if (!first) {
                output.put(',');
            }
            first = false;
            PrintString(key, output);
            output.put(':');
            PrintNode(value, output);
        }
        output.put('}');
    }
}

}  // namespace

Node::Node(std::nullptr_t) : value_(nullptr) {
}
Node::Node(Array array) : value_(std::move(array)) {
}
Node::Node(Dict map) : value_(std::move(map)) {
}
Node::Node(int value) : value_(value) {
}
Node::Node(double value) : value_(value) {
}
Node::Node(bool value) : value_(value) {
}
Node::Node(std::string value) : value_(std::move(value)) {
}
Node::Node(const char* value) : value_(std::string(value)) {
}

bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
bool Node::IsDouble() const { return IsInt() || IsPureDouble(); }
bool Node::IsPureDouble() const { return std::holds_alternative<double>(value_); }
bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

int Node::AsInt() const {
    if (const auto* value = std::get_if<int>(&value_)) {
        return *value;
    }
    throw std::logic_error("Node is not an int");
}

double Node::AsDouble() const {
    if (const auto* value = std::get_if<double>(&value_)) {
        return *value;
    }
    if (const auto* value = std::get_if<int>(&value_)) {
        return static_cast<double>(*value);
    }
    throw std::logic_error("Node is not a number");
}

bool Node::AsBool() const {
    if (const auto* value = std::get_if<bool>(&value_)) {
        return *value;
    }
    throw std::logic_error("Node is not a bool");
}

const std::string& Node::AsString() const {
    if (const auto* value = std::get_if<std::string>(&value_)) {
        return *value;
    }
    throw std::logic_error("Node is not a string");
}

const Array& Node::AsArray() const {
    if (const auto* value = std::get_if<Array>(&value_)) {
        return *value;
    }
    throw std::logic_error("Node is not an array");
}

const Dict& Node::AsMap() const {
    if (const auto* value = std::get_if<Dict>(&value_)) {
        return *value;
    }
    throw std::logic_error("Node is not a map");
}

bool operator==(const Node& lhs, const Node& rhs) {
    return lhs.value_ == rhs.value_;
}

bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}

Document::Document(Node root) : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}

bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

Document Load(std::istream& input) {
    const std::string text(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
    return Document{Parser(text).ParseDocument()};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json
