#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <memory>
#include <type_traits>

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
        return core::toInt32(s.data(), value);
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
        return core::toInt64(s.data(), value);
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

struct Column {
    std::string_view name;
    RecordValueType type;
    int32_t index;
};

template <size_t RecordSize>
struct Collection {
    static_assert(RecordSize > 0, "RecordSize must be greater than 0");

    using RecordType = Record<RecordSize>;
    using ColumnsType = std::unordered_map<std::string, Column>;
    using ColumnNamesType = std::array<std::string, RecordSize>;

    using RecordsMapType = std::unordered_map<typename RecordType::IdType, RecordType>;
    using StrIndices = std::unordered_map<std::string, typename RecordType::IdType>;
    using Int64Indices = std::unordered_map<int64_t, typename RecordType::IdType>;

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

    RecordsMapType::const_iterator begin() const { return m_records.begin(); }
    RecordsMapType::const_iterator end() const { return m_records.end(); }

    bool insertRecord(RecordType&& record) {
        bool ok = true;

        Int32RecordValue* idRecord = dynamic_cast<Int32RecordValue*>(value.get());
        if (!idRecord) {
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
                    strIndices.insert(std::make_pair(strRecord->value, id));
                    ok = true;
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
                    int64Indices.insert(std::make_pair(int64Record->value, id));
                    ok = true;
                }
            }
        }

        if (ok) {
            // Insert record
            m_records.insert(std::make_pair(id, std::move(record)));
        }

        return ok;
    }

    Collection<RecordSize> match(const std::string& columnName, const std::string& matchString) {
        ColumnNamesType namesCopy = m_columnNames;
        Collection<RecordSize> res (std::move(namesCopy));

        bool isTheIdColumn = columnName == m_columnNames[0];

        if (isTheIdColumn) {
            bool ok = core::toInt32(matchString.data(), id);
            if (!ok) return res;

            auto it = m_records.find(id);
            if (it != m_records.end()) {
                // When matching the id column there is only one record to return
                auto recCpy = it->second;
                res.insertRecord(recCpy);
            }

            return res;
        }

        auto columnIt = m_columns.find(columnName);
        if (columnIt == m_columns.end()) {
            return res;
        }

        auto& column = columnIt->second;
        if (column.index == -1) {
            // No index set for this column
            return res;
        }

        if (column.type == RecordValueType::String) {
            if (column.index >= m_strIndices.size()) {
                return res;
            }

            StrIndices& strIndices = m_strIndices[column.index];
            auto range = strIndices.equal_range(matchString);
            for (auto it = range.first; it != range.second; ++it) {
                auto recIt = m_records.find(it->second);
                if (recIt != m_records.end()) {
                    auto recCpy = recIt->second;
                    res.insertRecord(recCpy);
                }
            }
        }
        else if (column.type == RecordValueType::Int64) {
            if (column.index >= m_int64Indices.size()) {
                return res;
            }

            Int64Indices& int64Indices = m_int64Indices[column.index];
            auto range = int64Indices.equal_range(core::toInt64(matchString.data()));
            for (auto it = range.first; it != range.second; ++it) {
                auto recIt = m_records.find(it->second);
                if (recIt != m_records.end()) {
                    auto recCpy = recIt->second;
                    res.insertRecord(recCpy);
                }
            }
        }

        return res;
    }

public: // TODO: make private
    ColumnsType m_columns;
    ColumnNamesType m_columnNames;
    RecordsMapType m_records;
    std::vector<StrIndices> m_strIndices;
    std::vector<Int64Indices> m_int64Indices;
};

} // qb
