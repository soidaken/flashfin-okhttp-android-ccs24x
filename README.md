### 这是什么
> 这是在CocosCreater2.4.x版本中Android平台使用websocket的wss连接时不需要指定本地cafile的补丁.
> 实现方式是: c++层不再使用libwebsocket库而是通过jni调用java的okhttp3库

### 兼容版本
` 2.4.0 - 2.4.15都支持 `

### 修改步骤
1. 复制新文件`cocos2d-x/cocos/network/WebSocket-okhttp_android.cpp` 到对应引擎目录
2. 对比修改 `cocos2d-x/cocos/Android.mk`的` network/WebSocket-libwebsockets.cpp \` 替换为`network/WebSocket-okhttp_android.cpp \`
3. 对比修改 `cocos2d-x/cocos/platform/android/jni/JniHelper.cpp`
4. 对比修改 `cocos2d-x/cocos/platform/android/jni/JniHelper.h`
5. 复制新文件夹`cocos2d-x\cocos\platform\android\java\src\src\org\cocos2dx\lib\websocket`到对应引擎目录
6. 复制新文件`cocos2d-x\cocos\platform\android\java\src\src\org\cocos2dx\lib\GlobalObject.java`到对应引擎目录
7. 结束. 重新编译android工程即可.
8. 在脚本代码中正常 `new websocket(xxx)`即可正常连接上wss://xxxx的服务器地址.


### 帮到你了吗?
如果对你有帮助,请不吝赞助我一杯卡布奇诺☕️,谢谢!  
![myRewardCode](https://github.com/soidaken/flashfin-tipQRcode/blob/main/reward-qrcode-small.jpg)
