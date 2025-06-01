
/*
@module  wlan
@summary wifi操作
@catalog 外设API
@version 1.0
@date    2022.09.30
@demo wlan
@tag LUAT_USE_WLAN
@usage
--[[
提醒:
对于仅支持wifiscan的模块, 仅 init/scan/scanResult 函数是可用的

例如: Air780E/Air600E/Air780EG等仅支持wifiscan
]]

*/

#include "luat_base.h"
#include "luat_wlan.h"

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

uint32_t ipaddr_addr(const char *cp);

static inline void to_ipv4(const char* data, uint8_t* dst) {
    uint32_t tmpip = ipaddr_addr(data);
    dst[3] = (tmpip >> 24) & 0xFF;
    dst[2] = (tmpip >> 16) & 0xFF;
    dst[1] = (tmpip >> 8) & 0xFF;
    dst[0] = (tmpip >> 0) & 0xFF;
}

/*
初始化
@api wlan.init()
@return bool 成功返回true,否则返回false
*/
static int l_wlan_init(lua_State* L){
    int ret = luat_wlan_init(NULL);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*
设置wifi模式
@api wlan.setMode(mode)
@int wifi模式
@return bool 成功返回true,否则返回false
@usage
-- 设置为AP模式, 广播ssid, 接收wifi客户端的链接
wlan.setMode(wlan.AP)

-- 设置为STATION模式, 也是初始化后的默认模式
wlan.setMode(wlan.STATION)

-- 混合模式, 做AP又做STATION
wlan.setMode(wlan.APSTA)
*/
static int l_wlan_mode(lua_State* L){
    int mode = LUAT_WLAN_MODE_STA;
    if (lua_isinteger(L, 1)) {
        mode = lua_tointeger(L, 1);
    }
    else if (lua_isinteger(L, 2)) {
        mode = lua_tointeger(L, 2);
    }

    if (mode <= LUAT_WLAN_MODE_NULL || mode >= LUAT_WLAN_MODE_MAX) {
        mode = LUAT_WLAN_MODE_STA;
    }

    // switch (mode)
    // {
    // case LUAT_WLAN_MODE_NULL:
    //     LLOGD("wlan mode NULL");
    //     break;
    // case LUAT_WLAN_MODE_STA:
    //     LLOGD("wlan mode STATION");
    //     break;
    // case LUAT_WLAN_MODE_AP:
    //     LLOGD("wlan mode AP");
    //     break;
    // case LUAT_WLAN_MODE_APSTA:
    //     LLOGD("wlan mode AP-STATION");
    //     break;
    
    // default:
    //     break;
    // }
    luat_wlan_config_t conf = {
        .mode = mode
    };
    int ret = luat_wlan_mode(&conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*
作为STATION时,是否已经连接上AP,且获取IP成功
@api wlan.ready()
@return bool 已经连接成功返回true,否则返回false
*/
static int l_wlan_ready(lua_State* L){
    lua_pushboolean(L, luat_wlan_ready());
    return 1;
}

/*
作为STATION时,连接到指定AP
@api wlan.connect(ssid, password, auto_reconnect)
@string AP的ssid
@string AP的password,可选
@int    0关闭自动重连,1开启自动重连.当前强制开启自动重连
@return bool 发起连接成功返回true,否则返回false.注意,不代表连接AP成功!!
@usage

-- 普通模式,带密码
wlan.connect("myap", "12345678")
-- 普通模式,不带密码
wlan.connect("myap")
-- 特殊模式, 重用之前的ssid和密码,本次直接连接
-- 注意, 前提是本次上电后已经传过ssid和或password,否则必失败
wlan.connect()
*/
static int l_wlan_connect(lua_State* L){
    const char* ssid = luaL_optstring(L, 1, "");
    const char* password = luaL_optstring(L, 2, "");
    luat_wlan_conninfo_t info = {0};
    info.auto_reconnection = 1;
    memcpy(info.ssid, ssid, strlen(ssid));
    memcpy(info.password, password, strlen(password));

    int ret = luat_wlan_connect(&info);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*
作为STATION时,断开AP
@api wlan.disconnect()
*/
static int l_wlan_disconnect(lua_State* L){
    (void)L;
    luat_wlan_disconnect();
    return 0;
}

/*
扫描wifi频段
@api wlan.scan()
@usage
-- 注意, wlan.scan()是异步API,启动扫描后会马上返回

-- wifi扫描成功后, 会有WLAN_SCAN_DONE消息, 读取即可
sys.subscribe("WLAN_SCAN_DONE", function ()
    local results = wlan.scanResult()
    log.info("scan", "results", #results)
    for k,v in pairs(results) do
        log.info("scan", v["ssid"], v["rssi"], (v["bssid"]:toHex()))
    end
end)

-- 下面演示的是初始化wifi后定时扫描,请按实际业务需求修改
sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    while 1 do
        wlan.scan()
        sys.wait(15000)
    end
end)
*/
static int l_wlan_scan(lua_State* L){
    (void)L;
    luat_wlan_scan();
    return 0;
}

/*
获取wifi扫描结果
@api wlan.scanResult()
@return table 扫描结果
@usage
-- 用法请查阅 wlan.scan() 函数
*/
static int l_wlan_scan_result(lua_State* L) {
    int ap_limit = luaL_optinteger(L, 1, 20);
    if (ap_limit > 32)
        ap_limit = 32;
    else if (ap_limit < 8)
        ap_limit = 8;
    lua_newtable(L);
    luat_wlan_scan_result_t *results = luat_heap_malloc(sizeof(luat_wlan_scan_result_t) * ap_limit);
    if (results == NULL) {
        LLOGE("out of memory when malloc scan result");
        return 1;
    }
    memset(results, 0, sizeof(luat_wlan_scan_result_t) * ap_limit);
    int len = luat_wlan_scan_get_result(results, ap_limit);
    for (int i = 0; i < len; i++)
    {
        lua_newtable(L);

        lua_pushstring(L, (const char *)results[i].ssid);
        lua_setfield(L, -2, "ssid");

        // lua_pushfstring(L, "%02X%02X%02X%02X%02X%02X", results[i].bssid[0], 
        //                                                results[i].bssid[1], 
        //                                                results[i].bssid[2], 
        //                                                results[i].bssid[3], 
        //                                                results[i].bssid[4], 
        //                                                results[i].bssid[5]);
        lua_pushlstring(L, (const char *)results[i].bssid, 6);
        lua_setfield(L, -2, "bssid");

        lua_pushinteger(L, results[i].ch);
        lua_setfield(L, -2, "channel");

        lua_pushinteger(L, results[i].rssi);
        lua_setfield(L, -2, "rssi");

        lua_seti(L, -2, i + 1);
    }
    luat_heap_free(results);
    return 1;
}

/*
配网
@api wlan.smartconfig(mode)
@int 配网模式, 默认为esptouch, 若传0则主动停止配网
@return bool 启动成功或停止成功, 返回true, 否则返回false
@usage
wlan.smartconfig()
local ret, ssid, passwd = sys.waitUntil("SC_RESULT", 180*1000) -- 最多等3分钟
log.info("sc", ret, ssid, passwd)
-- 详细用法请查看demo
*/
static int l_wlan_smartconfig(lua_State *L) {
    int tp = luaL_optinteger(L, 1, LUAT_SC_TYPE_ESPTOUCH);
    if (tp == LUAT_SC_TYPE_STOP) {
        luat_wlan_smartconfig_stop();
        lua_pushboolean(L, 1);
    }
    else {
        int ret = luat_wlan_smartconfig_start(tp);
        lua_pushboolean(L, ret == 0 ? 1 : 0);
    }
    return 1;
}

/*
获取mac
@api wlan.getMac(tp, hexstr)
@int 设置何种mac地址,对ESP32系列来说,只能设置STA的地址,即0,默认值也是0
@bool 是否转HEX字符, 默认是true,即输出hex字符串
@return string MAC地址,十六进制字符串形式 "AABBCCDDEEFF" 或原始数据

log.info("wlan mac", wlan.getMac())
*/
static int l_wlan_get_mac(lua_State* L){
    char tmp[6] = {0};
    char tmpbuff[16] = {0};
    luat_wlan_get_mac(luaL_optinteger(L, 1, 0), tmp);
    if (lua_isboolean(L, 2) && !lua_toboolean(L, 2)) {
        lua_pushlstring(L, tmp, 6);
    }
    else {
        sprintf_(tmpbuff, "%02X%02X%02X%02X%02X%02X", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
        lua_pushstring(L, tmpbuff);
    }
    return 1;
}


/*
设置mac
@api wlan.setMac(tp, mac)
@int 设置何种mac地址,对ESP32系列来说,只能设置STA的地址,即0
@string 待设置的MAC地址,长度6字节
@return bool 成功返回true,否则返回false
@usage
-- 设置MAC地址, 2023-03-01之后编译的固件可用
local mac = string.fromHex("F01122334455")
wlan.setMac(0, mac)

-- 部分模块支持恢复默认MAC, 例如esp32系列
-- 在2023-11-01 之后编译的固件可用
local mac = string.fromHex("000000000000")
wlan.setMac(0, mac)
*/
static int l_wlan_set_mac(lua_State* L){
    // int id = luaL_optinteger(L, 1, 0);
    const char* mac = luaL_checkstring(L, 2);
    int ret = luat_wlan_set_mac(luaL_optinteger(L, 1, 0), mac);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


/*
获取ip,仅STATION或APSTA模式下有意义
@api wlan.getIP()
@return string ip地址,当前仅返回ipv4地址,例如 "192.168.1.25"
*/
static int l_wlan_get_ip(lua_State* L){
    char tmpbuff[16] = {0};
    luat_wlan_get_ip(luaL_optinteger(L, 1, 0), tmpbuff);
    lua_pushstring(L, tmpbuff);
    return 1;
}

/*
启动AP
@api wlan.createAP(ssid, passwd, gateway, netmask, channel)
@string AP的SSID,必填
@string AP的密码,可选
@string AP的网关地址, 默认192.168.4.1
@string AP的网关掩码, 默认255.255.255.0
@int    AP建立的通道, 默认6
@return bool 成功创建返回true,否则返回false
@usage
-- 注意, 调用本AP时,若wifi模式为STATION,会自动切换成 APSTA
wlan.createAP("uiot", "12345678")
-- 设置网关IP,掩码, 通道, 2023.7.13 新增, BSP未必支持
-- wlan.createAP("uiot", "12345678", "192.168.4.1", "255.255.255.0", 6)
*/
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
static int l_wlan_ap_start(lua_State *L) {
    size_t ssid_len = 0;
    size_t password_len = 0;
    luat_wlan_apinfo_t apinfo = {0};
    const char* ssid = luaL_checklstring(L, 1, &ssid_len);
    const char* password = luaL_optlstring(L, 2, "", &password_len);
    const char* gateway = luaL_optstring(L, 3, "192.168.4.1");
    const char* netmask = luaL_optstring(L, 4, "255.255.255.0");
    if (strlen(gateway) > 7) {
        to_ipv4(gateway,  apinfo.gateway);
    }
    if (strlen(netmask) > 7) {
        to_ipv4(netmask,  apinfo.netmask);
    }

    apinfo.channel = (uint8_t)luaL_optinteger(L, 5, 6);
    if (ssid_len < 1) {
        LLOGE("ssid MUST NOT EMTRY");
        return 0;
    }
    if (ssid_len > 32) {
        LLOGE("ssid too long [%s]", ssid);
        return 0;
    }
    if (password_len > 63) {
        LLOGE("password too long [%s]", password);
        return 0;
    }

    memcpy(apinfo.ssid, ssid, ssid_len);
    memcpy(apinfo.password, password, password_len);

    int ret = luat_wlan_ap_start(&apinfo);
    LLOGD("apstart ret %d", ret);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
关闭AP功能
@api wlan.stopAP()
@return bool 成功创建返回true,否则返回false
@usage
wlan.stopAP()
*/
static int l_wlan_ap_stop(lua_State *L) {
    int ret = luat_wlan_ap_stop();
    LLOGD("apstop ret %d", ret);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*
获取信息,如AP的bssid,信号强度
@api wlan.getInfo()
@return table 详情,键值对形式
@usage

log.info("wlan", "info", json.encode(wlan.getInfo()))
--[[
典型输出
{
    "bssid" : "xxxxxx",
    "rssi"  : -89,
    "gw" : "192.168.1.1"
}
]]
*/
static int l_wlan_get_info(lua_State *L) {
    char buff[48] = {0};
    char buff2[32] = {0};
    lua_newtable(L);

    luat_wlan_get_ap_bssid(buff);
    sprintf_(buff2, "%02X%02X%02X%02X%02X%02X", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
    lua_pushstring(L, buff2);
    lua_setfield(L, -2, "bssid");

    memset(buff, 0, 48);
    luat_wlan_get_ap_gateway(buff);
    lua_pushstring(L, buff);
    lua_setfield(L, -2, "gw");

    lua_pushinteger(L, luat_wlan_get_ap_rssi());
    lua_setfield(L, -2, "rssi");

    return 1;
}

/*
读取或设置省电模式
@api wlan.powerSave(mode)
@int 省电模式,可选, 传入就是设置, 例如wlan.PS_NONE
@return int 当前省电模式/设置后的省电模式
@usage
-- 请查阅常量表  PS_NONE/PS_MIN_MODEM/PS_MAX_MODEM
log.info("wlan", "PS", wlan.powerSave(wlan.PS_NONE))
-- 本API于 2023.03.31 新增
*/
static int l_wlan_powerSave(lua_State *L) {
    int mode = 0;
    if (lua_isinteger(L, 1)) {
        mode = luaL_checkinteger(L, 1);
        luat_wlan_set_ps(mode);
    }
    mode = luat_wlan_get_ps();
    lua_pushinteger(L, mode);
    return 1;
}

/*
读取或设置Hostname
@api wlan.hostname(new_name)
@string 新的hostname,可选, 传入就是设置
@return string 当前的hostname或者设置后的hostname
@usage
-- 本API于 2023.07.23 新增
-- 本函数应该在wlan.init之前设置好, 最晚应早于wlan.connect
-- hostname的默认值是  "LUATOS_" + 设备的MAC值
-- 例如: LUATOS_0022EECC2399

wlan.hostname("我的wifi物联网设备")
*/
static int l_wlan_get_set_hostname(lua_State *L) {
    if (lua_isstring(L, 1)) {
        size_t len = 0;
        const char* hostname = luaL_checklstring(L, 1, &len);
        if (len > 0) {
            if (len > 31) {
                LLOGE("hostname is too long");
                return 0;
            }
            luat_wlan_set_hostname(0, hostname);
        }
    }
    const char* tmp = luat_wlan_get_hostname(0);
    lua_pushstring(L, tmp);
    return 1;
}

/*
设置Station模式下的IP获取模式
@api wlan.staIp(dhcp_enable, ip, netmask, gateway)
@bool 是否启用DHCP,默认是true
@string 本机IP地址,例如192.168.2.200, 禁用DHCP时必填
@string 本机IP掩码,例如255.255.255.0, 禁用DHCP时必填
@string 本机IP网关,例如192.168.2.1, 禁用DHCP时必填
@return bool 成功返回true,否则返回false
@usage
-- 本API于 2023.10.06 新增
-- 本函数需要在wlan.init之后才允许调用

-- 启用DHCP, 默认也是启用DHCP,这里是演示API使用
wlan.staIp(true)
-- 禁用DHCP,自行设置IP/掩码/网关
wlan.staIp(false, "192.168.2.200", "255.255.255.0", "192.168.2.1")
*/
static int l_wlan_set_sta_ip(lua_State *L) {
    luat_wlan_station_info_t info = {
        .dhcp_enable = 1
    };
    const char *data = NULL;
    size_t len = 0;
    // 是否DHCP
    if (lua_isinteger(L, 1))
        info.dhcp_enable = luaL_optinteger(L, 1, 1);
    else if (lua_isboolean(L, 1))
        info.dhcp_enable = lua_toboolean(L, 1);

    // 本地IP
    data = luaL_optlstring(L, 2, "192.168.1.201", &len);
    to_ipv4(data, info.ipv4_addr);

    // 掩码
    data = luaL_optlstring(L, 3, "255.255.255.0", &len);
    to_ipv4(data, info.ipv4_netmask);

    // 网关
    data = luaL_optlstring(L, 4, "192.168.1.1", &len);
    to_ipv4(data, info.ipv4_gateway);

    int ret = luat_wlan_set_station_ip(&info);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_wlan[] =
{
    { "init",               ROREG_FUNC(l_wlan_init)},
    { "scan",               ROREG_FUNC(l_wlan_scan)},
    { "scanResult",         ROREG_FUNC(l_wlan_scan_result)},
#ifndef LUAT_USE_WLAN_SCANONLY
    { "mode",               ROREG_FUNC(l_wlan_mode)},
    { "setMode",            ROREG_FUNC(l_wlan_mode)},
    { "ready",              ROREG_FUNC(l_wlan_ready)},
    { "connect",            ROREG_FUNC(l_wlan_connect)},
    { "disconnect",         ROREG_FUNC(l_wlan_disconnect)},

    // 配网相关
    { "smartconfig",         ROREG_FUNC(l_wlan_smartconfig)},

    { "getIP",               ROREG_FUNC(l_wlan_get_ip)},
    { "getInfo",             ROREG_FUNC(l_wlan_get_info)},
    { "getMac",              ROREG_FUNC(l_wlan_get_mac)},
    { "setMac",              ROREG_FUNC(l_wlan_set_mac)},
    { "hostname",            ROREG_FUNC(l_wlan_get_set_hostname)},

    { "powerSave",           ROREG_FUNC(l_wlan_powerSave)},

    { "staIp",               ROREG_FUNC(l_wlan_set_sta_ip)},

    // AP相关
    { "createAP",            ROREG_FUNC(l_wlan_ap_start)},
    { "stopAP",              ROREG_FUNC(l_wlan_ap_stop)},
    // wifi模式
    //@const NONE WLAN模式,停用
    {"NONE",                ROREG_INT(LUAT_WLAN_MODE_NULL)},
    //@const STATION WLAN模式,STATION模式,主动连AP
    {"STATION",             ROREG_INT(LUAT_WLAN_MODE_STA)},
    //@const AP WLAN模式,AP模式,接受STATION连接
    {"AP",                  ROREG_INT(LUAT_WLAN_MODE_AP)},
    //@const AP WLAN模式,混合模式
    {"STATIONAP",           ROREG_INT(LUAT_WLAN_MODE_APSTA)},

    // 配网模式
    //@const STOP 停止配网
    {"STOP",                ROREG_INT(LUAT_SC_TYPE_STOP)},
    //@const ESPTOUCH esptouch配网, V1
    {"ESPTOUCH",            ROREG_INT(LUAT_SC_TYPE_ESPTOUCH)},
    //@const AIRKISS Airkiss配网, 微信常用
    {"AIRKISS",             ROREG_INT(LUAT_SC_TYPE_AIRKISS)},
    //@const ESPTOUCH_AIRKISS esptouch和Airkiss混合配网
    {"ESPTOUCH_AIRKISS",    ROREG_INT(LUAT_SC_TYPE_ESPTOUCH_AIRKISS)},
    //@const ESPTOUCH_V2 esptouch配网, V2, 未测试
    {"ESPTOUCH_V2",         ROREG_INT(LUAT_SC_TYPE_ESPTOUCH_V2)},

    //@const PS_NONE 关闭省电模式
    {"PS_NONE",             ROREG_INT(0)},
    //@const PS_MIN_MODEM 最小Modem省电模式
    {"PS_MIN_MODEM",        ROREG_INT(1)},
    //@const PS_MAX_MODEM 最大Modem省电模式
    {"PS_MAX_MODEM",        ROREG_INT(2)},

#endif
	{ NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_wlan( lua_State *L ) {
    luat_newlib2(L, reg_wlan);
    return 1;
}
