package org.cocos2dx.lib.websocket;

import android.os.Build;
import android.util.Log;

import org.cocos2dx.lib.Cocos2dxHelper;
import org.cocos2dx.lib.GlobalObject;

import org.cocos2dx.okhttp3.CipherSuite;
import org.cocos2dx.okhttp3.Dispatcher;
import org.cocos2dx.okhttp3.OkHttpClient;
import org.cocos2dx.okhttp3.Protocol;
import org.cocos2dx.okhttp3.Request;
import org.cocos2dx.okhttp3.Response;
import org.cocos2dx.okhttp3.WebSocketListener;
import org.cocos2dx.okio.ByteString;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.security.GeneralSecurityException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;


@SuppressWarnings("unused")
public class CocosWebSocket extends WebSocketListener {
    private final static String _TAG = "cocos-websocket";

    static {
        NativeInit();
    }

    private static class _WebSocketContext {
        long identifier;
        long handlerPtr;
    }
    private static Dispatcher dispatcher = null;

    private final long              _timeout;
    private final boolean           _perMessageDeflate;
    private final boolean           _tcpNoDelay;
    private final                   String[] _header;
    private final _WebSocketContext _wsContext =
        new _WebSocketContext();

    private org.cocos2dx.okhttp3.WebSocket _webSocket;
    private OkHttpClient                   _client;

    CocosWebSocket(long ptr, long handler, String[] header, boolean tcpNoDelay,
                   boolean perMessageDeflate, long timeout) {
        _wsContext.identifier = ptr;
        _wsContext.handlerPtr = handler;
        _header               = header;
        _tcpNoDelay           = tcpNoDelay;
        _perMessageDeflate    = perMessageDeflate;
        _timeout              = timeout;
    }

    private void _removeHandler() {
        synchronized (_wsContext) {
            _wsContext.identifier = 0;
            _wsContext.handlerPtr = 0;
        }
    }

    private void _send(final byte[] msg) {
        //        Log.d(_TAG, "try sending binary msg");
        if (null == _webSocket) {
            Log.e(_TAG, "WebSocket hasn't connected yet");
            return;
        }

        ByteString byteString = ByteString.of(msg);
        _webSocket.send(byteString);
    }

    private void _send(final String msg) {
        //        Log.d(_TAG, "try sending string msg: " + msg);
        if (null == _webSocket) {
            Log.e(_TAG, "WebSocket hasn't connected yet");
            return;
        }

        _webSocket.send(msg);
    }

    /**
     * Returns the VM's default SSL socket factory, using {@code trustManager} for
     * trusted root certificates.
     */
    private SSLSocketFactory
    defaultSslSocketFactory(X509TrustManager trustManager)
        throws NoSuchAlgorithmException, KeyManagementException {
        SSLContext   sslContext = SSLContext.getInstance("TLS");
        SecureRandom random;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            random = SecureRandom.getInstanceStrong();
        } else {
            random = SecureRandom.getInstance("SHA1PRNG");
        }
        sslContext.init(null, new TrustManager[] {trustManager}, random);
        return sslContext.getSocketFactory();
    }

    private String[] javaNames(List<CipherSuite> cipherSuites) {
        if (cipherSuites == null) {
            return new String[0];
        } else {
            String[] result = new String[cipherSuites.size()];
            for (int i = 0; i < result.length; i++) {
                result[i] = cipherSuites.get(i).javaName();
            }
            return result;
        }
    }

    private void _connect(final String url, final String protocols,
                          final String caFilePath) {
        Log.d(_TAG, "connect ws url: '" + url + "' ,protocols: '" + protocols + "' ,ca_: '" + caFilePath + "'");
        Request.Builder requestBuilder = new Request.Builder().url(url);
        URI uriObj = null;
        try {
            requestBuilder = requestBuilder.url(url.trim());
            uriObj = URI.create(url);
        } catch (NullPointerException | IllegalArgumentException  e) {
            Cocos2dxHelper.runOnGLThread(() -> {
                synchronized (_wsContext) {
                    nativeOnError("invalid url", _wsContext.identifier,
                        _wsContext.handlerPtr);
                }
            });
            return;
        }
        if (!protocols.isEmpty()) {
            requestBuilder.header("Sec-WebSocket-Protocol", protocols);
        }
        if (_header != null) {
            for (int index = 0; index < _header.length; index += 2) {
                requestBuilder.header(_header[index], _header[index + 1]);
            }
        }

        String originProtocol =uriObj.getScheme().toLowerCase();
        String uriScheme = (originProtocol.equals("wss") || originProtocol.equals("https"))? "https" : "http";
        requestBuilder.addHeader("Origin", uriScheme + "://" + uriObj.getHost() + (uriObj.getPort() < 0 ? "" : ":" + uriObj.getPort()));

        Request request = requestBuilder.build();

        if(dispatcher == null) {
            dispatcher = new Dispatcher();
        }



        OkHttpClient.Builder builder =
            new OkHttpClient.Builder()
                .dispatcher(dispatcher)

                .protocols(Collections.singletonList(Protocol.HTTP_1_1))
                .readTimeout(_timeout, TimeUnit.MILLISECONDS)
                .writeTimeout(_timeout, TimeUnit.MILLISECONDS)
                .connectTimeout(_timeout, TimeUnit.MILLISECONDS);

        if (_perMessageDeflate) {
            // 开启压缩扩展, 开启 Gzip 压缩
            builder.addInterceptor(new CocosGzipRequestInterceptor());
        }
        KeyStore keyStore = null;
        if (url.toLowerCase().startsWith("wss://") && !caFilePath.isEmpty()) {
            try {
                InputStream caInput = null;

                if (caFilePath.startsWith("assets/")) {
                    caInput = GlobalObject.getContext().getResources().getAssets().open(caFilePath);
                } else {
                    caInput = new FileInputStream(caFilePath);
                }
                if (caFilePath.toLowerCase().endsWith(".pem")) {
                    keyStore = CocosWebSocketUtils.GetPEMKeyStore(caInput);
                } else {
                    keyStore = CocosWebSocketUtils.GetCERKeyStore(caInput);
                }
            } catch (Exception e) {
                e.printStackTrace();
                String msg = e.getMessage();
                final String errMsg = msg != null ? msg : "unknown error";
                Cocos2dxHelper.runOnGLThread(() -> {
                    synchronized (_wsContext) {
                        nativeOnError(errMsg, _wsContext.identifier, _wsContext.handlerPtr);
                    }
                });
                return;
            }

            builder.hostnameVerifier(new HostnameVerifier() {
                @Override
                public boolean verify(String hostname, SSLSession session) {
                    Log.d(_TAG, "ca hostname: " + hostname);
                    HostnameVerifier hv = HttpsURLConnection.getDefaultHostnameVerifier();
                    return hv.verify(hostname, session);
                }
            });
        }
        if (url.toLowerCase().startsWith("wss://") || _tcpNoDelay) {
            try {
                X509TrustManager trustManager =
                    CocosWebSocketUtils.GetTrustManager(keyStore);
                SSLContext   sslContext = SSLContext.getInstance("TLS");
                SecureRandom random;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    random = SecureRandom.getInstanceStrong();
                    Log.d(_TAG, "random use strong" );
                } else {
                    random = SecureRandom.getInstance("SHA1PRNG");
                    Log.d(_TAG, "random use SHA1PRNG" );
                }
                sslContext.init(null, new TrustManager[] {trustManager}, random);
                SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();
                SSLSocketFactory customSslSocketFactory =
                    new CocosDelegatingSSLSocketFactory(sslSocketFactory) {
                        @Override
                        protected SSLSocket configureSocket(SSLSocket socket)
                            throws          IOException {
                            socket.setTcpNoDelay(_tcpNoDelay);
                            // TLSv1.2 is disabled default below API20----
                            // https://developer.android.com/reference/javax/net/ssl/SSLSocket
                            if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
                                socket.setEnabledProtocols(new String[] {"TLSv1.2"});
                            }
                            return socket;
                        }
                    };
                builder.sslSocketFactory(customSslSocketFactory, trustManager);
            } catch (GeneralSecurityException e) {
                e.printStackTrace();
                String msg = e.getMessage();
                final String errMsg = msg != null ? msg : "unknown error";
                Cocos2dxHelper.runOnGLThread(() -> {
                    synchronized (_wsContext) {
                        nativeOnError(errMsg, _wsContext.identifier, _wsContext.handlerPtr);
                    }
                });
                return;
            }
        }
        _client    = builder.build();

        _debugInfo(request,_client);

        _webSocket = _client.newWebSocket(request, this);
    }

    private  void _debugInfo( Request r, OkHttpClient c){
        System.out.println("=== OkHttpClient.Builder ===");
        System.out.println("Connect Timeout: " + c.connectTimeoutMillis() + "ms");
        System.out.println("Read Timeout: " + c.readTimeoutMillis() + "ms");
        System.out.println("Write Timeout: " + c.writeTimeoutMillis() + "ms");
        System.out.println("Protocols: " + c.protocols());
        System.out.println("Proxy: " + c.proxy());
        System.out.println("SSL Socket Factory: " + c.sslSocketFactory());
        System.out.println("Hostname Verifier: " + c.hostnameVerifier());

    // 打印 Request 信息
        System.out.println("\n=== Request ===");
        System.out.println("URL: " + r.url());
        System.out.println("Method: " + r.method());
        System.out.println("Headers: " + r.headers());
        System.out.println("Body: " + r.body());
//        System.out.println("Tags: " + r.tags());
        System.out.println("Full Request: " + r);
    }


    private void _close(final int code, final String reason) {
        _webSocket.close(code, reason);
        // _client.dispatcher().executorService().shutdown();
    }

    private long _getBufferedAmountID() {
        return _webSocket.queueSize();
    }

    private void output(final String content) {
        Log.w(_TAG, content);
    }

    @Override
    public void onOpen(org.cocos2dx.okhttp3.WebSocket _webSocket, Response response) {
        output("WebSocket onOpen _client: " + _client);
        output("WebSocket onOpen response.protocol().toString(): " + response.protocol().toString());
        output("WebSocket onOpen response.headers().toString(): " + response.headers().toString());
        Log.d("MyWebSocketDebug", "Scheduling nativeOnOpen on game thread..."); // 添加日志点 1
        Cocos2dxHelper.runOnGLThread(() -> {
            Log.d("MyWebSocketDebug", "Now on game thread, entering synchronized block..."); // 添加日志点 2
            synchronized (_wsContext) {
                Log.d("MyWebSocketDebug", "Inside synchronized block, about to call nativeOnOpen..."); // 添加日志点 3
                try {
                    nativeOnOpen(response.protocol().toString(),
                            response.headers().toString(), _wsContext.identifier,
                            _wsContext.handlerPtr);
                    Log.d("MyWebSocketDebug", "Call to nativeOnOpen finished (or threw exception)."); // 添加日志点 4
                } catch (Throwable t) {
                    Log.e("MyWebSocketDebug", "Exception occurred during nativeOnOpen call or inside it:", t); // 捕获潜在错误
                }
            }
            Log.d("MyWebSocketDebug", "Exited synchronized block."); // 添加日志点 5
        });
        Log.d("MyWebSocketDebug", "Scheduled nativeOnOpen, onOpen method exiting."); // 添加日志点 6
    }

    @Override
    public void onMessage(org.cocos2dx.okhttp3.WebSocket _webSocket, String text) {
        //        output("Receiving string msg: " + text);
        Cocos2dxHelper.runOnGLThread(() -> {
            synchronized (_wsContext) {
                nativeOnStringMessage(text, _wsContext.identifier, _wsContext.handlerPtr);
            }
        });
    }

    @Override
    public void onMessage(org.cocos2dx.okhttp3.WebSocket _webSocket, ByteString bytes) {
        //        output("Receiving binary msg");
        Cocos2dxHelper.runOnGLThread(() -> {
            synchronized (_wsContext) {
                nativeOnBinaryMessage(bytes.toByteArray(), _wsContext.identifier,
                    _wsContext.handlerPtr);
            }
        });
    }

    @Override
    public void onClosing(org.cocos2dx.okhttp3.WebSocket _webSocket, int code,
                          String reason) {
        output("Closing : " + code + " / " + reason);
        if (_webSocket != null) {
            _webSocket.close(code, reason);
        }
    }

    @Override
    public void onFailure(org.cocos2dx.okhttp3.WebSocket _webSocket, Throwable t,
                          Response response) {
        final String msg;
        if (t != null) {
            msg = t.getMessage() == null ?  t.getClass().getSimpleName() : t.getMessage();
        } else {
            msg = "";
        }
        output("onFailure Error : " + msg);
        Cocos2dxHelper.runOnGLThread(() -> {
            synchronized (_wsContext) {
                nativeOnError(msg, _wsContext.identifier, _wsContext.handlerPtr);
            }
        });
    }

    @Override
    public void onClosed(org.cocos2dx.okhttp3.WebSocket _webSocket, int code,
                         String reason) {
        output("onClosed : " + code + " / " + reason);
        Cocos2dxHelper.runOnGLThread(() -> {
            synchronized (_wsContext) {
                nativeOnClosed(code, reason, _wsContext.identifier,
                    _wsContext.handlerPtr);
            }
        });
    }

    private static native void NativeInit();

    private native void nativeOnStringMessage(final String msg, long identifier,
                                              long handler);

    private native void nativeOnBinaryMessage(final byte[] msg, long identifier,
                                              long handler);

    private native void nativeOnOpen(final String protocol,
                                     final String headerString, long identifier,
                                     long handler);

    private native void nativeOnClosed(final int code, final String reason,
                                       long identifier, long handler);

    private native void nativeOnError(final String msg, long identifier,
                                      long handler);
}
