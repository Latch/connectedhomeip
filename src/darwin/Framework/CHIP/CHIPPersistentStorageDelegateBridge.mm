/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#import "CHIPPersistentStorageDelegateBridge.h"

CHIPPersistentStorageDelegateBridge::CHIPPersistentStorageDelegateBridge(id<CHIPPersistentStorageDelegate> delegate)
    : mDelegate(delegate)
    , mWorkQueue(dispatch_queue_create("com.csa.matter.framework.storage.workqueue", DISPATCH_QUEUE_SERIAL))
{
}

CHIPPersistentStorageDelegateBridge::~CHIPPersistentStorageDelegateBridge(void) {}

CHIP_ERROR CHIPPersistentStorageDelegateBridge::SyncGetKeyValue(const char * key, void * buffer, uint16_t & size)
{
    if (buffer == nullptr && size != 0) {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    __block CHIP_ERROR error = CHIP_NO_ERROR;
    NSString * keyString = [NSString stringWithUTF8String:key];

    dispatch_sync(mWorkQueue, ^{
        NSLog(@"PersistentStorageDelegate Sync Get Value for Key: %@", keyString);

        NSData * value = [mDelegate storageDataForKey:keyString];

        if (value == nil) {
            error = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
            return;
        }

        if ([value length] > UINT16_MAX) {
            error = CHIP_ERROR_BUFFER_TOO_SMALL;
            size = 0;
            return;
        }

        uint16_t valueSize = static_cast<uint16_t>([value length]);
        if (valueSize > size) {
            error = CHIP_ERROR_BUFFER_TOO_SMALL;
            size = valueSize;
            return;
        }

        size = valueSize;
        if (size != 0) {
            // buffer is known to be non-null here.
            memcpy(buffer, [value bytes], size);
        }
    });

    return error;
}

CHIP_ERROR CHIPPersistentStorageDelegateBridge::SyncSetKeyValue(const char * key, const void * value, uint16_t size)
{
    if (value == nullptr && size != 0) {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    NSString * keyString = [NSString stringWithUTF8String:key];
    NSData * valueData = (value == nullptr) ? [NSData data] : [NSData dataWithBytes:value length:size];

    __block CHIP_ERROR error = CHIP_NO_ERROR;
    dispatch_sync(mWorkQueue, ^{
        NSLog(@"PersistentStorageDelegate Set Key %@", keyString);

        if ([mDelegate setStorageData:valueData forKey:keyString] == NO) {
            error = CHIP_ERROR_PERSISTED_STORAGE_FAILED;
        }
    });

    return error;
}

CHIP_ERROR CHIPPersistentStorageDelegateBridge::SyncDeleteKeyValue(const char * key)
{
    NSString * keyString = [NSString stringWithUTF8String:key];

    __block CHIP_ERROR error = CHIP_NO_ERROR;
    dispatch_sync(mWorkQueue, ^{
        NSLog(@"PersistentStorageDelegate Delete Key: %@", keyString);

        if ([mDelegate removeStorageDataForKey:keyString] == NO) {
            error = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
        }
    });

    return error;
}
