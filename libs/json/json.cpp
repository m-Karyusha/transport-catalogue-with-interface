#include "json.h"

#include <iostream>

namespace json {

    namespace detail{

        // Длина отступа
        static const int INDENT_LEN = 2;

        /**
         * Считывает один символ из потока input.
         * Выбрасывает исключение, если чтение не удалось
         */
        static char ReadChar(std::istream& input) {
            const int ch = input.get();
            if (!input) {
                throw ParsingError("Failed to read character from stream");
            }
            return static_cast<char>(ch);
        }

        /**
         * Проверяет, что символ ch равен expected.
         * Выбрасывает исключение с сообщением error_message при несовпадении
         */
        static void ExpectChar(char ch, char expected, const std::string& error_message) {
            if (ch != expected) {
                throw ParsingError(error_message);
            }
        }

        /**
         * Считывает из потока последовательность символов expected
         * и проверяет, что она совпадает с ожидаемой
         */
        static void ExpectSequence(std::istream& input, const std::string& expected,
                                   const std::string& error_message) {
            for (char ch : expected) {
                ExpectChar(ReadChar(input), ch, error_message);
            }
        }

        static bool TryReadChar(std::istream& input, char expected) {
            if (input.peek() == expected) {
                input.get();
                return true;
            }
            return false;
        }

        /**
         * Преобразует escape-последовательность JSON-строки в символ
         */
        static char LoadEscapedChar(std::istream& input) {
            switch (ReadChar(input)) {
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case '"':
                    return '"';
                case '\\':
                    return '\\';
                default:
                    throw ParsingError("Unrecognized escape sequence");
            }
        }

        static Node LoadNode(std::istream& input);

        /**
         * Загружает JSON-массив из потока input
         */
        static Node LoadArray(std::istream& input) {
            Array result;

            input >> std::ws;
            if (TryReadChar(input, ']')) {
                return Node(std::move(result));
            }

            while (true) {
                result.emplace_back(LoadNode(input));

                input >> std::ws;
                if (TryReadChar(input, ']')) {
                    break;
                }

                input >> std::ws;
                ExpectChar(ReadChar(input), ',', "Array parsing error");
            }

            return Node(move(result));
        }

        /**
         * Загружает JSON-число из потока input
         */
        static Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;
            bool is_int = true;

            auto read_digits = [&input, &parsed_num] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    parsed_num += ReadChar(input);
                }
            };

            // Знак
            if (input.peek() == '-') {
                parsed_num += ReadChar(input);
            }

            // Целая часть
            if (input.peek() == '0') {
                parsed_num += ReadChar(input);
            } else {
                read_digits();
            }

            // Дробная часть
            if (input.peek() == '.') {
                parsed_num += ReadChar(input);
                read_digits();
                is_int = false;
            }

            // Экспонента
            if (input.peek() == 'e' || input.peek() == 'E') {
                is_int = false;
                parsed_num += ReadChar(input);

                if (input.peek() == '+' || input.peek() == '-') {
                    parsed_num += ReadChar(input);
                }

                read_digits();
            }

            // Преобразование строки к целому или дробному числу
            try {
                if (is_int) {
                    try {
                        return Node(std::stoi(parsed_num));
                    } catch (...) {
                    }
                }
                return Node(std::stod(parsed_num));
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        /**
         * Загружает JSON-строку из потока input
         */
        static std::string LoadString(std::istream& input) {
            std::string result;

            while (true) {
                const char ch = ReadChar(input);

                if (ch == '"') {
                    return result;
                }

                if (ch == '\\') {
                    result.push_back(LoadEscapedChar(input));
                    continue;
                }

                if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line");
                }

                result.push_back(ch);
            }
        }

        /**
         * Загружает JSON-словарь из потока input
         */
        static Node LoadDict(std::istream& input) {
            Dict result;

            input >> std::ws;
            if (TryReadChar(input, '}')) {
                return Node(std::move(result));
            }

            while (true) {
                input >> std::ws;
                ExpectChar(ReadChar(input), '"', "Dict parsing error");
                std::string key = LoadString(input);

                input >> std::ws;
                ExpectChar(ReadChar(input), ':', "Dict parsing error");
                result.insert({ std::move(key), LoadNode(input) });

                input >> std::ws;
                if (TryReadChar(input, '}')) {
                    break;
                }
                ExpectChar(ReadChar(input), ',', "Dict parsing error");
            }

            return Node(std::move(result));
        }

        /**
         * Загружает один JSON-узел из потока input
         */
        static Node LoadNode(std::istream& input) {
            input >> std::ws;
            const char c = ReadChar(input);

            // Определение типа узла по ближайшему символу
            if (c == '[') {
                return LoadArray(input);
            } else if (c == '{') {
                return LoadDict(input);
            } else if (c == '"') {
                return Node(LoadString(input));
            } else if (c == 't') {
                ExpectSequence(input, "rue", "Invalid value");
                return Node(true);
            } else if (c == 'f') {
                ExpectSequence(input, "alse", "Invalid value");
                return Node(false);
            } else if (c == 'n') {
                ExpectSequence(input, "ull", "Invalid value");
                return Node(nullptr);
            } else {
                input.putback(c);
                return LoadNumber(input);
            }

            throw ParsingError("Invalid value");
        }

        static void PrintString(const std::string& value, std::ostream& out) {
            out.put('"');
            for (char c : value) {
                switch (c) {
                    case '\n':
                        out << "\\n";
                        break;
                    case '\r':
                        out << "\\r";
                        break;
                    case '\t':
                        out << "\\t";
                        break;
                    case '"':
                        out << "\\\"";
                        break;
                    case '\\':
                        out << "\\\\";
                        break;
                    default:
                        out.put(c);
                        break;
                }
            }
            out.put('"');
        }

        static void PrintIndent(std::ostream& out, int indent) {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        static void PrintNode(const Node& node, std::ostream& out, int indent);

        static void PrintArray(const Array& array, std::ostream& out, int indent) {
            out << "[\n";

            bool first = true;
            for (const auto& node : array) {
                if (!first) {
                    out << ",\n";
                }

                PrintIndent(out, indent + INDENT_LEN);
                PrintNode(node, out, indent + INDENT_LEN);

                first = false;
            }

            out << "\n";
            PrintIndent(out, indent);
            out << "]";
        }

        static void PrintDict(const Dict& dict, std::ostream& out, int indent) {
            out << "{\n";

            bool first = true;
            for (const auto& [key, value] : dict) {
                if (!first) {
                    out << ",\n";
                }

                PrintIndent(out, indent + INDENT_LEN);
                PrintString(key, out);
                out << ": ";
                PrintNode(value, out, indent + INDENT_LEN);

                first = false;
            }

            out << "\n";
            PrintIndent(out, indent);
            out << "}";
        }

        template <typename Value>
        static void PrintValue(const Value& value, std::ostream& out, int) {
            out << value;
        }

        static void PrintValue(nullptr_t, std::ostream& out, int) {
            out << "null";
        }

        static void PrintValue(const std::string& value, std::ostream& out, int) {
            PrintString(value, out);
        }

        static void PrintValue(const Array& value, std::ostream& out, int indent) {
            PrintArray(value, out, indent);
        }

        static void PrintValue(const Dict& value, std::ostream& out, int indent) {
            PrintDict(value, out, indent);
        }

        static void PrintValue(bool value, std::ostream& out, int) {
            out << (value ? "true" : "false");
        }

        static void PrintNode(const Node& node, std::ostream& out, int indent = 0) {
            visit(
                [&out, indent](const auto& value) {
                    PrintValue(value, out, indent);
                },
                node.GetValue()
            );
        }

    }  // namespace detail

    bool Node::IsInt() const {
        return holds_alternative<int>(*this);
    }

    bool Node::IsDouble() const {
        return IsInt() || holds_alternative<double>(*this);
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(*this);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(*this);
    }

    bool Node::IsString() const {
        return holds_alternative<std::string>(*this);
    }

    bool Node::IsNull() const {
        return holds_alternative<std::nullptr_t>(*this);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(*this);
    }

    bool Node::IsMap() const {
        return holds_alternative<Dict>(*this);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Node is not an array");
        }
        return get<Array>(*this);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Node is not a map");
        }
        return get<Dict>(*this);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Node is not an int");
        }
        return get<int>(*this);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Node is not a bool");
        }
        return get<bool>(*this);
    }

    double Node::AsDouble() const {
        if (IsInt()) {
            return AsInt();
        }
        if (IsPureDouble()) {
            return get<double>(*this);
        }
        throw std::logic_error("Node is not a double");
    }

    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Node is not a string");
        }
        return get<std::string>(*this);
    }

    const Node::variant& Node::GetValue() const {
        return static_cast<const variant&>(*this);;
    }

    bool Node::operator==(const Node& other) const {
        return GetValue() == other.GetValue();
    }

    bool Node::operator!=(const Node& other) const {
        return !(*this == other);
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document& other) const {
        return root_ == other.root_;
    }

    bool Document::operator!=(const Document& other) const {
        return !(*this == other);
    }

    Document Load(std::istream& input) {
        Document doc{detail::LoadNode(input)};

        input >> std::ws;
        if (input.peek() != std::char_traits<char>::eof()) {
            throw ParsingError("Extra characters after JSON document");
        }

        return doc;
    }

    void Print(const Document& doc, std::ostream& output) {
        detail::PrintNode(doc.GetRoot(), output);
    }

}  // namespace json
