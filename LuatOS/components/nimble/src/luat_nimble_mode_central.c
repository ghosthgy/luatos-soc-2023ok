
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "luat_base.h"
#if (defined(TLS_CONFIG_CPU_XT804))
#include "FreeRTOS.h"
#else
#include "freertos/FreeRTOS.h"
#endif

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_nimble.h"

/* BLE */
#include "nimble/nimble_port.h"

// #define LUAT_NIMBLE_DEBUG 0
// #if LUAT_NIMBLE_DEBUG == 0 
// #undef LLOGD
// #define LLOGD(...) 
// #endif

static size_t ble_gatt_svc_counter = 0;
static size_t ble_gatt_chr_counter = 0;

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

extern struct ble_gatt_svc *peer_servs[];
extern struct ble_gatt_chr *peer_chrs[];

static uint16_t peer_dscs[MAX_PER_SERV * MAX_PER_SERV];

extern uint16_t g_ble_conn_handle;
extern uint16_t g_ble_state;

#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

static char selfname[32];
// extern uint16_t g_ble_conn_handle;
// static uint16_t g_ble_state;

typedef struct luat_nimble_scan_result
{
    uint16_t uuids_16[16];
    uint32_t uuids_32[16];
    uint8_t uuids_128[16][16];
    uint8_t mfg_data[128];
    char name[64];
    char addr[7]; // 地址类型 + MAC地址
    uint8_t mfg_data_len;
    int8_t rssi;
}luat_nimble_scan_result_t;

void rand_bytes(uint8_t *data, int len);

void print_bytes(const uint8_t *bytes, int len);

void print_addr(const void *addr);

void print_mbuf(const struct os_mbuf *om);

char *addr_str(const void *addr);

void print_uuid(const ble_uuid_t *uuid);

void print_conn_desc(const struct ble_gap_conn_desc *desc);

void print_adv_fields(const struct ble_hs_adv_fields *fields);


void ble_store_config_init(void);

static int blecent_gap_event(struct ble_gap_event *event, void *arg);
static int svc_disced(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service,
                                 void *arg);
static int chr_disced(uint16_t conn_handle,
                            const struct ble_gatt_error *error,
                            const struct ble_gatt_chr *chr, void *arg);
static int dsc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                void *arg);

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    (void)arg;
    LLOGD("gatt_svr_register_cb op %d", ctxt->op);
}

/**
 * Initiates the GAP general discovery procedure.
 */
int luat_nimble_blecent_scan(int timeout)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params = {0};
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        LLOGE("error determining address type; rc=%d", rc);
        return rc;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 1;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 0;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, timeout*1000, &disc_params,
                      blecent_gap_event, NULL);
    if (rc != 0) {
        LLOGE("Error initiating GAP discovery procedure; rc=%d", rc);
    }
    return rc;
}


#define ADDR_FMT "%02X%02X%02X%02X%02X%02X"
#define ADDR_T(addr) addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]

static void
bleprph_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    LLOGI("handle=%d our_ota_addr_type=%d our_ota_addr=" ADDR_FMT, desc->conn_handle, desc->our_ota_addr.type, ADDR_T(desc->our_ota_addr.val));
    LLOGI(" our_id_addr_type=%d our_id_addr=" ADDR_FMT, desc->our_id_addr.type, ADDR_T(desc->our_id_addr.val));
    LLOGI(" peer_ota_addr_type=%d peer_ota_addr=" ADDR_FMT, desc->peer_ota_addr.type, ADDR_T(desc->peer_ota_addr.val));
    LLOGI(" peer_id_addr_type=%d peer_id_addr=" ADDR_FMT, desc->peer_id_addr.type, ADDR_T(desc->peer_id_addr.val));
    LLOGI(" conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

// int luat_nimble_connect(ble_addr_t *addr) {
int luat_nimble_blecent_connect(const char* _addr){
    uint8_t own_addr_type;
    int rc;
    ble_addr_t *addr;
    addr = (ble_addr_t *)_addr;
    for (size_t i = 0; i < ble_gatt_svc_counter; i++)
    {
        if (peer_servs[i]) {
            luat_heap_free(peer_servs[i]);
            peer_servs[i] = NULL;
        }
    }
    ble_gatt_svc_counter = 0;
    for (size_t i = 0; i < MAX_PER_SERV*MAX_PER_SERV; i++)
    {
        if (peer_chrs[i]) {
            luat_heap_free(peer_chrs[i]);
            peer_chrs[i] = NULL;
        }
    }
    ble_gatt_chr_counter = 0;
    

    // 首先, 停止搜索
    rc = ble_gap_disc_cancel();
    LLOGD("ble_gap_disc_cancel %d", rc);
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    LLOGD("ble_hs_id_infer_auto %d", rc);
    rc = ble_gap_connect(own_addr_type, addr, 30000, NULL, blecent_gap_event, NULL);
    LLOGD("ble_gap_connect %d", rc);
    return rc;
}


int luat_nimble_blecent_disconnect(int id) {
    (void)id;
    if (0 == g_ble_conn_handle) {
        return 0;
    }
    return ble_gap_terminate(g_ble_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

int luat_nimble_central_disc_srv(int id) {
    (void)id;
    if (!g_ble_conn_handle) {
        LLOGW("尚未建立连接");
        return -1;
    }
    return ble_gattc_disc_all_svcs(g_ble_conn_handle, svc_disced, NULL);
}

int luat_nimble_status_cb(lua_State*L, void*ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L,"sys_pub");
    lua_pushliteral(L, "BLE_CONN_STATUS");
    lua_pushboolean(L, msg->arg1 == 0 ? 1 : 0);
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 4, 0);
    return 0;
}

int luat_nimble_scan_cb(lua_State*L, void*ptr) {
    luat_nimble_scan_result_t* res = (luat_nimble_scan_result_t*)ptr;
    lua_getglobal(L,"sys_pub");

    lua_pushliteral(L, "BLE_SCAN_RESULT");

    lua_pushlstring(L, (const char*)res->addr, 7);

    if (res->name[0]) {
        lua_pushstring(L, res->name);
    }
    else {
        lua_pushliteral(L, ""); // 设备没有名字
    }
    lua_newtable(L);
    // char buff[64];

    // https://gitee.com/openLuat/LuatOS/issues/I7QSJP
    // 添加RSSI
    lua_pushinteger(L, res->rssi);
    lua_setfield(L, -2, "rssi");

    if (res->uuids_16[0]) {
        lua_newtable(L);
        for (size_t i = 0; i < 16; i++)
        {
            if (res->uuids_16[i] == 0)
                break;
            //lua_pushlstring(L, (const char*)&res->uuids_16[i], 2);
            lua_pushinteger(L, res->uuids_16[i]);
            lua_seti(L, -2, i+1);
        }
        lua_setfield(L, -2, "uuids16");
    }
    // if (res->uuids_32[0]) {
    //     lua_newtable(L);
    //     for (size_t i = 0; i < 16; i++)
    //     {
    //         if (res->uuids_32[i] == 0)
    //             break;
    //         lua_pushlstring(L, (const char*)&res->uuids_32[i], 4);
    //         lua_pushinteger(L, res->uuids_32[i]);
    //         lua_seti(L, -2, i+1);
    //     }
    //     lua_setfield(L, -2, "uuids32");
    // }
    // if (res->uuids_128[0][0]) {
    //     lua_newtable(L);
    //     for (size_t i = 0; i < 16; i++)
    //     {
    //         if (res->uuids_32[i] == 0)
    //             break;
    //         lua_pushlstring(L, (const char*)res->uuids_128[i], 16);
    //         lua_seti(L, -2, i+1);
    //     }
    //     lua_setfield(L, -2, "uuids128");
    // }
    if (res->mfg_data_len) {
        lua_pushlstring(L, (const char*)res->mfg_data, res->mfg_data_len);
    }
    else {
        lua_pushnil(L);
    }
    luat_heap_free(res);
    lua_call(L, 5, 0);
    return 0;
}

int luat_nimble_connect_cb(lua_State*L, void*ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L,"sys_pub");
    lua_pushliteral(L, "BLE_CONN_RESULT");
    lua_pushboolean(L, msg->arg1 == 0 ? 1 : 0);
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 4, 0);
    return 0;
}

int luat_nimble_chr_disc_cb(lua_State*L, void*ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L,"sys_pub");
    lua_pushliteral(L, "BLE_CHR_DISC_RESULT");
    lua_pushboolean(L, msg->arg1 == 0 ? 1 : 0);
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 4, 0);
    return 0;
}

static int svc_disced(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service,
                                 void *arg) {
    (void)conn_handle;
    (void)arg;
    rtos_msg_t msg = {.handler=luat_nimble_connect_cb};
    // LLOGD("svc_disced status %d", error->status);
    if (error->status == BLE_HS_EDONE) {
        // LLOGD("service discovery done count %d", ble_gatt_svc_counter);
        msg.arg1 = 0;
        msg.arg2 = ble_gatt_svc_counter;
        luat_msgbus_put(&msg, 0);
        int ret = ble_gattc_disc_all_chrs(g_ble_conn_handle, service->start_handle, service->end_handle, chr_disced, service);
        LLOGD("ble_gattc_disc_all_chrs ret %d", ret);
        return 0;
    }
    if (error->status != 0) {
        msg.arg1 = error->status;
        luat_msgbus_put(&msg, 0);
        return error->status;
    }
    if (ble_gatt_svc_counter >= MAX_PER_SERV) {
        return 0;
    }
    if (peer_servs[ble_gatt_svc_counter] == NULL) {
        peer_servs[ble_gatt_svc_counter] = luat_heap_malloc(sizeof(struct ble_gatt_svc));
    }
    memcpy(peer_servs[ble_gatt_svc_counter], service, sizeof(struct ble_gatt_svc));
    
    // char buff[64] = {0};
    // LLOGD("service->start_handle %04X", service->start_handle);
    // LLOGD("service->end_handle %04X",   service->end_handle);
    // LLOGD("service->uuid %s",         ble_uuid_to_str(&service->uuid, buff));
    ble_gatt_svc_counter ++;
    return 0;
}

static const ble_uuid_t *uuid_ccc =
                BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16);

static int dsc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                void *arg) {
    // LLOGD("dsc_disced status %d", error->status);
    rtos_msg_t msg = {.handler=luat_nimble_chr_disc_cb};
    if (error->status == BLE_HS_EDONE) {
        msg.arg1 = 0;
        msg.arg2 = ble_gatt_chr_counter;
        luat_msgbus_put(&msg, 0);
        return 0;
    }
    if (error->status == 0) {
        struct ble_gatt_chr *chr = (struct ble_gatt_chr *)(arg);
        // char buff[64];
        // LLOGD("dsc_disced %d %d %s", chr_val_handle, dsc->handle, ble_uuid_to_str(&dsc->uuid, buff));
        for (size_t i = 0; i < MAX_PER_SERV * MAX_PER_SERV; i++)
        {
            if (peer_chrs[i] == NULL || peer_chrs[i]->val_handle != chr_val_handle) {
                continue;
            }
            if (0 == ble_uuid_cmp(uuid_ccc, &dsc->uuid)) {
                LLOGD("设置chr %d dsc handle %d", peer_chrs[i]->val_handle, dsc->handle);
                peer_dscs[i] = dsc->handle;
            }
        }
    }
    else {
        msg.arg1 = error->status;
        msg.arg2 = ble_gatt_chr_counter;
        luat_msgbus_put(&msg, 0);
        return 0;
    }
    return 0;
}

static int chr_disced(uint16_t conn_handle,
                            const struct ble_gatt_error *error,
                            const struct ble_gatt_chr *chr, void *arg) {
    rtos_msg_t msg = {.handler=luat_nimble_chr_disc_cb};
    struct ble_gatt_svc *service = (struct ble_gatt_svc *)arg;
    // LLOGD("chr_disced status %d", error->status);
    if (error->status == BLE_HS_EDONE) {
        // LLOGD("chr discovery done count %d", ble_gatt_chr_counter);
        int ret = ble_gattc_disc_all_dscs(g_ble_conn_handle, service->start_handle, service->end_handle, dsc_disced, service);
        LLOGD("ble_gattc_disc_all_dscs %d", ret);
        if (ret == 0)
            return 0;
        else {
            msg.arg1 = -80;
            msg.arg2 = ble_gatt_chr_counter;
            luat_msgbus_put(&msg, 0);
            return 0;
        }
    }
    if (error->status != 0) {
        msg.arg1 = error->status;
        luat_msgbus_put(&msg, 0);
        return error->status;
    }
    if (ble_gatt_chr_counter >= MAX_PER_SERV) {
        return 0; // 太多了
    }
    // char buff[64];
    for (size_t i = 0; i < MAX_PER_SERV; i++)
    {
        if (peer_servs[i] == NULL)
            continue;
        if (peer_servs[i] == arg) {
            if (peer_chrs[i*MAX_PER_SERV+ble_gatt_chr_counter] == NULL) {
                peer_chrs[i*MAX_PER_SERV+ble_gatt_chr_counter] = luat_heap_malloc(sizeof(struct ble_gatt_chr));
            }
            memcpy(peer_chrs[i*MAX_PER_SERV+ble_gatt_chr_counter], chr, sizeof(struct ble_gatt_chr));
            // LLOGD("特征值 %s flags %d", ble_uuid_to_str(&chr->uuid, buff), chr->properties);
            ble_gatt_chr_counter ++;
        }
    }
    return 0;
}

int luat_nimble_central_disc_chr(int id, struct ble_gatt_svc *service) {
    (void)id;
    ble_gatt_chr_counter = 0;
    return ble_gattc_disc_all_chrs(g_ble_conn_handle, service->start_handle, service->end_handle, chr_disced, service);
}

int luat_nimble_central_disc_dsc(int id, struct ble_gatt_svc *service, struct ble_gatt_chr *chr) {
    (void)id;
    // LLOGD("service %d %d chr %d %d", service->start_handle, service->end_handle, chr->val_handle, chr->def_handle);
    int ret = ble_gattc_disc_all_dscs(g_ble_conn_handle, service->start_handle, service->end_handle, dsc_disced, chr);
    LLOGD("ble_gattc_disc_all_dscs %d", ret);
    return ret;
    // return 0;
}

static int l_ble_chr_read_cb(lua_State* L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "BLE_GATT_READ_CHR");
        if (msg->ptr) {
            lua_pushlstring(L, msg->ptr, msg->arg2);
        }
        else {
            lua_pushnil(L);
        }
        // lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    if (ptr)
        luat_heap_free(ptr);
    return 0;
}

static int write_attr_cb(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg) {
    LLOGD("write_attr_cb %d", error->status);
    return 0;
}


static int read_attr_cb(uint16_t conn_handle,
                        const struct ble_gatt_error *error,
                        struct ble_gatt_attr *attr,
                        void *arg) {
    LLOGD("read_attr_cb %d", error->status);
    const struct os_mbuf *om = attr->om;
    
    rtos_msg_t msg = {.handler=l_ble_chr_read_cb};
    msg.arg1 = error->status;
    if (error->status == 0) {
        char* ptr = luat_heap_malloc(1024);
        size_t act = 0;
        while(om != NULL && ptr != NULL) {
            if (om->om_len > 0) {
                if (act + om->om_len > 1024) {
                    LLOGE("too many data, skip");
                    break;
                }
                memcpy(ptr + act, om->om_data, om->om_len);
                act += om->om_len;
            }
            om = SLIST_NEXT(om, om_next);
        }
        msg.arg2 = act;
        msg.ptr = ptr;
    }
    luat_msgbus_put(&msg, 0);
    return 0;
}

int luat_nimble_central_write(int id, struct ble_gatt_chr *chr, char* data, size_t data_len) {
    struct os_mbuf *om;
    int rc;
    om = ble_hs_mbuf_from_flat(data, data_len);

    if(om == NULL) {
        return BLE_HS_ENOMEM;
    }
    if (chr->properties & BLE_GATT_CHR_F_WRITE_NO_RSP)
        return ble_gattc_write_no_rsp(g_ble_conn_handle, chr->val_handle, om);
    else
        return ble_gattc_write(g_ble_conn_handle, chr->val_handle, om, write_attr_cb, NULL);
}

int luat_nimble_central_read(int id, struct ble_gatt_chr *chr) {
    return ble_gattc_read(g_ble_conn_handle, chr->val_handle, read_attr_cb, NULL);
}

int luat_nimble_central_subscribe(int id, struct ble_gatt_chr * chr, int onoff) {
    char buff[2] = {0, 0};
    if (onoff) {
        if (chr->properties & BLE_GATT_CHR_F_NOTIFY) {
            buff[0] |= 0x01;
        }
        if (chr->properties & BLE_GATT_CHR_F_INDICATE) {
            buff[0] |= 0x02;
        }
    }
    for (size_t i = 0; i < MAX_PER_SERV * MAX_PER_SERV; i++)
    {
        if (peer_chrs[i] == NULL || peer_chrs[i] != chr) {
            continue;
        }
        // LLOGD("subscribe %d %d %d", onoff, g_ble_conn_handle, peer_dscs[i]);
        return ble_gattc_write_flat(g_ble_conn_handle, 13, buff, 2, write_attr_cb, chr);
    }
    return -1;
}

static int luat_blecent_tx_cb(lua_State *L, void* ptr) {
rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "BLE_GATT_TX_DATA");
        if (msg->ptr) {
            lua_pushlstring(L, msg->ptr, msg->arg1);
        }
        else {
            lua_pushnil(L);
        }
        // lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    if (ptr)
        luat_heap_free(ptr);
    return 0;
}

static int blecent_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;
    struct ble_gap_conn_desc desc;
    int rc = 0;
    int i = 0;
    rtos_msg_t msg = {0};
    struct os_mbuf *om;
    size_t offset = 0;
    char* ptr;

    // LLOGD("blecent_gap_event %d", event->type);ble_gattc_disc_all_dscs

    switch (event->type) {
    case BLE_GAP_EVENT_DISC_COMPLETE:
        LLOGD("ble scan complete");
        return 0;
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0) {
            LLOGI("ble_hs_adv_parse_fields rc %d", rc);
            return 0;
        }
        // if (event->disc.event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        //         event->disc.event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        //     LLOGI("disc event_type Not ADC or DIR %d", event->disc.event_type);
        //     return 0;
        // }
        luat_nimble_scan_result_t* res = luat_heap_malloc(sizeof(luat_nimble_scan_result_t));
        if (res == NULL) {
            LLOGI("out of memory when malloc luat_nimble_scan_result_t");
            return 0;
        }
        memset(res, 0, sizeof(luat_nimble_scan_result_t));

        res->rssi = event->disc.rssi;

        // char tmpbuff[64] = {0};

        for (i = 0; i < fields.num_uuids16 && i < 16; i++) {
            // LLOGD("uuids_16 %s", ble_uuid_to_str(&fields.uuids16[i], tmpbuff));
            res->uuids_16[i] = fields.uuids16[i].value;
        }
        for (i = 0; i < fields.num_uuids32 && i < 16; i++) {
            // LLOGD("uuids_32 %s", ble_uuid_to_str(&fields.uuids32[i], tmpbuff));
            res->uuids_32[i] = fields.uuids32[i].value;
        }
        for (i = 0; i < fields.num_uuids128 && i < 16; i++) {
            // LLOGD("uuids_128 %s", ble_uuid_to_str(&fields.uuids128[i], tmpbuff));
            // memcpy(res->uuids_128[i], fields.uuids128[i].value, 16);
        }
        memcpy(res->addr, &event->disc.addr, 7);
        memcpy(res->name, fields.name, fields.name_len);
        if (fields.mfg_data_len) {
            memcpy(res->mfg_data, fields.mfg_data, fields.mfg_data_len);
            res->mfg_data_len = fields.mfg_data_len;
        }
        // LLOGD("addr %02X%02X%02X%02X%02X%02X", event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2], 
        //                                        event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);
        // for (i = 0; i < fields.num_uuids128 && i < 16; i++) {
        //     res->uuids_128[i] = fields.num_uuids128.value >> 32;
        // }
        // LLOGD("uuids 16=%d 32=%d 128=%d", fields.num_uuids16, fields.num_uuids32, fields.num_uuids128);
        msg.handler=luat_nimble_scan_cb;
        msg.ptr = res;
        luat_msgbus_put(&msg, 0);

        /* An advertisment report was received during GAP discovery. */
        print_adv_fields(&fields);

        /* Try to connect to the advertiser if it looks interesting. */
        //blecent_connect_if_interesting(&event->disc);
        return 0;
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        LLOGI("connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        msg.handler = luat_nimble_status_cb;
        msg.arg1 = event->connect.status;
        // msg.arg2 = event->connect.conn_handle;
        luat_msgbus_put(&msg, 0);
        if (event->connect.status == 0) {
            g_ble_conn_handle = event->connect.conn_handle;
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (rc == 0)
                bleprph_print_conn_desc(&desc);
            g_ble_state = BT_STATE_CONNECTED;
            /* Perform service discovery. */
            rc = ble_gattc_disc_all_svcs(event->connect.conn_handle, svc_disced, NULL);
        }
        else {
            g_ble_state = BT_STATE_DISCONNECT;
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        g_ble_state = BT_STATE_DISCONNECT;
        LLOGI("disconnect; reason=%d ", event->disconnect.reason);
        // bleprph_print_conn_desc(&event->disconnect.conn);
        msg.handler = luat_nimble_status_cb;
        msg.arg1 = 0xff;
        // msg.arg2 = event->connect.conn_handle;
        luat_msgbus_put(&msg, 0);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        LLOGI("connection updated; status=%d ", event->conn_update.status);
        // rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        // if (rc == 0)
        //     bleprph_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        LLOGI("encryption change event; status=%d ", event->enc_change.status);
        // rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        // if (rc == 0)
        //     bleprph_print_conn_desc(&desc);
        // LLOGI("");
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        LLOGI("subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        LLOGI("mtu update event; conn_handle=%d cid=%d mtu=%d",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        LLOGI("PASSKEY_ACTION_EVENT started");
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX :
        // LLOGD("BLE_GAP_EVENT_NOTIFY_RX");
        om = event->notify_rx.om;
        ptr = luat_heap_malloc(1024);
        if (ptr == NULL) {
            LLOGE("out of memory when malloc buff");
            return 0;
        }
        while(om != NULL) {
            if (offset + om->om_len > 1024) {
                LLOGD("data too big");
                break;
            }
            memcpy(ptr + offset, om->om_data, om->om_len);
            offset += om->om_len;
            om = SLIST_NEXT(om, om_next);
        }
        msg.handler = luat_blecent_tx_cb;
        msg.arg1 = offset;
        msg.ptr = ptr;
        msg.arg2 = event->notify_rx.attr_handle;
        // msg.arg2 = event->connect.conn_handle;
        luat_msgbus_put(&msg, 0);
        return 0;
    }

    return 0;
}

static void
blecent_on_reset(int reason)
{
    g_ble_state = BT_STATE_OFF;
    LLOGE("Resetting state; reason=%d", reason);
    //app_adapter_state_changed_callback(WM_BT_STATE_OFF);
}


static void
blecent_on_sync(void)
{
    // int rc;

    /* Make sure we have proper identity address set (public preferred) */
    ble_hs_util_ensure_addr(0);
}



int luat_nimble_init_central(uint8_t uart_idx, char* name, int mode) {
    // int rc = 0;
    nimble_port_init();

    if (name == NULL || strlen(name) == 0) {
        if (selfname[0] == 0) {
            memcpy(selfname, "LuatOS", strlen("LuatOS") + 1);
        }
    }
    else {
        memcpy(selfname, name, strlen(name) + 1);
    }

    /* Set the default device name. */
    if (strlen(selfname))
        ble_svc_gap_device_name_set((const char*)selfname);


    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_sc = 0;


    ble_svc_gap_init();
    ble_svc_gatt_init();

    // rc = gatt_svr_init();
    // LLOGD("gatt_svr_init rc %d", rc);

    /* XXX Need to have template for store */
    ble_store_config_init();

    return 0;
}

//-----------------------------------
//            helper
//-----------------------------------

/**
 * Utility function to log an array of bytes.
 */
void
print_bytes(const uint8_t *bytes, int len)
{
    int i;
    char buff[256 + 1] = {0};

    for (i = 0; i < len; i++) {
        sprintf_(buff + strlen(buff), "%02X", bytes[i]);
        // LLOGD("%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
    LLOGD("%s", buff);
}

char *
addr_str(const void *addr)
{
    static char buf[6 * 2 + 5 + 1];
    const uint8_t *u8p;

    u8p = addr;
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);

    return buf;
}

void
print_uuid(const ble_uuid_t *uuid)
{
    char buf[BLE_UUID_STR_LEN];

    LLOGD("%s", ble_uuid_to_str(uuid, buf));
}


/**
 * Logs information about a connection to the console.
 */
void
print_conn_desc(const struct ble_gap_conn_desc *desc)
{
    LLOGD("handle=%d our_ota_addr_type=%d our_ota_addr=%s ",
                desc->conn_handle, desc->our_ota_addr.type,
                addr_str(desc->our_ota_addr.val));
    LLOGD("our_id_addr_type=%d our_id_addr=%s ",
                desc->our_id_addr.type, addr_str(desc->our_id_addr.val));
    LLOGD("peer_ota_addr_type=%d peer_ota_addr=%s ",
                desc->peer_ota_addr.type, addr_str(desc->peer_ota_addr.val));
    LLOGD("peer_id_addr_type=%d peer_id_addr=%s ",
                desc->peer_id_addr.type, addr_str(desc->peer_id_addr.val));
    LLOGD("conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}


void
print_adv_fields(const struct ble_hs_adv_fields *fields)
{
    // char s[BLE_HS_ADV_MAX_SZ];
    // const uint8_t *u8p;
    // int i;

    // if (fields->flags != 0) {
    //     LLOGD("    flags=0x%02x", fields->flags);
    // }

    // if (fields->uuids16 != NULL) {
    //     LLOGD("    uuids16(%scomplete)=",
    //                 fields->uuids16_is_complete ? "" : "in");
    //     for (i = 0; i < fields->num_uuids16; i++) {
    //         print_uuid(&fields->uuids16[i].u);
    //         //LLOGD(" ");
    //     }
    // }

    // if (fields->uuids32 != NULL) {
    //     LLOGD("    uuids32(%scomplete)=",
    //                 fields->uuids32_is_complete ? "" : "in");
    //     for (i = 0; i < fields->num_uuids32; i++) {
    //         print_uuid(&fields->uuids32[i].u);
    //         //LLOGD(" ");
    //     }
    // }

    // if (fields->uuids128 != NULL) {
    //     LLOGD("    uuids128(%scomplete)=",
    //                 fields->uuids128_is_complete ? "" : "in");
    //     for (i = 0; i < fields->num_uuids128; i++) {
    //         print_uuid(&fields->uuids128[i].u);
    //         LLOGD(" ");
    //     }
    //     //LLOGD("");
    // }

    // if (fields->name != NULL) {
    //     assert(fields->name_len < sizeof s - 1);
    //     memcpy(s, fields->name, fields->name_len);
    //     s[fields->name_len] = '\0';
    //     LLOGD("    name(%scomplete)=%s",
    //                 fields->name_is_complete ? "" : "in", s);
    // }

    // if (fields->tx_pwr_lvl_is_present) {
    //     LLOGD("    tx_pwr_lvl=%d", fields->tx_pwr_lvl);
    // }

    // if (fields->slave_itvl_range != NULL) {
    //     LLOGD("    slave_itvl_range=");
    //     print_bytes(fields->slave_itvl_range, BLE_HS_ADV_SLAVE_ITVL_RANGE_LEN);
    // }

    // if (fields->svc_data_uuid16 != NULL) {
    //     LLOGD("    svc_data_uuid16=");
    //     print_bytes(fields->svc_data_uuid16, fields->svc_data_uuid16_len);
    // }

    // if (fields->public_tgt_addr != NULL) {
    //     LLOGD("    public_tgt_addr=");
    //     u8p = fields->public_tgt_addr;
    //     for (i = 0; i < fields->num_public_tgt_addrs; i++) {
    //         LLOGD("public_tgt_addr=%s ", addr_str(u8p));
    //         u8p += BLE_HS_ADV_PUBLIC_TGT_ADDR_ENTRY_LEN;
    //     }
    //     // LLOGD("");
    // }

    // if (fields->appearance_is_present) {
    //     LLOGD("    appearance=0x%04x", fields->appearance);
    // }

    // if (fields->adv_itvl_is_present) {
    //     LLOGD("    adv_itvl=0x%04x", fields->adv_itvl);
    // }

    // if (fields->svc_data_uuid32 != NULL) {
    //     LLOGD("    svc_data_uuid32=");
    //     print_bytes(fields->svc_data_uuid32, fields->svc_data_uuid32_len);
    //     LLOGD("");
    // }

    // if (fields->svc_data_uuid128 != NULL) {
    //     LLOGD("    svc_data_uuid128=");
    //     print_bytes(fields->svc_data_uuid128, fields->svc_data_uuid128_len);
    //     LLOGD("");
    // }

    // if (fields->uri != NULL) {
    //     LLOGD("    uri=");
    //     print_bytes(fields->uri, fields->uri_len);
    //     LLOGD("");
    // }

    // if (fields->mfg_data != NULL) {
    //     LLOGD("    mfg_data=");
    //     print_bytes(fields->mfg_data, fields->mfg_data_len);
    //     LLOGD("");
    // }
}

