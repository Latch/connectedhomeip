/**
 *    Copyright (c) 2022 Project CHIP Authors
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

#import <CHIP/CHIP.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CHIPTestStorage : NSObject <CHIPPersistentStorageDelegate>
- (nullable NSData *)storageDataForKey:(NSString *)key;
- (BOOL)setStorageData:(NSData *)value forKey:(NSString *)key;
- (BOOL)removeStorageDataForKey:(NSString *)key;
@end

NS_ASSUME_NONNULL_END
