#ifndef CPLUGIN_HELPER_MQTT_H
#define CPLUGIN_HELPER_MQTT_H

#if FEATURE_MQTT
# include "../Helpers/_CPlugin_Helper.h"

bool MQTT_handle_topic_commands(struct EventStruct *event,
                                bool                handleCmd       = true,
                                bool                handleSet       = true,
                                bool                tryRemoteConfig = false);
void MQTT_execute_command(String& command,
                          bool    tryRemoteConfig = false);
bool MQTT_protocol_send(EventStruct *event,
                        String       pubname,
                        bool         retainFlag);

# if FEATURE_MQTT_DISCOVER

typedef int (*QueryVType_ptr)(uint8_t);

bool getDiscoveryVType(struct EventStruct *event,
                       QueryVType_ptr      func_ptr,
                       uint8_t             pConfigOffset,
                       uint8_t             nrVars);

int Plugin_QueryVType_Analog(uint8_t value_nr);
int Plugin_QueryVType_CO2(uint8_t value_nr);
int Plugin_QueryVType_Distance(uint8_t value_nr);
int Plugin_QueryVType_DustPM2_5(uint8_t value_nr);
int Plugin_QueryVType_Lux(uint8_t value_nr);
int Plugin_QueryVType_Temperature(uint8_t value_nr);
int Plugin_QueryVType_Weight(uint8_t value_nr);

struct DiscoveryItem {
  DiscoveryItem(Sensor_VType _VType, int _valueCount, taskVarIndex_t _varIndex)
    : VType(_VType), valueCount(_valueCount), varIndex(_varIndex) {}

  Sensor_VType   VType;
  int            valueCount;
  taskVarIndex_t varIndex;
};

bool   MQTT_SendAutoDiscovery(controllerIndex_t ControllerIndex,
                              cpluginID_t       CPluginID);
bool   MQTT_HomeAssistant_SendAutoDiscovery(controllerIndex_t         ControllerIndex,
                                            ControllerSettingsStruct& ControllerSettings);
bool   MQTT_DiscoveryGetDeviceVType(taskIndex_t                 TaskIndex,
                                    std::vector<DiscoveryItem>& discoveryItems,
                                    int                         valueCount);
String MQTT_TaskValueUniqueName(const String& taskName,
                                const String& valueName);
String MQTT_DiscoveryBuildValueTopic(const String            & topic,
                                     struct EventStruct       *event,
                                     uint8_t                   taskValueIndex,
                                     const __FlashStringHelper*deviceClass);

bool MQTT_DiscoveryPublish(controllerIndex_t ControllerIndex,
                           const String    & topic,
                           const String    & discoveryMessage,
                           taskIndex_t       x,
                           uint8_t           v,
                           const String    & taskName);

bool MQTT_DiscoveryPublishWithStatusAndSet(taskIndex_t               taskIndex,
                                           uint8_t                   taskValue,
                                           String                    taskName,
                                           controllerIndex_t         ControllerIndex,
                                           String                    publishTopic,
                                           String                    discoveryTopic,
                                           const __FlashStringHelper*componentClass,
                                           String                    deviceClass,
                                           String                    unitOfMeasure,
                                           struct EventStruct       *event,
                                           const String              deviceElement,
                                           bool                      success,
                                           bool                      hasSet,
                                           bool                      hasIcon);
# endif // if FEATURE_MQTT_DISCOVER
#endif // if FEATURE_MQTT
#endif // ifndef CPLUGIN_HELPER_MQTT_H
