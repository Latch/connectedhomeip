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

#ifndef CHIP_DEVICE_CONTROLLER_H
#define CHIP_DEVICE_CONTROLLER_H

#import <Foundation/Foundation.h>

#import <CHIP/CHIPOnboardingPayloadParser.h>

@class CHIPDevice;

NS_ASSUME_NONNULL_BEGIN

typedef void (^CHIPDeviceConnectionCallback)(CHIPDevice * _Nullable device, NSError * _Nullable error);

@class CHIPCommissioningParameters;
@protocol CHIPDevicePairingDelegate;

@interface CHIPDeviceController : NSObject

@property (readonly, nonatomic) BOOL isRunning;

/**
 * Return the Node Id assigned to the controller.  Will return nil if the
 * controller is not running (and hence does not know its node id).
 */
@property (readonly, nonatomic, nullable) NSNumber * controllerNodeId;

/**
 * Start pairing for a device with the given ID, using the provided setup PIN
 * to establish a PASE connection.
 *
 * The IP and port for the device will be discovered automatically based on the
 * provided discriminator.
 *
 * The pairing process will proceed until a PASE session is established or an
 * error occurs, then notify onPairingComplete on the CHIPDevicePairingDelegate
 * for this controller.  That delegate is expected to call commissionDevice
 * after that point if it wants to commission the device.
 */
- (BOOL)pairDevice:(uint64_t)deviceID
     discriminator:(uint16_t)discriminator
      setupPINCode:(uint32_t)setupPINCode
             error:(NSError * __autoreleasing *)error;

/**
 * Start pairing for a device with the given ID, using the provided IP address
 * and port to connect to the device and the provided setup PIN to establish a
 * PASE connection.
 *
 * The pairing process will proceed until a PASE session is established or an
 * error occurs, then notify onPairingComplete on the CHIPDevicePairingDelegate
 * for this controller.  That delegate is expected to call commissionDevice
 * after that point if it wants to commission the device.
 */
- (BOOL)pairDevice:(uint64_t)deviceID
           address:(NSString *)address
              port:(uint16_t)port
      setupPINCode:(uint32_t)setupPINCode
             error:(NSError * __autoreleasing *)error;

/**
 * Start pairing for a device with the given ID and onboarding payload (QR code
 * or manual setup code).  The payload will be used to discover the device and
 * establish a PASE connection.
 *
 * The pairing process will proceed until a PASE session is established or an
 * error occurs, then notify onPairingComplete on the CHIPDevicePairingDelegate
 * for this controller.  That delegate is expected to call commissionDevice
 * after that point if it wants to commission the device.
 */
- (BOOL)pairDevice:(uint64_t)deviceID onboardingPayload:(NSString *)onboardingPayload error:(NSError * __autoreleasing *)error;
- (BOOL)commissionDevice:(uint64_t)deviceId
     commissioningParams:(CHIPCommissioningParameters *)commissioningParams
                   error:(NSError * __autoreleasing *)error;

- (BOOL)continueCommissioningDevice:(void *)device
           ignoreAttestationFailure:(BOOL)ignoreAttestationFailure
                              error:(NSError * __autoreleasing *)error;

- (BOOL)stopDevicePairing:(uint64_t)deviceID error:(NSError * __autoreleasing *)error;

- (nullable CHIPDevice *)getDeviceBeingCommissioned:(uint64_t)deviceId error:(NSError * __autoreleasing *)error;
- (BOOL)getConnectedDevice:(uint64_t)deviceID
                     queue:(dispatch_queue_t)queue
         completionHandler:(CHIPDeviceConnectionCallback)completionHandler;

- (BOOL)openPairingWindow:(uint64_t)deviceID duration:(NSUInteger)duration error:(NSError * __autoreleasing *)error;
- (nullable NSString *)openPairingWindowWithPIN:(uint64_t)deviceID
                                       duration:(NSUInteger)duration
                                  discriminator:(NSUInteger)discriminator
                                       setupPIN:(NSUInteger)setupPIN
                                          error:(NSError * __autoreleasing *)error;

/**
 * Temporary until PairingDelegate is fixed to clearly communicate this
 * information to consumers.
 */
- (BOOL)deviceBeingCommissionedOverBLE:(uint64_t)deviceId;

/**
 * Controllers are created via the MatterControllerFactory object.
 */
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/**
 * Set the Delegate for the Device Pairing  as well as the Queue on which the Delegate callbacks will be triggered
 *
 * @param[in] delegate The delegate the pairing process should use
 *
 * @param[in] queue The queue on which the callbacks will be delivered
 */
- (void)setPairingDelegate:(id<CHIPDevicePairingDelegate>)delegate queue:(dispatch_queue_t)queue;

/**
 * Shutdown the controller. Calls to shutdown after the first one are NO-OPs.
 */
- (void)shutdown;

@end

NS_ASSUME_NONNULL_END

#endif /* CHIP_DEVICE_CONTROLLER_H */
