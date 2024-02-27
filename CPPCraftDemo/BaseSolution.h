#pragma once

#include <string>
#include <vector>

namespace base_impl {

/**
    Represents a Record Object
*/
struct QBRecord
{
    uint32_t column0; // unique id column
    std::string column1;
    int64_t column2;
    std::string column3;
};

inline bool operator==(const QBRecord& lhs, const QBRecord& rhs) {
    return lhs.column0 == rhs.column0 && 
            lhs.column1 == rhs.column1 && 
            lhs.column2 == rhs.column2 && 
            lhs.column3 == rhs.column3;
}

/**
    Represents a Record Collections
*/
typedef std::vector<QBRecord> QBRecordCollection;

/**
    Return records that contains a string in the StringValue field
    records - the initial set of records to filter
    matchString - the string to search for
*/
QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString);

/**
    Utility to populate a record collection
    prefix - prefix for the string value for every record
    numRecords - number of records to populate in the collection
*/
QBRecordCollection populateDummyData(const std::string& prefix, int numRecords);

} // namespace base_impl
