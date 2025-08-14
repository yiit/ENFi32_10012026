#include "../PluginStructs/P089_data_struct.h"

#ifdef USES_P089


# include "../Helpers/Networking.h"

# include "../Helpers/_Plugin_init.h"

# ifdef ESP32
P089_data_struct::P089_data_struct() {
  espPing                     = new (std::nothrow) PingClass();
  _ping_task_data.initialized = isInitialized();
}

P089_data_struct::~P089_data_struct() {
  if (_ping_task_data.taskHandle) {
    vTaskDelete(_ping_task_data.taskHandle);
  }
  delete espPing;
}

void P089_execute_ping_task(void *parameter)
{
  P089_ping_task_data*ping_task_data = static_cast<P089_ping_task_data *>(parameter);

  if ((ping_task_data->status == P089_status::Working) && (nullptr != ping_task_data->espPing)) {
    // Blocking operation
    ping_task_data->result = ping_task_data->espPing->ping(ping_task_data->pingIp, ping_task_data->count);

    // Results are in
    ping_task_data->avgTime = ping_task_data->espPing->averageTime();
    ping_task_data->status  = P089_status::Ready;
  }

  ping_task_data->taskHandle = NULL;
  vTaskDelete(ping_task_data->taskHandle);
}

bool P089_data_struct::send_ping(struct EventStruct *event) {
  /* This ping lost for sure */
  if (!isInitialized() || !NetworkConnected()) {
    return true;
  }

  if (!_ping_task_data.initialized) {
    // addLog(LOG_LEVEL_ERROR, F("PING : Not initialized."));
    return true; // Not (yet?) initialized
  }

  if (_ping_task_data.status == P089_status::Working) {
    // addLog(LOG_LEVEL_INFO, F("PING : Still working."));
    return false; // Busy, but not a failure
  }

  if (_ping_task_data.status == P089_status::Ready) {
    if (_ping_task_data.result) {
      addLog(LOG_LEVEL_INFO, strformat(F("PING : Successfully pinged %s (%s) count: %d avg: %.03f ms"),
                                       _ping_task_data.hostname.c_str(),
                                       _ping_task_data.pingIp.toString().c_str(),
                                       _ping_task_data.count,
                                       _ping_task_data.avgTime));
    } else {
      addLog(LOG_LEVEL_ERROR, strformat(F("PING : Error pinging %s (%s)"),
                                        _ping_task_data.hostname.c_str(),
                                        _ping_task_data.pingIp.toString().c_str()));
    }
    UserVar.setFloat(event->TaskIndex, 1, _ping_task_data.avgTime); // Set always, even when not specifically enabled

    _ping_task_data.status = P089_status::Initial;                  // Ready for new work
    return !_ping_task_data.result;                                 // Inverted result to report back!
  }

  IPAddress ip;
  char hostname[PLUGIN_089_HOSTNAME_SIZE]{};

  LoadCustomTaskSettings(event->TaskIndex, (uint8_t *)&hostname, PLUGIN_089_HOSTNAME_SIZE);

  /* This one lost as well, DNS dead? */
  if (!resolveHostByName(hostname, ip)) {
    return true;
  }

  int16_t pingCount = P089_PING_COUNT;

  if ((pingCount < 1) || (pingCount > 100)) {
    pingCount = 5;
  }

  _ping_task_data.status   = P089_status::Working;
  _ping_task_data.count    = pingCount;
  _ping_task_data.pingIp   = ip;
  _ping_task_data.hostname = hostname; // Bring all data to the task
  _ping_task_data.espPing  = espPing;

  xTaskCreatePinnedToCore(
    P089_execute_ping_task,      // Function that should be called
    "PingClass.ping()",          // Name of the task (for debugging)
    4000,                        // Stack size (bytes)
    &_ping_task_data,            // Parameter to pass
    1,                           // Task priority
    &_ping_task_data.taskHandle, // Task handle
    xPortGetCoreID()             // Core you want to run the task on (0 or 1)
    );

  return false;                  // Started work, not a failure
}

# endif // ifdef ESP32

# ifdef ESP8266
P089_data_struct::P089_data_struct() {
  destIPAddress.addr = 0;
  idseq              = 0;

  if (nullptr == P089_data) {
    P089_data = new (std::nothrow) P089_icmp_pcb();

    if (P089_data != nullptr) {
      P089_data->m_IcmpPCB = raw_new(IP_PROTO_ICMP);
      raw_recv(P089_data->m_IcmpPCB, PingReceiver, nullptr);
      raw_bind(P089_data->m_IcmpPCB, IP_ADDR_ANY);
    }
  } else {
    P089_data->instances++;
  }
}

P089_data_struct::~P089_data_struct() {
  if (P089_data != nullptr) {
    P089_data->instances--;

    if (P089_data->instances == 0) {
      raw_remove(P089_data->m_IcmpPCB);
      delete P089_data;
      P089_data = nullptr;
    }
  }
}

bool P089_data_struct::send_ping(struct EventStruct *event) {
  bool is_failure = false;
  IPAddress ip;

  // Do we have unanswered pings? If we are sending new one, this means old one is lost
  if (destIPAddress.addr != 0) {
    is_failure = true;
  }

  /* This ping lost for sure */
  if (!NetworkConnected()) {
    return true;
  }

  char hostname[PLUGIN_089_HOSTNAME_SIZE];

  LoadCustomTaskSettings(event->TaskIndex, (uint8_t *)&hostname, PLUGIN_089_HOSTNAME_SIZE);

  /* This one lost as well, DNS dead? */
  if (!resolveHostByName(hostname, ip)) {
    return true;
  }
  destIPAddress.addr = ip;

  /* Generate random ID & seq */
  idseq = HwRandom();
  u16_t ping_len            = ICMP_PAYLOAD_LEN + sizeof(struct icmp_echo_hdr);
  struct pbuf *packetBuffer = pbuf_alloc(PBUF_IP, ping_len, PBUF_RAM);

  /* Lost for sure, TODO: Might be good to log such failures, this means we are short on ram? */
  if (packetBuffer == nullptr) {
    return true;
  }

  struct icmp_echo_hdr *echoRequestHeader = (struct icmp_echo_hdr *)packetBuffer->payload;

  ICMPH_TYPE_SET(echoRequestHeader, ICMP_ECHO);
  ICMPH_CODE_SET(echoRequestHeader, 0);
  echoRequestHeader->chksum = 0;
  echoRequestHeader->id     = (uint16_t)((idseq & 0xffff0000) >> 16);
  echoRequestHeader->seqno  = (uint16_t)(idseq & 0xffff);
  size_t icmpHeaderLen = sizeof(struct icmp_echo_hdr);
  size_t icmpDataLen   = ping_len - icmpHeaderLen;
  char   dataByte      = 0x61;

  for (size_t i = 0; i < icmpDataLen; i++) {
    ((char *)echoRequestHeader)[icmpHeaderLen + i] = dataByte;
    ++dataByte;

    if (dataByte > 0x77) // 'w' character
    {
      dataByte = 0x61;
    }
  }
  echoRequestHeader->chksum = inet_chksum(echoRequestHeader, ping_len);
  ip_addr_t destIPAddress;

  destIPAddress.addr = ip;
  raw_sendto(P089_data->m_IcmpPCB, packetBuffer, &destIPAddress);

  pbuf_free(packetBuffer);

  return is_failure;
}

uint8_t PingReceiver(void *origin, struct raw_pcb *pcb, struct pbuf *packetBuffer, const ip_addr_t *addr)
{
  if ((packetBuffer == nullptr) || (addr == nullptr)) {
    return 0;
  }

  if (packetBuffer->len < sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr) + ICMP_PAYLOAD_LEN) {
    return 0;
  }

  // TODO: Check some ipv4 header values?
  // struct ip_hdr * ip = (struct ip_hdr *)packetBuffer->payload;

  if (pbuf_header(packetBuffer, -PBUF_IP_HLEN) != 0) {
    return 0;
  }

  // After the IPv4 header, we can access the icmp echo header
  struct icmp_echo_hdr *icmp_hdr = (struct icmp_echo_hdr *)packetBuffer->payload;

  // Is it echo reply?
  if (icmp_hdr->type != 0) {
    pbuf_header(packetBuffer, PBUF_IP_HLEN);
    return 0;
  }

  bool is_found = false;

  for (taskIndex_t index = 0; index < TASKS_MAX; index++) {
    deviceIndex_t deviceIndex = getDeviceIndex_from_TaskIndex(index);

    // Match all ping plugin instances and check them
    constexpr pluginID_t PLUGIN_ID_P089_PING(PLUGIN_ID_089);

    if (getPluginID_from_DeviceIndex(deviceIndex) == PLUGIN_ID_P089_PING) {
      P089_data_struct *P089_taskdata = static_cast<P089_data_struct *>(getPluginTaskData(index));

      if ((P089_taskdata != nullptr) && (icmp_hdr->id == (uint16_t)((P089_taskdata->idseq & 0xffff0000) >> 16)) &&
          (icmp_hdr->seqno == (uint16_t)(P089_taskdata->idseq & 0xffff))) {
        UserVar.setFloat(index, 0, 0); // Reset "fails", we got reply
        P089_taskdata->idseq              = 0;
        P089_taskdata->destIPAddress.addr = 0;
        is_found                          = true;
      }
    }
  }

  if (!is_found) {
    pbuf_header(packetBuffer, PBUF_IP_HLEN);
    return 0;
  }

  // Everything fine, release the kraken, ehm, buffer
  pbuf_free(packetBuffer);
  return 1;
}

# endif // ifdef ESP8266
#endif // ifdef USES_P089
