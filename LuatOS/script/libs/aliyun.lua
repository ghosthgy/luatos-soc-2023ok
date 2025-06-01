-- PROJECT = "aliyundemo"
-- VERSION = "1.0.0"
-- local sys = require "sys"
-- sys库是标配
_G.sys = require("sys")
--[[特别注意, 使用mqtt库需要下列语句]]
_G.sysplus = require("sysplus")

aliyun = {}

local ClientId,PassWord,UserName,SetClientidFnc,SetDeviceTokenFnc,SetDeviceSecretFnc

local OutQueue =
{
    SUBSCRIBE = {},
    PUBLISH = {},
}

local Item = {}

local EvtCb = {}

local mqttc = nil

local Key,Dname 

--添加
local function insert(type,topic,qos,payload,cbFnc,cbPara)
    table.insert(OutQueue[type],{t=topic,q=qos,p=payload,cb=cbFnc,para=cbPara})
end

--删除
local function remove(type)
    if #OutQueue[type]>0 then return table.remove(OutQueue[type],1) end
end

--订阅步骤
local function procSubscribe(client)
        local i

        for i=1,#OutQueue["SUBSCRIBE"] do
            if not client:subscribe(OutQueue["SUBSCRIBE"][i].t , OutQueue["SUBSCRIBE"][i].q) then
                OutQueue["SUBSCRIBE"] = {}
                return false,"procSubscribe"
            end
        end
        OutQueue["SUBSCRIBE"] = {}
        return true
end

--发布
local function procSend(client)
    if not procSubscribe(client) then
        return false,"procSubscribe"
    end

    if #OutQueue["PUBLISH"] == 0 then
        sys.waitUntil("ALIYUN_PUB")
    end

    while #OutQueue["PUBLISH"] > 0 do
        Item = table.remove(OutQueue["PUBLISH"],1)
        local result = client:publish(Item.t,Item.p,Item.q)
        if type(result) == nil then
            if Item.cb then Item.cb(false,Item.para) end
        else
            local result, data = sys.waitUntil("PUB_SENT")
            if Item.cb then Item.cb(data,Item.para) end
        end
    end
    return true,"procSend"
end

--二次连接
-- local function clientDataTask(ClientId,user,PassWord,mqtt_host,mqtt_port,mqtt_isssl,DeviceName,ProductKey)
local function clientDataTask(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl,passtoken,Registration)

    sys.taskInit(function()
        if not Registration then
            local client_id,user_name,password = iotauth.aliyun(ProductKey,DeviceName,SetDeviceSecretFnc)
            mqttc = mqtt.create(nil,mqtt_host, mqtt_port,mqtt_isssl)  --mqtt客户端创建
            mqttc:auth(client_id,user_name,password) --mqtt三元组配置
        else
            mqttc = mqtt.create(nil,mqtt_host, mqtt_port,mqtt_isssl)  --mqtt客户端创建
            mqttc:auth(DeviceName,ProductKey,passtoken) --mqtt三元组配置
        end

        mqttc:keepalive(30) -- 默认值240s
        mqttc:autoreconn(true, 20000) -- 自动重连机制
        mqttc:connect()
        mqttc:on(mqtt_cbevent)  --mqtt回调注册

        local conres = sys.waitUntil("mqtt_conack",30000)
        if mqttc:ready() and conres then
            -- if connectCb then connectCb(true,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](true) end

            local result,prompt = procSubscribe(mqttc)
            if result then
                while true do
                    procSend(mqttc)
                end
            end

            -- if connectCb then connectCb(false,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](false) end
        end
    end)
end

--根据返回的数据进行二次加密
local function directProc(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl,Registration)
    if not Registration then
        local ClientId = DeviceName.."|securemode=3,signmethod=hmacmd5,timestamp=789|"
        local UserName = DeviceName.."&"..ProductKey
        
        local content = "ClientId"..DeviceName.."deviceName"..DeviceName.."productKey"..ProductKey.."timestamp789"
        local signKey= SetDeviceSecretFnc
        PassWord = crypto.hmac_md5(content,signKey)
        
        clientDataTask(ClientId,UserName,PassWord,mqtt_host,mqtt_port,mqtt_isssl,DeviceName,ProductKey)
    else
        local ClientId = SetClientidFnc.."|securemode=-2,authType=connwl|"
        local UserName = DeviceName.."&"..ProductKey
        local PassWord = SetDeviceTokenFnc
        
        clientDataTask(ClientId,UserName,mqtt_host,mqtt_port,mqtt_isssl,PassWord,Registration)
    end
end

--获取预注册和免预注册一型一密一次连接返回的数据
local function clientEncryptionTask(Registration,DeviceName,ProductKey,ProductSecret,InstanceId,mqtt_host,mqtt_port,mqtt_isssl)
    sys.taskInit(function()
        local tm = os.time()
        --一型一密
            --预注册
            if not Registration then
                ClientId = DeviceName.."|securemode=2,authType=register,random="..tm..",signmethod=hmacmd5|"
            --免预注册
            else
                ClientId = DeviceName.."|securemode=-2,authType=regnwl,random="..tm..",signmethod=hmacmd5,instanceId="..InstanceId.."|"
            end
            UserName = DeviceName.."&"..ProductKey
            local content = "deviceName"..DeviceName.."productKey"..ProductKey.."random"..tm
            PassWord = crypto.hmac_md5(content,ProductSecret)
            
            local mqttClient = mqtt.create(nil,mqtt_host,mqtt_port,mqtt_isssl)  --客户端创建
            mqttClient:auth(ClientId,UserName,PassWord) --三元组配置
            mqttClient:on(function(mqtt_client, event, data, payload)  --mqtt回调注册
                -- 用户自定义代码
                if event == "conack" then

                elseif event == "recv" then
                    log.info("mqtt", "downlink", "topic", data, "payload", payload)
                    if payload then
                        local tJsonDecode,res = json.decode(payload)
                        if not Registration then
                            --预注册
                            if res and tJsonDecode["deviceName"] and tJsonDecode["deviceSecret"] then
                                
                                SetDeviceSecretFnc = tJsonDecode["deviceSecret"]
                                mqttClient:disconnect()
                                -- directProc(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl,Registration)
                                clientDataTask(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl)
                            end
                        else
                             --免预注册
                            if res and tJsonDecode["deviceName"] and tJsonDecode["deviceToken"] then
                                SetDeviceTokenFnc = tJsonDecode["deviceToken"]
                                SetClientidFnc = tJsonDecode["clientId"]
                                mqttClient:disconnect()
                                directProc(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl,Registration)
                            end
                        end
                        
                    end
                elseif event == "sent" then
                    log.info("mqtt", "sent", "pkgid", data)
                end
            end)
    
            mqttClient:connect()
    end)
end


--底层libMQTT回调函数，上层的回调函数，通过 aliyun.on注册
function mqtt_cbevent(mqtt_client, event, data, payload,metas)
    if event == "conack" then
        sys.publish("mqtt_conack")
        EvtCb["connect"](true) 
    elseif event == "recv" then -- 服务器下发的数据
        log.info("mqtt", "downlink", "topic", data, "payload", payload,"qos",metas.qos,"retain",metas.retain,"dup",metas.dup)

        -- log.info("aliyun.procReceive",data,string.toHex(payload))
        -- --OTA消息
        -- if data =="/ota/device/upgrade/"..Key.."/"..Dname then
        --     -- if aliyun and aliyun.upgrade then
        --         -- log.info("数据开始传进去upgrade",payload)
        --         upgrade(payload)
        --     -- end
        -- end

        if EvtCb["receive"] then
            EvtCb["receive"](data, payload,metas.qos,metas.retain,metas.dup)
        end
    elseif event == "sent" then
        if type(data) == "number" then
            sys.publish("PUB_SENT",true)
        end

    elseif event == "disconnect" then
        log.info("mqtt", "disconnect")

        sys.publish("PUB_SENT",false)

        if EvtCb["connect"] then 
            EvtCb["connect"](false) 
        end
    end
end


--[[
配置阿里云物联网套件的产品信息和设备信息
@api aliyun.setup(tPara)
@table tPara，填写设备信息的表函数
@usage
一机一密认证方案时，ProductSecret参数传入nil
一型一密认证方案时，ProductSecret参数传入真实的产品密钥
tPara[Registration] ,是否是预注册 已预注册为false，未预注册为true
tPara[DeviceName] ,设备名称
tPara[ProductKey] ,产品key
tPara[ProductSecret] ,产品secret，根据此信息判断是一机一密还是一型一密
tPara[DeviceSecret] ,设备secret
tPara[InstanceId] ,如果没有注册需要填写实例id，在实例详情页面
tPara[mqtt_port] ,mqtt端口
tPara[mqtt_isssl] ,是否使用ssl加密连接，true为无证书最简单的加密
]]
function aliyun.setup(tPara)
    mqtt_host = tPara.host or tPara.InstanceId..".mqtt.iothub.aliyuncs.com"
    if tPara.ProductSecret == "" or tPara.ProductSecret == nil then
        confiDentialTask(tPara.DeviceName,tPara.ProductKey,tPara.DeviceSecret,mqtt_host,tPara.mqtt_port,tPara.mqtt_isssl)
    else
        clientEncryptionTask(tPara.Registration,tPara.DeviceName,tPara.ProductKey,tPara.ProductSecret,tPara.InstanceId,mqtt_host,tPara.mqtt_port,tPara.mqtt_isssl)
    end
end

--一机一密连接  confiDentialTask
function confiDentialTask(DeviceName,ProductKey,DeviceSecret,mqtt_host,mqtt_port,mqtt_isssl)
    Key = ProductKey
    Dname = DeviceName
    sys.taskInit(function()
        local client_id,user_name,password = iotauth.aliyun(ProductKey,DeviceName,DeviceSecret)
        mqttc = mqtt.create(nil,mqtt_host, mqtt_port,mqtt_isssl)  --mqtt客户端创建
        mqttc:auth(client_id,user_name,password) --mqtt三元组配置


        mqttc:keepalive(30) -- 默认值240s
        mqttc:autoreconn(true, 20000) -- 自动重连机制
        mqttc:connect()
        mqttc:on(mqtt_cbevent)  --mqtt回调注册

        local conres = sys.waitUntil("mqtt_conack",30000)
        if mqttc:ready() and conres then
            -- if connectCb then connectCb(true,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](true) end

            local result,prompt = procSubscribe(mqttc)
            if result then
                while true do
                    procSend(mqttc)
                end
            end

            -- if connectCb then connectCb(false,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](false) end
        end
    end)
end


--正常连接 预注册一型一密获取DeviceSecret后就是正常的一机一密连接   
function aliyun.clientGetDirectDataTask(DeviceName,ProductKey,mqtt_host,mqtt_port,mqtt_isssl,Registration,DeviceSecret,deviceToken,cid)
    sys.taskInit(function()
        if not Registration then
            local client_id,user_name,password = iotauth.aliyun(ProductKey,DeviceName,DeviceSecret)
            mqttc = mqtt.create(nil,mqtt_host, mqtt_port,mqtt_isssl)  --mqtt客户端创建
            mqttc:auth(client_id,user_name,password) --mqtt三元组配置
        else
            local clientId = cid.."|securemode=-2,authType=connwl|"

            local client_id,user_name,password = iotauth.aliyun(ProductKey,DeviceName,deviceToken)
            mqttc = mqtt.create(nil,mqtt_host, mqtt_port,mqtt_isssl)  --mqtt客户端创建
    
            mqttc:auth(clientId,user_name,deviceToken) --mqtt三元组配置
        end

        mqttc:keepalive(30) -- 默认值240s
        mqttc:autoreconn(true, 20000) -- 自动重连机制
        mqttc:connect()
        mqttc:on(mqtt_cbevent)  --mqtt回调注册

        local conres = sys.waitUntil("mqtt_conack",30000)
        if mqttc:ready() and conres then
            -- if connectCb then connectCb(true,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](true) end

            local result,prompt = procSubscribe(mqttc)
            if result then
                while true do
                    procSend(mqttc)
                end
            end

            -- if connectCb then connectCb(false,ProductKey,DeviceName) end
            -- if EvtCb["connect"] then EvtCb["connect"](false) end
        end
    end)
end


--[[
订阅主题
@api aliyun.subscribe(topic,qos)
@param topic，string类型，主题内容为UTF8编码
@param qos，number，qos为number类型(0/1，默认0)；
@return nil
@usage
aliyun.subscribe("/b0FMK1Ga5cp/862991234567890/get", 0)
]]
function aliyun.subscribe(topic,qos)
    insert("SUBSCRIBE",topic,qos)
end


--[[
发布一条消息
@api aliyun.publish(topic,qos,payload,cbFnc,cbPara)
@string topic，UTF8编码的主题
@number[opt=0] qos，质量等级，0/1，默认0
@string payload，负载内容，UTF8编码
@function[opt=nil] cbFnc，消息发布结果的回调函数,回调函数的调用形式为：cbFnc(result,cbPara)。result为true表示发布成功，false或者nil表示订阅失败；cbPara为本接口中的第5个参数
@param[opt=nil] cbPara，消息发布结果回调函数的回调参数
@return nil
@usage
aliyun.publish("/b0FMK1Ga5cp/862991234567890/update",0,"test")
aliyun.publish("/b0FMK1Ga5cp/862991234567890/update",1,"test",cbFnc,"cbFncPara")
]]
function aliyun.publish(topic,qos,payload,cbFnc,cbPara)
    insert("PUBLISH",topic,qos,payload,cbFnc,cbPara)
    sys.publish("ALIYUN_PUB")
end


--[[
注册事件的处理函数
@api aliyun.on(evt,cbFnc)
@string evt 事件
"connect"表示接入服务器连接结果事件
"receive"表示接收到接入服务器的消息事件
"publish"表示发送消息的结果事件
@function cbFnc 事件的处理函数
当evt为"connect"时，cbFnc的调用形式为：cbFnc(result)，result为true表示连接成功，false或者nil表示连接失败
当evt为"receive"时，cbFnc的调用形式为：cbFnc(topic,payload)，topic为UTF8编码的主题(string类型)，payload为原始编码的负载(string类型)
当evt为"publish"时，cbFnc的调用形式为：cbFnc(result)，result为true表示发送成功，false或者nil表示发送失败
@return nil
@usage
aliyun.on("connect",cbFnc)
]]
function aliyun.on(evt,cbFnc)
	EvtCb[evt] = cbFnc
end


--[[
@api aliyun.getDeviceSecret()
@function 预注册一型一密阿里云返回的DeviceSecret
可以在应用层使用kv区来保存该参数并使用判断来避免重启后无法连接
]]
function aliyun.getDeviceSecret()
    return SetDeviceSecretFnc
end

--[[
@api aliyun.getDeviceToken()
@function 免预注册一型一密阿里云返回的DeviceToken
可以在应用层使用kv区来保存该参数并使用判断来避免重启后无法连接
]]
function aliyun.getDeviceToken()
    return SetDeviceTokenFnc
end

--[[
@api aliyun.getClientid()
@function 免预注册一型一密阿里云返回的Clientid
可以在应用层使用kv区来保存该参数并使用判断来避免重启后无法连接
]]
function aliyun.getClientid()
    return SetClientidFnc
end



return aliyun
-- 用户代码已结束---------------------------------------------
