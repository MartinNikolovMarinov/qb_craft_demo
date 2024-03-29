#include "stdafx.h"

#include <algorithm>
#include <iterator>

namespace base_impl {

/**
    Return records that contains a string in the StringValue field
    records - the initial set of records to filter
    matchString - the string to search for
*/
QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString) {
    QBRecordCollection result;

    std::copy_if(records.begin(), records.end(), std::back_inserter(result), [&](QBRecord rec) {
        if (columnName == "column0") {
            uint32_t matchValue = std::stoul(matchString);
            return matchValue == rec.column0;
        }
        else if (columnName == "column1") {
            return rec.column1.find(matchString) != std::string::npos;
        }
        else if (columnName == "column2") {
            int64_t matchValue = std::stol(matchString);
            return matchValue == rec.column2;
        }
        else if (columnName == "column3") {
            return rec.column3.find(matchString) != std::string::npos;
        }
        else {
            return false;
        }
    });

    return result;
}

/**
    Utility to populate a record collection
    prefix - prefix for the string value for every record
    numRecords - number of records to populate in the collection
*/
QBRecordCollection populateDummyData(const std::string& prefix, int numRecords) {
    QBRecordCollection data;
    data.reserve(numRecords);
    for (uint32_t i = 0; i < uint32_t(numRecords); i++) {
        QBRecord rec = QBRecord{ i, prefix + std::to_string(i), i % 100, std::to_string(i) + prefix };
        data.emplace_back(rec);
    }
    return data;
}

} // namespace base_impl
