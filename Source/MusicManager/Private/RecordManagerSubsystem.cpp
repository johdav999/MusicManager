#include "RecordManagerSubsystem.h"

#include "Misc/Guid.h"

FString URecordManagerSubsystem::CreateRecord(const FRecordData& Data)
{
    FString NewRecordId = Data.RecordId;
    if (NewRecordId.IsEmpty())
    {
        NewRecordId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    }

    FRecordData StoredRecord = Data;
    StoredRecord.RecordId = NewRecordId;

    Records.Add(NewRecordId, StoredRecord);

    return NewRecordId;
}

bool URecordManagerSubsystem::GetRecordById(const FString& RecordId, FRecordData& OutData) const
{
    if (const FRecordData* Found = Records.Find(RecordId))
    {
        OutData = *Found;
        return true;
    }

    return false;
}
