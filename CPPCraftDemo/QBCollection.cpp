#include "stdafx.h"

namespace qb {

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString) {
    bool ok = false;
    QBRecordCollection ret = records.match(columnName, matchString, ok);
    assert(ok);
    return ret;
}

void DeleteRecordByID(QBRecordCollection& records, uint32_t id) {
    records.remove(id);
}

} // namespace qb