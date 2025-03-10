/*
 *
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

// THIS FILE IS GENERATED BY ZAP

#pragma once

#include <app-common/zap-generated/callbacks/PluginCallbacks.h>

#define MATTER_PLUGINS_INIT                                                                                                        \
    MatterIdentifyPluginServerInitCallback();                                                                                      \
    MatterGroupsPluginServerInitCallback();                                                                                        \
    MatterScenesPluginServerInitCallback();                                                                                        \
    MatterOnOffPluginServerInitCallback();                                                                                         \
    MatterOnOffSwitchConfigurationPluginServerInitCallback();                                                                      \
    MatterLevelControlPluginServerInitCallback();                                                                                  \
    MatterBinaryInputBasicPluginServerInitCallback();                                                                              \
    MatterDescriptorPluginServerInitCallback();                                                                                    \
    MatterBindingPluginServerInitCallback();                                                                                       \
    MatterAccessControlPluginServerInitCallback();                                                                                 \
    MatterBridgedActionsPluginServerInitCallback();                                                                                \
    MatterBasicPluginServerInitCallback();                                                                                         \
    MatterOtaSoftwareUpdateProviderPluginClientInitCallback();                                                                     \
    MatterOtaSoftwareUpdateRequestorPluginServerInitCallback();                                                                    \
    MatterLocalizationConfigurationPluginServerInitCallback();                                                                     \
    MatterTimeFormatLocalizationPluginServerInitCallback();                                                                        \
    MatterUnitLocalizationPluginServerInitCallback();                                                                              \
    MatterPowerSourceConfigurationPluginServerInitCallback();                                                                      \
    MatterPowerSourcePluginServerInitCallback();                                                                                   \
    MatterGeneralCommissioningPluginServerInitCallback();                                                                          \
    MatterNetworkCommissioningPluginServerInitCallback();                                                                          \
    MatterDiagnosticLogsPluginServerInitCallback();                                                                                \
    MatterGeneralDiagnosticsPluginServerInitCallback();                                                                            \
    MatterSoftwareDiagnosticsPluginServerInitCallback();                                                                           \
    MatterThreadNetworkDiagnosticsPluginServerInitCallback();                                                                      \
    MatterWiFiNetworkDiagnosticsPluginServerInitCallback();                                                                        \
    MatterEthernetNetworkDiagnosticsPluginServerInitCallback();                                                                    \
    MatterSwitchPluginServerInitCallback();                                                                                        \
    MatterAdministratorCommissioningPluginServerInitCallback();                                                                    \
    MatterOperationalCredentialsPluginServerInitCallback();                                                                        \
    MatterGroupKeyManagementPluginServerInitCallback();                                                                            \
    MatterFixedLabelPluginServerInitCallback();                                                                                    \
    MatterUserLabelPluginServerInitCallback();                                                                                     \
    MatterBooleanStatePluginServerInitCallback();                                                                                  \
    MatterModeSelectPluginServerInitCallback();                                                                                    \
    MatterDoorLockPluginServerInitCallback();                                                                                      \
    MatterWindowCoveringPluginServerInitCallback();                                                                                \
    MatterBarrierControlPluginServerInitCallback();                                                                                \
    MatterPumpConfigurationAndControlPluginServerInitCallback();                                                                   \
    MatterThermostatPluginServerInitCallback();                                                                                    \
    MatterFanControlPluginServerInitCallback();                                                                                    \
    MatterThermostatUserInterfaceConfigurationPluginServerInitCallback();                                                          \
    MatterColorControlPluginServerInitCallback();                                                                                  \
    MatterIlluminanceMeasurementPluginServerInitCallback();                                                                        \
    MatterTemperatureMeasurementPluginServerInitCallback();                                                                        \
    MatterPressureMeasurementPluginServerInitCallback();                                                                           \
    MatterFlowMeasurementPluginServerInitCallback();                                                                               \
    MatterRelativeHumidityMeasurementPluginServerInitCallback();                                                                   \
    MatterOccupancySensingPluginServerInitCallback();                                                                              \
    MatterWakeOnLanPluginServerInitCallback();                                                                                     \
    MatterChannelPluginServerInitCallback();                                                                                       \
    MatterTargetNavigatorPluginServerInitCallback();                                                                               \
    MatterMediaPlaybackPluginServerInitCallback();                                                                                 \
    MatterMediaInputPluginServerInitCallback();                                                                                    \
    MatterLowPowerPluginServerInitCallback();                                                                                      \
    MatterKeypadInputPluginServerInitCallback();                                                                                   \
    MatterContentLauncherPluginServerInitCallback();                                                                               \
    MatterAudioOutputPluginServerInitCallback();                                                                                   \
    MatterApplicationLauncherPluginServerInitCallback();                                                                           \
    MatterApplicationBasicPluginServerInitCallback();                                                                              \
    MatterAccountLoginPluginServerInitCallback();                                                                                  \
    MatterElectricalMeasurementPluginServerInitCallback();                                                                         \
    MatterTestClusterPluginServerInitCallback();
