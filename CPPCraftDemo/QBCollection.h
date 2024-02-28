#pragma once

#include <assert.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <memory>
#include <type_traits>
#include <stdexcept>

#ifdef _DEBUG
#include <iostream>
#endif

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
    virtual std::unique_ptr<RecordValue> copy() const = 0;
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

    std::unique_ptr<RecordValue> copy() const override {
        return std::make_unique<StrRecordValue>(value);
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
        return core::toInt32(s.data(), value);
    }

    std::string toStr() const override {
        return std::to_string(value);
    }

    std::unique_ptr<RecordValue> copy() const override {
        return std::make_unique<Int32RecordValue>(value);
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
        return core::toInt64(s.data(), value);
    }

    std::string toStr() const override {
        return std::to_string(value);
    }

    std::unique_ptr<RecordValue> copy() const override {
        return std::make_unique<Int64RecordValue>(value);
    }
};

template <size_t N>
struct Record {
    using IdType = uint32_t;
    static constexpr size_t RecordsCount = N;

    std::array<std::unique_ptr<RecordValue>, RecordsCount> columns;

    Record copy() const {
        Record res;
        for (size_t i = 0; i < RecordsCount; i++) {
            res.columns[i] = columns[i]->copy();
        }
        return res;
    }
};

struct Column {
    std::string_view name;
    RecordValueType type;
    int32_t index;

    Column() : name({}), type(RecordValueType::None), index(-1) {}
    Column(std::string_view s, RecordValueType t, int32_t i) : name(s), type(t), index(i) {}
};

template <size_t RecordSize>
struct Collection {
    static_assert(RecordSize > 0, "RecordSize must be greater than 0");

    using RecordType = Record<RecordSize>;
    using ColumnsType = std::unordered_map<std::string, Column>;
    using ColumnNamesType = std::array<std::string, RecordSize>;

    using RecordsMapType = std::unordered_map<typename RecordType::IdType, RecordType>;
    using StrIndices = std::unordered_map<std::string, std::vector<typename RecordType::IdType>>;
    using Int64Indices = std::unordered_map<int64_t, std::vector<typename RecordType::IdType>>;

    Collection(std::array<std::string, RecordSize>&& columnNames) {
        if (columnNames.size() != RecordSize) {
            throw std::invalid_argument("Invalid column names size");
        }

        m_columnNames = std::move(columnNames);
        for (size_t i = 0; i < RecordSize; i++) {
            m_columns.insert(std::make_pair(m_columnNames[i], Column{ m_columnNames[i], RecordValueType::None, -1 }));
        }
    }

    bool createIndex(const std::string& columnName, RecordValueType type) {
        auto it = m_columns.find(columnName);

        bool ok = false;
        if (it != m_columns.end()) {
            int32_t index = 0;
            it->second.type = type;

            switch (type) {
                case RecordValueType::String:
                    index = static_cast<int32_t>(m_strIndices.size());
                    m_strIndices.push_back(StrIndices());
                    ok = true;
                    break;
                case RecordValueType::Int64:
                    index = static_cast<int32_t>(m_int64Indices.size());
                    m_int64Indices.push_back(Int64Indices());
                    ok = true;
                    break;
                default:
                    break;
            }

            it->second.index = index;
        }

        return ok;
    }

    void reserve(size_t n) {
        m_records.reserve(n);
    }

    bool empty() const { return m_records.empty(); }

    size_t size() const { return m_records.size(); }

    RecordsMapType::const_iterator begin() const { return m_records.begin(); }
    RecordsMapType::const_iterator end() const { return m_records.end(); }

    bool insertRecord(RecordType&& record) {
        bool ok = true;

        if (record.columns.size() < 1) {
            ok = false;
            return ok;
        }

        Int32RecordValue* idRecord = dynamic_cast<Int32RecordValue*>(record.columns[0].get());
        if (!idRecord) {
            // The first column must be the recrod id!
            return false;
        }
        auto id = idRecord->value;

        // Create indices for each column
        for (size_t i = 1; i < RecordSize; i++) {
            auto& columnName = m_columnNames[i];
            auto columnIt = m_columns.find(columnName);
            if (columnIt == m_columns.end()) {
                ok = false;
                break;
            }
            auto& column = columnIt->second;
            auto& value = record.columns[i];

            if (column.index == -1) {
                // No index set for this column
                continue;
            }

            if (column.type == RecordValueType::String) {
                if (column.index >= m_strIndices.size()) {
                    ok = false;
                    break;
                }

                StrIndices& strIndices = m_strIndices[column.index];
                StrRecordValue* strRecord = dynamic_cast<StrRecordValue*>(value.get());
                if (strRecord) {
                    auto it = strIndices.find(strRecord->value);
                    if (it == strIndices.end()) {
                        it = strIndices.insert({ strRecord->value, { uint32_t(id) } }).first;
                    }

                    it->second.push_back(id);
                    ok = true;
                }
                else {
                    ok = false;
                    break;
                }
            }
            else if (column.type == RecordValueType::Int64) {
                if (column.index >= m_int64Indices.size()) {
                    ok = false;
                    break;
                }

                Int64Indices& int64Indices = m_int64Indices[column.index];
                Int64RecordValue* int64Record = dynamic_cast<Int64RecordValue*>(value.get());
                if (int64Record) {
                    auto it = int64Indices.find(int64Record->value);
                    if (it == int64Indices.end()) {
                        it = int64Indices.insert({ int64Record->value, { uint32_t(id) } }).first;
                    }

                    it->second.push_back(id);
                    ok = true;
                }
                else {
                    ok = false;
                    break;
                }
            }
            else {
                ok = false;
                break;
            }
        }

        if (ok) {
            // Insert record
            m_records.insert(std::make_pair(id, std::move(record)));
        }

        return ok;
    }

    Collection<RecordSize> match(const std::string& columnName, const std::string& matchString, bool& ok) const {
        ColumnNamesType namesCopy = m_columnNames;
        Collection<RecordSize> res (std::move(namesCopy));

        bool isTheIdColumn = columnName == m_columnNames[0];

        if (isTheIdColumn) {
            int32_t id = 0;
            ok = core::toInt32(matchString.data(), id);
            if (!ok) return res;

            auto it = m_records.find(id);
            if (it != m_records.end()) {
                // When matching the id column there is only one record to return
                auto recCpy = it->second.copy();
                res.insertRecord(std::move(recCpy));
            }

            ok = true;
            return res;
        }

        auto columnIt = m_columns.find(columnName);
        if (columnIt == m_columns.end()) {
            ok = false;
            return res;
        }

        auto& column = columnIt->second;
        if (column.index == -1) {
            // No index set for this column
            ok = false;
            return res;
        }

        auto insertRangeFromIndex = [&](const auto& indices, const auto& val) {
            auto range = indices.equal_range(val);
            for (auto it = range.first; it != range.second; it++) {
                for (auto& id : it->second) {
                    auto recIt = m_records.find(id);
                    if (recIt != m_records.end()) {
                        auto recCpy = recIt->second.copy();
                        res.insertRecord(std::move(recCpy));
                    }
                }
            }
        };

        if (column.type == RecordValueType::String) {
            if (column.index >= m_strIndices.size()) {
                ok = false;
                return res;
            }

            const auto& strIndices = m_strIndices[column.index];
            insertRangeFromIndex(strIndices, matchString);
        }
        else if (column.type == RecordValueType::Int64) {
            if (column.index >= m_int64Indices.size()) {
                ok = false;
                return res;
            }

            int64_t v;
            ok = core::toInt64(matchString.data(), v);
            if (!ok) return res;

            const auto& int64Indices = m_int64Indices[column.index];
            insertRangeFromIndex(int64Indices, v);
        }

        ok = true;
        return res;
    }

    void remove(typename RecordType::IdType id) {
        auto it = m_records.find(id);
        if (it != m_records.end()) {
            for (size_t i = 1; i < RecordSize; i++) {
                auto& columnName = m_columnNames[i];
                auto columnIt = m_columns.find(columnName);
                if (columnIt == m_columns.end()) {
                    continue;
                }
                auto& column = columnIt->second;
                auto& value = it->second.columns[i];

                if (column.index == -1) {
                    // No index set for this column
                    continue;
                }

                // Remove from indices

                auto removeIdFromIndex = [&](auto& indices, auto& val) {
                    auto it = indices.find(val);
                    if (it != indices.end()) {
                        auto& vec = it->second;
                        vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
                    }
                };

                if (column.type == RecordValueType::String) {
                    if (column.index >= m_strIndices.size()) {
                        continue;
                    }

                    StrIndices& strIndices = m_strIndices[column.index];
                    StrRecordValue* strRecord = dynamic_cast<StrRecordValue*>(value.get());
                    if (strRecord) {
                        removeIdFromIndex(strIndices, strRecord->value);
                    }
                }
                else if (column.type == RecordValueType::Int64) {
                    if (column.index >= m_int64Indices.size()) {
                        continue;
                    }

                    Int64Indices& int64Indices = m_int64Indices[column.index];
                    Int64RecordValue* int64Record = dynamic_cast<Int64RecordValue*>(value.get());
                    if (int64Record) {
                        removeIdFromIndex(int64Indices, int64Record->value);
                    }
                }
            }

            m_records.erase(it);
        }
    }

#ifdef _DEBUG

    void debug_PrintCollection(bool printIndices = false) const {
       std::cout << "Records: " << std::endl;
        for (auto& rec : m_records) {
            std::cout << "\t{ " ;
            for (size_t i = 0; i < RecordSize; i++) {
                std::cout << m_columnNames[i] << ": " << rec.second.columns[i]->toStr() << ", ";
            }
            std::cout << "}" << std::endl;
        }

        if (printIndices) {
            std::cout << "String indices: " << std::endl;
            for (size_t i = 0; i < m_strIndices.size(); i++) {
                std::cout << "\t{ ";
                for (auto& idx : m_strIndices[i]) {
                    std::cout << idx.first << ": { ";
                    for (auto& id : idx.second) {
                        std::cout << id << ", ";
                    }
                    std::cout << "}, ";
                }
                std::cout << "} " << std::endl;
            }
        }
        else {
            std::cout << "String indices: " << m_strIndices.size() << std::endl;
        }

        if (printIndices) {
            std::cout << "Int64 indices: " << std::endl;
            for (size_t i = 0; i < m_int64Indices.size(); i++) {
                std::cout << "\t{ ";
                for (auto& idx : m_int64Indices[i]) {
                    std::cout << idx.first << ": { ";
                    for (auto& id : idx.second) {
                        std::cout << id << ", ";
                    }
                    std::cout << "}, ";
                }
                std::cout << "} " << std::endl;
            }
        }
        else {
            std::cout << "Int64 indices: " << m_int64Indices.size() << std::endl;
        }

        std::cout << std::endl;
    }

#endif

private:
    ColumnsType m_columns;
    ColumnNamesType m_columnNames;
    RecordsMapType m_records;
    std::vector<StrIndices> m_strIndices;
    std::vector<Int64Indices> m_int64Indices;
};

using QBRecordCollection = qb::Collection<4>;

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString);
void DeleteRecordByID(QBRecordCollection& records, uint32_t id);

} // qb
