#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <memory>

namespace qb {

enum struct RecordValueType {
    None,
    Int32,
    Int64,
    String,

    SENTINEL
};

struct RecordValue {
    virtual bool fromStr(std::string_view s) = 0;
    virtual std::string toStr() const = 0;
};

struct StrRecordValue : public RecordValue {
    std::string value;

    StrRecordValue() = default;
    StrRecordValue(StrRecordValue&&) = default;
    StrRecordValue(const StrRecordValue&) = default;
    ~StrRecordValue() = default;

    StrRecordValue(const std::string& s) : value(s) {}
    StrRecordValue(std::string&& s) : value(std::move(s)) {}

    bool fromStr(std::string_view s) override {
        value = s;
        return true;
    }

    std::string toStr() const override {
        return value;
    }
};

struct Int32RecordValue : public RecordValue {
    int32_t value;

    Int32RecordValue() = default;
    Int32RecordValue(Int32RecordValue&&) = default;
    Int32RecordValue(const Int32RecordValue&) = default;
    ~Int32RecordValue() = default;

    Int32RecordValue(int32_t v) : value(v) {}

    bool fromStr(std::string_view s) override {
        char* pEnd = nullptr;
        value = std::strtol(s.data(), &pEnd, 10);
        return *pEnd == 0;
    }

    std::string toStr() const override {
        return std::to_string(value);
    }
};

struct Int64RecordValue : public RecordValue {
    int64_t value;

    Int64RecordValue() = default;
    Int64RecordValue(Int64RecordValue&&) = default;
    Int64RecordValue(const Int64RecordValue&) = default;
    ~Int64RecordValue() = default;

    Int64RecordValue(int64_t v) : value(v) {}

    bool fromStr(std::string_view s) override {
        char* pEnd = nullptr;
        value = std::strtoll(s.data(), &pEnd, 10);
        return *pEnd == 0;
    }

    std::string toStr() const override {
        return std::to_string(value);
    }
};

template <size_t N>
struct Record {
    using IdType = uint32_t;
    static constexpr size_t RecordsCount = N;

    std::array<std::unique_ptr<RecordValue>, RecordsCount> columns;
};

template <typename TRecord>
struct Collection {
    using ColumnNamesType = std::array<std::string, TRecord::RecordsCount>;
    using RecordsType = std::unordered_map<typename TRecord::IdType, TRecord>;
    
    ColumnNamesType columnNames;
    RecordsType records;

    void reserve(size_t n) {
        records.reserve(n);
    }

    void insert(typename TRecord::IdType id, TRecord&& record) {
        records[id] = std::move(record);
    }

    bool empty() const { return records.empty(); }

    RecordsType::const_iterator begin() const { return records.begin(); }
    RecordsType::const_iterator end() const { return records.end(); }
};

} // qb
