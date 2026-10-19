#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace jsonx {

class Value {
public:
    enum class Kind { Null, Bool, Number, String, Array, Object };

    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    Value();
    Value(bool value);
    Value(double value);
    Value(int value);
    Value(const char* value);
    Value(std::string value);
    explicit Value(Array value);
    explicit Value(Object value);

    static Value MakeArray();
    static Value MakeObject();
    static Value Parse(const std::string& text, std::string* error = nullptr);

    Kind kind() const { return kind_; }
    bool IsNull() const { return kind_ == Kind::Null; }
    bool IsBool() const { return kind_ == Kind::Bool; }
    bool IsNumber() const { return kind_ == Kind::Number; }
    bool IsString() const { return kind_ == Kind::String; }
    bool IsArray() const { return kind_ == Kind::Array; }
    bool IsObject() const { return kind_ == Kind::Object; }

    bool AsBool(bool fallback = false) const;
    double AsNumber(double fallback = 0.0) const;
    int AsInt(int fallback = 0) const;
    const std::string& AsString() const;
    std::string AsString(const std::string& fallback) const;

    std::size_t size() const;
    bool empty() const;
    const Value& operator[](std::size_t index) const;
    const Value& operator[](const std::string& key) const;
    Value& operator[](const std::string& key);

    const Value* Find(const std::string& key) const;
    const Array& Items() const { return array_; }
    const Object& Members() const { return object_; }
    Array& ItemsMutable() { return array_; }
    Object& MembersMutable() { return object_; }

    void Push(Value value);
    void Set(std::string key, Value value);

    std::string Dump(int indent = 2) const;
    std::string DumpCompact() const;

private:
    Kind kind_ = Kind::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    Array array_;
    Object object_;

    void DumpTo(std::string& out, int indent, int depth) const;
};

}  // namespace jsonx
