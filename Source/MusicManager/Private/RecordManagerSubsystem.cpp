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

const FRecordData* URecordManagerSubsystem::GetRecordById(const FString& RecordId) const
{
    if (const FRecordData* const* Found = Records.Find(RecordId))
    {
        return *Found;
    }

    return nullptr;
}
