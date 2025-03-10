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

#import "MTRCertificates.h"
#import "CHIPError_Internal.h"
#import "CHIPOperationalCredentialsDelegate.h"
#import "CHIPP256KeypairBridge.h"
#import "MTRMemory.h"
#import "NSDataSpanConversion.h"

#include <credentials/CHIPCert.h>
#include <crypto/CHIPCryptoPAL.h>

using namespace chip;
using namespace chip::Crypto;
using namespace chip::Credentials;

@implementation MTRCertificates

+ (nullable NSData *)generateRootCertificate:(id<CHIPKeypair>)keypair
                                    issuerId:(nullable NSNumber *)issuerId
                                    fabricId:(nullable NSNumber *)fabricId
                                       error:(NSError * __autoreleasing *)error
{
    NSLog(@"Generating root certificate");

    [MTRMemory ensureInit];

    NSData * rootCert = nil;
    CHIP_ERROR err = CHIPOperationalCredentialsDelegate::GenerateRootCertificate(keypair, issuerId, fabricId, &rootCert);
    if (error) {
        *error = [CHIPError errorForCHIPErrorCode:err];
    }

    if (err != CHIP_NO_ERROR) {
        NSLog(@"Generating root certificate failed: %s", ErrorStr(err));
    }

    return rootCert;
}

+ (nullable NSData *)generateIntermediateCertificate:(id<CHIPKeypair>)rootKeypair
                                     rootCertificate:(NSData *)rootCertificate
                               intermediatePublicKey:(SecKeyRef)intermediatePublicKey
                                            issuerId:(nullable NSNumber *)issuerId
                                            fabricId:(nullable NSNumber *)fabricId
                                               error:(NSError * __autoreleasing *)error
{
    NSLog(@"Generating intermediate certificate");

    [MTRMemory ensureInit];

    NSData * intermediate = nil;
    CHIP_ERROR err = CHIPOperationalCredentialsDelegate::GenerateIntermediateCertificate(
        rootKeypair, rootCertificate, intermediatePublicKey, issuerId, fabricId, &intermediate);
    if (error) {
        *error = [CHIPError errorForCHIPErrorCode:err];
    }

    if (err != CHIP_NO_ERROR) {
        NSLog(@"Generating intermediate certificate failed: %s", ErrorStr(err));
    }

    return intermediate;
}

+ (nullable NSData *)generateOperationalCertificate:(id<CHIPKeypair>)signingKeypair
                                 signingCertificate:(NSData *)signingCertificate
                               operationalPublicKey:(SecKeyRef)operationalPublicKey
                                           fabricId:(NSNumber *)fabricId
                                             nodeId:(NSNumber *)nodeId
                              caseAuthenticatedTags:(NSArray<NSNumber *> * _Nullable)caseAuthenticatedTags
                                              error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
    NSLog(@"Generating operational certificate");

    [MTRMemory ensureInit];

    NSData * opcert = nil;
    CHIP_ERROR err = CHIPOperationalCredentialsDelegate::GenerateOperationalCertificate(
        signingKeypair, signingCertificate, operationalPublicKey, fabricId, nodeId, caseAuthenticatedTags, &opcert);
    if (error) {
        *error = [CHIPError errorForCHIPErrorCode:err];
    }

    if (err != CHIP_NO_ERROR) {
        NSLog(@"Generating operational certificate failed: %s", ErrorStr(err));
    }

    return opcert;
}

+ (BOOL)keypair:(id<CHIPKeypair>)keypair matchesCertificate:(NSData *)certificate
{
    [MTRMemory ensureInit];

    P256PublicKey keypairPubKey;
    CHIP_ERROR err = CHIPP256KeypairBridge::MatterPubKeyFromSecKeyRef(keypair.pubkey, &keypairPubKey);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract public key from keypair: %s", ErrorStr(err));
        return NO;
    }
    P256PublicKeySpan keypairKeySpan(keypairPubKey.ConstBytes());

    P256PublicKey certPubKey;
    err = ExtractPubkeyFromX509Cert(AsByteSpan(certificate), certPubKey);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract public key from certificate: %s", ErrorStr(err));
        return NO;
    }
    P256PublicKeySpan certKeySpan(certPubKey.ConstBytes());

    return certKeySpan.data_equal(keypairKeySpan);
}

+ (BOOL)isCertificate:(NSData *)certificate1 equalTo:(NSData *)certificate2
{
    [MTRMemory ensureInit];

    P256PublicKey pubKey1;
    CHIP_ERROR err = ExtractPubkeyFromX509Cert(AsByteSpan(certificate1), pubKey1);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract public key from first certificate: %s", ErrorStr(err));
        return NO;
    }
    P256PublicKeySpan keySpan1(pubKey1.ConstBytes());

    P256PublicKey pubKey2;
    err = ExtractPubkeyFromX509Cert(AsByteSpan(certificate2), pubKey2);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract public key from second certificate: %s", ErrorStr(err));
        return NO;
    }
    P256PublicKeySpan keySpan2(pubKey1.ConstBytes());

    if (!keySpan1.data_equal(keySpan2)) {
        return NO;
    }

    ChipDN subject1;
    err = ExtractSubjectDNFromX509Cert(AsByteSpan(certificate1), subject1);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract subject DN from first certificate: %s", ErrorStr(err));
        return NO;
    }

    ChipDN subject2;
    err = ExtractSubjectDNFromX509Cert(AsByteSpan(certificate2), subject2);
    if (err != CHIP_NO_ERROR) {
        NSLog(@"Can't extract subject DN from second certificate: %s", ErrorStr(err));
        return NO;
    }

    return subject1.IsEqual(subject2);
}

+ (nullable NSData *)generateCertificateSigningRequest:(id<CHIPKeypair>)keypair
                                                 error:(NSError * __autoreleasing _Nullable * _Nullable)error
{
    [MTRMemory ensureInit];

    CHIPP256KeypairBridge keypairBridge;
    CHIP_ERROR err = CHIP_NO_ERROR;
    do {
        err = keypairBridge.Init(keypair);
        if (err != CHIP_NO_ERROR) {
            break;
        }
        CHIPP256KeypairNativeBridge nativeKeypair(keypairBridge);

        uint8_t buf[kMAX_CSR_Length];
        MutableByteSpan csr(buf);
        err = GenerateCertificateSigningRequest(&nativeKeypair, csr);
        if (err != CHIP_NO_ERROR) {
            break;
        }

        return AsData(csr);
    } while (0);

    if (error) {
        *error = [CHIPError errorForCHIPErrorCode:err];
    }
    return nil;
}

@end
