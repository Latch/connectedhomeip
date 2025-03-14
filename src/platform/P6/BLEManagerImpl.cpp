/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    Copyright (c) 2020 Nest Labs, Inc.
 *    All rights reserved.
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

/**
 *    @file
 *          Provides an implementation of the BLEManager singleton object
 *          for the PSoC6 platform.
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#include <ble/CHIPBleServiceData.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/internal/BLEManager.h>

extern "C" {
#include "app_platform_cfg.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gatt_db.h"
}

#include "cy_utils.h"
#include "wiced_bt_stack.h"

#include <wiced_bt_ble.h>
#include <wiced_bt_gatt.h>

using namespace ::chip;
using namespace ::chip::Ble;

#define BLE_SERVICE_DATA_SIZE 10

namespace chip {
namespace DeviceLayer {
namespace Internal {

namespace {
const ChipBleUUID chipUUID_CHIPoBLEChar_RX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x11 } };

const ChipBleUUID ChipUUID_CHIPoBLEChar_TX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x12 } };

} // unnamed namespace

BLEManagerImpl BLEManagerImpl::sInstance;

wiced_bt_gatt_status_t app_gatts_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t * p_data);

wiced_result_t BLEManagerImpl::BLEManagerCallback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t * p_event_data)
{
    switch (event)
    {
    case BTM_ENABLED_EVT:
        // Post a event to _OnPlatformEvent.
        {
            // Register with stack to receive GATT callback
            wiced_bt_gatt_register(app_gatts_callback);

            // Inform the stack to use this app GATT database
            wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);

            ChipDeviceEvent bleEvent;
            bleEvent.Type = DeviceEventType::kP6BLEEnabledEvt;
            if (PlatformMgr().PostEvent(&bleEvent) != CHIP_NO_ERROR)
            {
                return WICED_BT_ERROR;
            }
        }
        break;
    }

    return WICED_BT_SUCCESS;
}

CHIP_ERROR BLEManagerImpl::_Init()
{
    CHIP_ERROR err;

    // Initialize the CHIP BleLayer.
    err = BleLayer::Init(this, this, &DeviceLayer::SystemLayer());
    SuccessOrExit(err);

    // Configure platform specific settings for Bluetooth
    cybt_platform_config_init(&bt_platform_cfg_settings);

    // Initialize the Bluetooth stack with a callback function and stack
    // configuration structure */
    if (WICED_SUCCESS != wiced_bt_stack_init(BLEManagerCallback, &wiced_bt_cfg_settings))
    {
        printf("Error initializing BT stack\n");
        CY_ASSERT(0);
    }

    mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Enabled;
    if (CHIP_DEVICE_CONFIG_CHIPOBLE_ENABLE_ADVERTISING_AUTOSTART)
    {
        mFlags.Set(Flags::kFlag_AdvertisingEnabled, true);
    }
    else
    {
        mFlags.Set(Flags::kFlag_AdvertisingEnabled, false);
    }
    mNumCons = 0;
    memset(mCons, 0, sizeof(mCons));
    memset(mDeviceName, 0, sizeof(mDeviceName));

    ChipLogProgress(DeviceLayer, "BLEManagerImpl::Init() complete");

    PlatformMgr().ScheduleWork(DriveBLEState, 0);

exit:
    return err;
}

CHIP_ERROR BLEManagerImpl::_SetCHIPoBLEServiceMode(CHIPoBLEServiceMode val)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrExit(val != ConnectivityManager::kCHIPoBLEServiceMode_NotSupported, err = CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(mServiceMode != ConnectivityManager::kCHIPoBLEServiceMode_NotSupported, err = CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE);

    if (val != mServiceMode)
    {
        mServiceMode = val;
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }

exit:
    return err;
}

bool BLEManagerImpl::_IsAdvertisingEnabled(void)
{
    return mFlags.Has(Flags::kFlag_AdvertisingEnabled);
}

CHIP_ERROR BLEManagerImpl::_SetAdvertisingEnabled(bool val)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrExit(mServiceMode != ConnectivityManager::kCHIPoBLEServiceMode_NotSupported, err = CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE);

    if (mFlags.Has(Flags::kFlag_AdvertisingEnabled) != val)
    {
        mFlags.Set(Flags::kFlag_AdvertisingEnabled, val);
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }

exit:
    return err;
}

/*
 * TODO
 */
CHIP_ERROR BLEManagerImpl::_SetAdvertisingMode(BLEAdvertisingMode mode)
{
    (void) (mode);

    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR BLEManagerImpl::_GetDeviceName(char * buf, size_t bufSize)
{
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_NotSupported)
    {
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    }
    if (strlen(mDeviceName) >= bufSize)
    {
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    strcpy(buf, mDeviceName);
    ChipLogProgress(DeviceLayer, "Getting device name to : \"%s\"", mDeviceName);
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::_SetDeviceName(const char * deviceName)
{
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_NotSupported)
    {
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    }
    if (deviceName != NULL && deviceName[0] != 0)
    {
        if (strlen(deviceName) >= kMaxDeviceNameLength)
        {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        memset(mDeviceName, 0, kMaxDeviceNameLength);
        strncpy(mDeviceName, deviceName, strlen(deviceName));
        mFlags.Set(Flags::kFlag_DeviceNameSet, true);
        ChipLogProgress(DeviceLayer, "Setting device name to : \"%s\"", deviceName);
    }
    else
    {
        wiced_bt_cfg_settings.device_name[0] = 0;
        mDeviceName[0]                       = 0;
        mFlags.Set(Flags::kFlag_DeviceNameSet, false);
    }

    return CHIP_NO_ERROR;
}

uint16_t BLEManagerImpl::_NumConnections(void)
{
    return mNumCons;
}

void BLEManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    switch (event->Type)
    {

    case DeviceEventType::kP6BLEEnabledEvt:
        mFlags.Set(Flags::kFlag_StackInitialized, true);
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
        break;

    case DeviceEventType::kCHIPoBLESubscribe:
        HandleSubscribeReceived(event->CHIPoBLESubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        {
            ChipDeviceEvent _event;
            _event.Type = DeviceEventType::kCHIPoBLEConnectionEstablished;
            PlatformMgr().PostEventOrDie(&_event);
        }
        break;

    case DeviceEventType::kCHIPoBLEUnsubscribe:
        HandleUnsubscribeReceived(event->CHIPoBLEUnsubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        break;

    case DeviceEventType::kCHIPoBLEWriteReceived:
        HandleWriteReceived(event->CHIPoBLEWriteReceived.ConId, &CHIP_BLE_SVC_ID, &chipUUID_CHIPoBLEChar_RX,
                            PacketBufferHandle::Adopt(event->CHIPoBLEWriteReceived.Data));
        break;

    case DeviceEventType::kCHIPoBLENotifyConfirm:
        HandleIndicationConfirmation(event->CHIPoBLENotifyConfirm.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        break;

    case DeviceEventType::kCHIPoBLEConnectionError:
        HandleConnectionError(event->CHIPoBLEConnectionError.ConId, event->CHIPoBLEConnectionError.Reason);
        break;

    case DeviceEventType::kServiceProvisioningChange:
    case DeviceEventType::kAccountPairingChange:

        // If CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED is enabled, and there is a change to the
        // device's provisioning state, then automatically disable CHIPoBLE advertising if the device
        // is now fully provisioned.
#if CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
        if (ConfigurationMgr().IsFullyProvisioned())
        {
            ClearFlag(mFlags, kFlag_AdvertisingEnabled);
            ChipLogProgress(DeviceLayer, "CHIPoBLE advertising disabled because device is fully provisioned");
        }
#endif // CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED

        // Force the advertising state to be refreshed to reflect new provisioning state.
        mFlags.Set(Flags::kFlag_AdvertisingRefreshNeeded, true);

        DriveBLEState();
        break;

    default:
        break;
    }
}

bool BLEManagerImpl::SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::UnsubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::CloseConnection(BLE_CONNECTION_OBJECT conId)
{
    ChipLogProgress(DeviceLayer, "Closing BLE GATT connection (con %u)", conId);

    // Initiate a GAP disconnect.
    wiced_bt_gatt_status_t gatt_err = wiced_bt_gatt_disconnect((uint16_t) conId);
    if (gatt_err != WICED_BT_GATT_SUCCESS)
    {
        ChipLogError(DeviceLayer, "wiced_bt_gatt_disconnect() failed: %ld", gatt_err);
    }

    return (gatt_err == WICED_BT_GATT_SUCCESS);
}

uint16_t BLEManagerImpl::GetMTU(BLE_CONNECTION_OBJECT conId) const
{
    CHIPoBLEConState * p_conn;

    /* Check if target connection state exists. */
    p_conn = BLEManagerImpl::sInstance.GetConnectionState(conId);

    if (!p_conn)
    {
        return wiced_bt_cfg_settings.gatt_cfg.max_mtu_size;
    }
    else
    {
        return p_conn->Mtu;
    }
}

bool BLEManagerImpl::SendIndication(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                    PacketBufferHandle data)
{
    CHIP_ERROR err                  = CHIP_NO_ERROR;
    uint16_t dataLen                = data->DataLength();
    wiced_bt_gatt_status_t gatt_err = WICED_BT_GATT_SUCCESS;
    CHIPoBLEConState * conState     = GetConnectionState(conId);

    VerifyOrExit(conState != NULL, err = CHIP_ERROR_INVALID_ARGUMENT);

#ifdef BLE_DEBUG
    ChipLogDetail(DeviceLayer, "Sending notification for CHIPoBLE TX characteristic (con %u, len %u)", conId, dataLen);
#endif

    // Send a notification for the CHIPoBLE TX characteristic to the client containing the supplied data.
    gatt_err = wiced_bt_gatt_send_notification((uint16_t) conId, HDLC_CHIP_SERVICE_CHAR_C2_VALUE, dataLen, data->Start());

exit:
    if (gatt_err != WICED_BT_GATT_SUCCESS)
    {
        ChipLogError(DeviceLayer, "BLEManagerImpl::SendNotification() failed: %ld", gatt_err);
        return false;
    }
    else
    {
        // Post an event to the CHIP queue.
        ChipDeviceEvent event;
        event.Type                        = DeviceEventType::kCHIPoBLENotifyConfirm;
        event.CHIPoBLENotifyConfirm.ConId = conId;
        err                               = PlatformMgr().PostEvent(&event);
    }
    return err == CHIP_NO_ERROR;
}

bool BLEManagerImpl::SendWriteRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                      PacketBufferHandle data)

{
    ChipLogError(DeviceLayer, "BLEManagerImpl::SendWriteRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                     PacketBufferHandle data)
{
    ChipLogError(DeviceLayer, "BLEManagerImpl::SendReadRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext,
                                      const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogError(DeviceLayer, "BLEManagerImpl::SendReadResponse() not supported");
    return false;
}

void BLEManagerImpl::NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId) {}

void BLEManagerImpl::DriveBLEState(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    // Exit if Stack not initialized
    VerifyOrExit(mFlags.Has(Flags::kFlag_StackInitialized), /* */);

    // Perform any initialization actions that must occur after the CHIP task is running.
    if (!mFlags.Has(Flags::kFlag_AsyncInitCompleted))
    {
        mFlags.Set(Flags::kFlag_AsyncInitCompleted, true);

        // If CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED is enabled,
        // disable CHIPoBLE advertising if the device is fully provisioned.
#if CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
        if (ConfigurationMgr().IsFullyProvisioned())
        {
            ClearFlag(mFlags, kFlag_AdvertisingEnabled);
            ChipLogProgress(DeviceLayer, "CHIPoBLE advertising disabled because device is fully provisioned");
        }
#endif // CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
    }

    // If the application has enabled CHIPoBLE and BLE advertising...
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_Enabled &&
        mFlags.Has(Flags::kFlag_AdvertisingEnabled)
#if CHIP_DEVICE_CONFIG_CHIPOBLE_SINGLE_CONNECTION
        // and no connections are active...
        && (mNumCons == 0)
#endif
    )
    {
        // Start/re-start SoftDevice advertising if not already advertising, or if the
        // advertising state of the SoftDevice needs to be refreshed.
        if (!mFlags.Has(Flags::kFlag_Advertising) || mFlags.Has(Flags::kFlag_AdvertisingRefreshNeeded))
        {
            ChipLogProgress(DeviceLayer, "CHIPoBLE advertising started");

            mFlags.Set(Flags::kFlag_Advertising, true);
            mFlags.Set(Flags::kFlag_AdvertisingRefreshNeeded, false);

            SetAdvertisingData();

            wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, BLE_ADDR_PUBLIC, NULL);

            // Post a CHIPoBLEAdvertisingChange(Started) event.
            {
                ChipDeviceEvent advChange;
                advChange.Type                             = DeviceEventType::kCHIPoBLEAdvertisingChange;
                advChange.CHIPoBLEAdvertisingChange.Result = kActivity_Started;
                err                                        = PlatformMgr().PostEvent(&advChange);
            }
        }
    }

    // Otherwise, stop advertising if currently active.
    else
    {
        if (mFlags.Has(Flags::kFlag_Advertising))
        {
            mFlags.Set(Flags::kFlag_Advertising, false);

            ChipLogProgress(DeviceLayer, "CHIPoBLE stop advertising");
            wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF, BLE_ADDR_PUBLIC, NULL);
        }
    }

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Disabling CHIPoBLE service due to error: %s", ErrorStr(err));
        mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Disabled;
    }
}

/*
 * This function searches through the GATT DB to point to the attribute
 * corresponding to the given handle
 */
gatt_db_lookup_table_t * BLEManagerImpl::GetGattAttr(uint16_t handle)
{
    /* Search for the given handle in the GATT DB and return the pointer to the
    correct attribute */
    uint8_t array_index = 0;

    for (array_index = 0; array_index < app_gatt_db_ext_attr_tbl_size; array_index++)
    {
        if (app_gatt_db_ext_attr_tbl[array_index].handle == handle)
        {
            return (&app_gatt_db_ext_attr_tbl[array_index]);
        }
    }
    return NULL;
}

/*
 * Currently there is no reason to pass Read Req to CHIP. Only process request for
 * attributes in the GATT DB attribute table
 */
wiced_bt_gatt_status_t BLEManagerImpl::HandleGattServiceRead(uint16_t conn_id, wiced_bt_gatt_read_t * p_read_data)
{
    gatt_db_lookup_table_t * puAttribute;
    int attr_len_to_copy;

    /* Get the right address for the handle in Gatt DB */
    if (NULL == (puAttribute = GetGattAttr(p_read_data->handle)))
    {
        ChipLogError(DeviceLayer, "Read handle attribute not found. Handle:0x%X\n", p_read_data->handle);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    attr_len_to_copy = puAttribute->cur_len;

    ChipLogProgress(DeviceLayer, "GATT Read handler: conn_id:%04x handle:%04x len:%d", conn_id, p_read_data->handle,
                    attr_len_to_copy);

    /* If the incoming offset is greater than the current length in the GATT DB
    then the data cannot be read back*/
    if (p_read_data->offset >= puAttribute->cur_len)
    {
        attr_len_to_copy = 0;
    }

    /* Calculate the number of bytes and the position of the data and copy it to
     the given pointer */
    if (attr_len_to_copy != 0)
    {
        uint8_t * from;
        int size_to_copy = attr_len_to_copy - p_read_data->offset;

        if (size_to_copy > *p_read_data->p_val_len)
        {
            size_to_copy = *p_read_data->p_val_len;
        }

        from                    = ((uint8_t *) puAttribute->p_data) + p_read_data->offset;
        *p_read_data->p_val_len = size_to_copy;

        memcpy(p_read_data->p_val, from, size_to_copy);
    }
    return WICED_BT_GATT_SUCCESS;
}

/*
 * If Attribute is for CHIP, pass it through. Otherwise process request for
 * attributes in the GATT DB attribute table.
 */
wiced_bt_gatt_status_t BLEManagerImpl::HandleGattServiceWrite(uint16_t conn_id, wiced_bt_gatt_write_t * p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_SUCCESS;
    gatt_db_lookup_table_t * puAttribute;
    const uint16_t valLen = p_data->val_len;

    // special handling for CHIP RX path
    if (p_data->handle == HDLC_CHIP_SERVICE_CHAR_C1_VALUE)
    {
        chip::System::PacketBuffer * buf;
        const uint16_t minBufSize = chip::System::PacketBuffer::kMaxSize + valLen;

        // Attempt to allocate a packet buffer with enough space to hold the characteristic value.
        // Note that we must use pbuf_alloc() directly, as PacketBuffer::New() is not interrupt-safe.
        if ((buf = (chip::System::PacketBuffer *) pbuf_alloc(PBUF_RAW, minBufSize, PBUF_POOL)) != NULL)
        {
            // Copy the characteristic value into the packet buffer.
            memcpy(buf->Start(), p_data->p_val, valLen);
            buf->SetDataLength(valLen);
#ifdef BLE_DEBUG
            ChipLogDetail(DeviceLayer, "Write received for CHIPoBLE RX characteristic con %04x len %d", conn_id, valLen);
#endif
            // Post an event to the CHIP queue to deliver the data into the CHIP stack.
            {
                ChipDeviceEvent event;
                event.Type                        = DeviceEventType::kCHIPoBLEWriteReceived;
                event.CHIPoBLEWriteReceived.ConId = conn_id;
                event.CHIPoBLEWriteReceived.Data  = buf;
                CHIP_ERROR status                 = PlatformMgr().PostEvent(&event);
                if (status != CHIP_NO_ERROR)
                {
                    result = WICED_BT_GATT_INTERNAL_ERROR;
                }
                buf = NULL;
            }
        }
        else
        {
            ChipLogError(DeviceLayer, "BLEManagerImpl: Out of buffers during CHIPoBLE RX");
            result = WICED_BT_GATT_NO_RESOURCES;
        }
    }
    else
    {
        ChipLogDetail(DeviceLayer, "Write received for CHIPoBLE RX characteristic con:%04x handle:%04x len:%d", conn_id,
                      p_data->handle, valLen);

        /* Get the right address for the handle in Gatt DB */
        if (NULL == (puAttribute = GetGattAttr(p_data->handle)))
        {
            ChipLogError(DeviceLayer, "BLEManagerImpl: Write wrong handle:%04x", p_data->handle);
            return WICED_BT_GATT_INVALID_HANDLE;
        }
        puAttribute->cur_len = valLen > puAttribute->max_len ? puAttribute->max_len : valLen;
        memcpy(puAttribute->p_data, p_data->p_val, puAttribute->cur_len);

        // Post an event to the Chip queue to process either a CHIPoBLE Subscribe or Unsubscribe based on
        // whether the client is enabling or disabling indications.
        if (p_data->handle == HDLD_CHIP_SERVICE_RX_CLIENT_CHAR_CONFIG)
        {
            ChipDeviceEvent event;
            event.Type = (app_chip_service_char_tx_client_char_config[0] != 0) ? DeviceEventType::kCHIPoBLESubscribe
                                                                               : DeviceEventType::kCHIPoBLEUnsubscribe;
            event.CHIPoBLESubscribe.ConId = conn_id;
            if (PlatformMgr().PostEvent(&event) != CHIP_NO_ERROR)
            {
                return WICED_BT_GATT_INTERNAL_ERROR;
            }
        }

        ChipLogProgress(DeviceLayer, "CHIPoBLE %s received",
                        app_chip_service_char_tx_client_char_config[0] != 0 ? "subscribe" : "unsubscribe");
    }
    return result;
}

/*
 * Process MTU request received from the GATT client
 */
wiced_bt_gatt_status_t BLEManagerImpl::HandleGattServiceMtuReq(wiced_bt_gatt_attribute_request_t * p_data,
                                                               CHIPoBLEConState * p_conn)
{
    p_data->data.mtu = p_conn->Mtu;

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process GATT attribute requests
 */
wiced_bt_gatt_status_t BLEManagerImpl::HandleGattServiceRequestEvent(wiced_bt_gatt_attribute_request_t * p_request,
                                                                     CHIPoBLEConState * p_conn)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;

    switch (p_request->request_type)
    {
    case GATTS_REQ_TYPE_READ:
        result = HandleGattServiceRead(p_request->conn_id, &(p_request->data.read_req));
        break;

    case GATTS_REQ_TYPE_WRITE:
        result = HandleGattServiceWrite(p_request->conn_id, &(p_request->data.write_req));
        break;

    case GATTS_REQ_TYPE_MTU:
        result = HandleGattServiceMtuReq(p_request, p_conn);
        break;

    default:
        break;
    }

    return result;
}

/*
 * Handle GATT connection events from the stack
 */
wiced_bt_gatt_status_t BLEManagerImpl::HandleGattConnectEvent(wiced_bt_gatt_connection_status_t * p_conn_status,
                                                              CHIPoBLEConState * p_conn)
{
    if (p_conn_status->connected)
    {
        /* Device got connected */
        p_conn->connected = true;
        ChipLogProgress(DeviceLayer, "BLE GATT connection up (con %u)", p_conn_status->conn_id);
    }
    else /* Device got disconnected */
    {
        ChipDeviceEvent event;
        event.Type                          = DeviceEventType::kCHIPoBLEConnectionError;
        event.CHIPoBLEConnectionError.ConId = p_conn_status->conn_id;

        switch (p_conn_status->reason)
        {
        case GATT_CONN_TERMINATE_PEER_USER:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_REMOTE_DEVICE_DISCONNECTED;
            break;

        case GATT_CONN_TERMINATE_LOCAL_HOST:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_APP_CLOSED_CONNECTION;
            break;

        default:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_CHIPOBLE_PROTOCOL_ABORT;
            break;
        }

        ChipLogProgress(DeviceLayer, "BLE GATT connection closed (con %u, reason %u)", p_conn_status->conn_id,
                        p_conn_status->reason);

        if (PlatformMgr().PostEvent(&event) != CHIP_NO_ERROR)
        {
            return WICED_BT_GATT_INTERNAL_ERROR;
        }

        // Arrange to re-enable connectable advertising in case it was disabled due to the
        // maximum connection limit being reached.
        mFlags.Set(Flags::kFlag_Advertising, false);
        PlatformMgr().ScheduleWork(DriveBLEState, 0);

        ReleaseConnectionState(p_conn->ConId);
    }
    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process GATT requests. Callback is received in the BT stack thread context.
 *
 */
wiced_bt_gatt_status_t app_gatts_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t * p_data)
{
    uint16_t conn_id;
    BLEManagerImpl::CHIPoBLEConState * p_conn;

    /* Check parameter. */
    if (!p_data)
    {
        return WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    /* Check if target connection state exists. */
    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        conn_id = p_data->connection_status.conn_id;
        break;

    case GATT_OPERATION_CPLT_EVT:
        conn_id = p_data->operation_complete.conn_id;
        break;

    case GATT_DISCOVERY_RESULT_EVT:
        conn_id = p_data->discovery_result.conn_id;
        break;

    case GATT_DISCOVERY_CPLT_EVT:
        conn_id = p_data->discovery_complete.conn_id;
        break;

    case GATT_ATTRIBUTE_REQUEST_EVT:
        conn_id = p_data->attribute_request.conn_id;
        break;

    case GATT_CONGESTION_EVT:
        conn_id = p_data->congestion.conn_id;
        break;

    default:
        return WICED_BT_GATT_ILLEGAL_PARAMETER;
    }

    p_conn = BLEManagerImpl::sInstance.GetConnectionState(conn_id);

    /* Allocate connection state if no exist. */
    if (!p_conn)
    {
        p_conn = BLEManagerImpl::sInstance.AllocConnectionState(conn_id);

        if (!p_conn)
        {
            return WICED_BT_GATT_INSUF_RESOURCE;
        }
    }

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        return BLEManagerImpl::sInstance.HandleGattConnectEvent(&p_data->connection_status, p_conn);

    case GATT_ATTRIBUTE_REQUEST_EVT:
        return BLEManagerImpl::sInstance.HandleGattServiceRequestEvent(&p_data->attribute_request, p_conn);

    default:
        break;
    }

    return WICED_BT_GATT_ILLEGAL_PARAMETER;
}

void BLEManagerImpl::SetAdvertisingData(void)
{
    CHIP_ERROR err;
    wiced_bt_ble_advert_elem_t adv_elem[4];
    uint8_t num_elem             = 0;
    uint8_t flag                 = BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED;
    uint8_t chip_service_uuid[2] = { BIT16_TO_8(__UUID16_CHIPoBLEService) };
    ChipBLEDeviceIdentificationInfo mDeviceIdInfo;
    uint16_t deviceDiscriminator = 0;
    uint8_t localDeviceNameLen;
    uint8_t service_data[BLE_SERVICE_DATA_SIZE];
    uint8_t * p = service_data;

    static_assert(BLE_SERVICE_DATA_SIZE == sizeof(ChipBLEDeviceIdentificationInfo) + 2, "BLE Service Data Size is incorrect");

    // Initialize the CHIP BLE Device Identification Information block that will be sent as payload
    // within the BLE service advertisement data.
    err = ConfigurationMgr().GetBLEDeviceIdentificationInfo(mDeviceIdInfo);
    SuccessOrExit(err);

    // Get device discriminator
    deviceDiscriminator = mDeviceIdInfo.GetDeviceDiscriminator();

    // Verify device name was not already set
    if (!sInstance.mFlags.Has(sInstance.Flags::kFlag_DeviceNameSet))
    {
        /* Default device name is CHIP-<DISCRIMINATOR> */
        memset(sInstance.mDeviceName, 0, kMaxDeviceNameLength);
        snprintf(sInstance.mDeviceName, kMaxDeviceNameLength, "%s%04u", CHIP_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX,
                 deviceDiscriminator);
        localDeviceNameLen = strlen(sInstance.mDeviceName);

        strncpy((char *) app_gap_device_name, sInstance.mDeviceName, sizeof(app_gap_device_name));
        app_gatt_db_ext_attr_tbl[0].cur_len = app_gatt_db_ext_attr_tbl[0].max_len < strlen(sInstance.mDeviceName)
            ? app_gatt_db_ext_attr_tbl[0].max_len
            : strlen(sInstance.mDeviceName);

        ChipLogProgress(DeviceLayer, "SetAdvertisingData: device name set: %s", sInstance.mDeviceName);
    }
    else
    {
        localDeviceNameLen = strlen(sInstance.mDeviceName);
    }

    /* First element is the advertisement flags */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_FLAG;
    adv_elem[num_elem].len         = sizeof(uint8_t);
    adv_elem[num_elem].p_data      = &flag;
    num_elem++;

    /* Second element is the service data for CHIP service */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_SERVICE_DATA;
    adv_elem[num_elem].len         = sizeof(service_data);
    adv_elem[num_elem].p_data      = service_data;
    num_elem++;
    UINT8_TO_STREAM(p, chip_service_uuid[0]);
    UINT8_TO_STREAM(p, chip_service_uuid[1]);
    UINT8_TO_STREAM(p, 0); // CHIP BLE Opcode == 0x00 (Uncommissioned)
    UINT16_TO_STREAM(p, deviceDiscriminator);
    UINT8_TO_STREAM(p, mDeviceIdInfo.DeviceVendorId[0]);
    UINT8_TO_STREAM(p, mDeviceIdInfo.DeviceVendorId[1]);
    UINT8_TO_STREAM(p, mDeviceIdInfo.DeviceProductId[0]);
    UINT8_TO_STREAM(p, mDeviceIdInfo.DeviceProductId[1]);
    UINT8_TO_STREAM(p, 0); // Additional Data Flag

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len         = localDeviceNameLen;
    adv_elem[num_elem].p_data      = (uint8_t *) sInstance.mDeviceName;
    num_elem++;

    wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);

    /* Configure Scan Response data */
    num_elem                       = 0;
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len         = localDeviceNameLen;
    adv_elem[num_elem].p_data      = (uint8_t *) sInstance.mDeviceName;
    num_elem++;

    wiced_bt_ble_set_raw_scan_response_data(num_elem, adv_elem);

exit:
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SetAdvertisingData err:%s", ErrorStr(err));
}

BLEManagerImpl::CHIPoBLEConState * BLEManagerImpl::AllocConnectionState(uint16_t conId)
{
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].connected == false)
        {
            mCons[i].ConId     = conId;
            mCons[i].Mtu       = wiced_bt_cfg_settings.gatt_cfg.max_mtu_size;
            mCons[i].connected = false;

            mNumCons++;

            return &mCons[i];
        }
    }
    ChipLogError(DeviceLayer, "Failed to allocate CHIPoBLEConState");
    return NULL;
}

BLEManagerImpl::CHIPoBLEConState * BLEManagerImpl::GetConnectionState(uint16_t conId)
{
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].ConId == conId)
        {
            return &mCons[i];
        }
    }
    ChipLogError(DeviceLayer, "Failed to find CHIPoBLEConState");
    return NULL;
}

bool BLEManagerImpl::ReleaseConnectionState(uint16_t conId)
{
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].ConId == conId)
        {
            memset(&mCons[i], 0, sizeof(CHIPoBLEConState));
            mNumCons--;
            return true;
        }
    }
    ChipLogError(DeviceLayer, "Failed to delete CHIPoBLEConState");
    return false;
}

void BLEManagerImpl::DriveBLEState(intptr_t arg)
{
    sInstance.DriveBLEState();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif
