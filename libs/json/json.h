#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class Node;

    // Словарь JSON
    using Dict = std::map<std::string, Node>;

    // Массив JSON
    using Array = std::vector<Node>;

    /**
     * Ошибка парсинга JSON-документа
     */
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    /**
     * Узел JSON-документа.
     * Может хранить одно из значений JSON:
     * null, массив, словарь, bool, int, double или строку.
     */
    class Node final : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
    public:
        using variant::variant;

        /**
         * Возвращает true, если узел хранит целое число
         */
        bool IsInt() const;

        /**
         * Возвращает true, если узел хранит число
         * (как int или double)
         */
        bool IsDouble() const;

        /**
         * Возвращает true, если узел хранит число типа double
         */
        bool IsPureDouble() const;

        /**
         * Возвращает true, если узел хранит bool
         */
        bool IsBool() const;

        /**
         * Возвращает true, если узел хранит строку
         */
        bool IsString() const;

        /**
         * Возвращает true, если узел хранит null
         */
        bool IsNull() const;

        /**
         * Возвращает true, если узел хранит массив
         */
        bool IsArray() const;

        /**
         * Возвращает true, если узел хранит словарь
         */
        bool IsMap() const;

        /**
         * Возвращает массив из узла
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        const Array& AsArray() const;

        /**
         * Возвращает словарь из узла
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        const Dict& AsMap() const;

        /**
         * Возвращает целое число из узла
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        int AsInt() const;

        /**
         * Возвращает bool из узла
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        bool AsBool() const;

        /**
         * Возвращает число из узла
         * Если узел хранит int, значение будет приведено к double
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        double AsDouble() const;

        /**
         * Возвращает строку из узла
         * Выбрасывает исключение, если узел хранит значение другого типа
         */
        const std::string& AsString() const;

        /**
         * Возвращает хранимое значение variant
         */
        const variant& GetValue() const;

        /**
         * Сравнивает два узла на равенство
         */
        bool operator==(const Node& other) const;

        /**
         * Сравнивает два узла на неравенство
         */
        bool operator!=(const Node& other) const;
    };

    /**
     * JSON-документ
     */
    class Document {
    public:
        /**
         * Конструирует документ с корневым узлом root
         */
        explicit Document(Node root);

        /**
         * Возвращает корневой узел документа
         */
        const Node& GetRoot() const;

        /**
         * Сравнивает два документа на равенство
         */
        bool operator==(const Document& other) const;

        /**
         * Сравнивает два документа на неравенство
         */
        bool operator!=(const Document& other) const;

    private:
        Node root_;
    };

    /**
     * Загружает JSON-документ из потока input
     */
    Document Load(std::istream& input);

    /**
     * Печатает JSON-документ в поток output
     */
    void Print(const Document& doc, std::ostream& output);

}  // namespace json
