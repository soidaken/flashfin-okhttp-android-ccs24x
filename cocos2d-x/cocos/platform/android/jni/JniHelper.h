/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#ifndef __ANDROID_JNI_HELPER_H__
#define __ANDROID_JNI_HELPER_H__

#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "base/ccMacros.h"
#include "math/Vec3.h"

//The macro must be used this way to find the native method. The principle is not well understood.
#define JNI_METHOD2(CLASS2,FUNC2) Java_##CLASS2##_##FUNC2
#define JNI_METHOD1(CLASS1,FUNC1) JNI_METHOD2(CLASS1,FUNC1)

NS_CC_BEGIN

typedef struct JniMethodInfo_
{
    JNIEnv *    env;
    jclass      classID;
    jmethodID   methodID;
} JniMethodInfo;


class CC_DLL JniHelper
{
public:

    typedef std::unordered_map<JNIEnv *, std::vector<jobject >> LocalRefMapType;

    static void setJavaVM(JavaVM *javaVM);
    static JavaVM* getJavaVM();
    static JNIEnv* getEnv();
    static jobject getActivity();

    static bool setClassLoaderFrom(jobject activityInstance);
    static bool getStaticMethodInfo(JniMethodInfo &methodinfo,
                                    const char *className,
                                    const char *methodName,
                                    const char *paramCode);
    static bool getMethodInfo(JniMethodInfo &methodinfo,
                              const char *className,
                              const char *methodName,
                              const char *paramCode);

    static std::string jstring2string(jstring str);

    static jmethodID loadclassMethod_methodID;
    static jobject classloader;
    static std::function<void()> classloaderCallback;

    template <typename... Ts>
    static jobject newObject(const std::string& className, Ts... xs)
    {
        jobject ret = nullptr;
        static const char* methodName = "<init>";
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")V";
        if (cocos2d::JniHelper::getMethodInfo(t, className.c_str(), methodName, signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->NewObject(t.classID, t.methodID, cocos2d::JniHelper::convert(localRefs,t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static void callObjectVoidMethod(jobject object,
                                     const std::string& className, 
                                     const std::string& methodName, 
                                     Ts... xs) {
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")V";
        if (cocos2d::JniHelper::getMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            t.env->CallVoidMethod(object, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
    }

    template <typename... Ts>
    static float callObjectFloatMethod(jobject object,
                                     const std::string& className, 
                                     const std::string& methodName, 
                                     Ts... xs) {
        float ret = 0.0f;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")F";
        if (cocos2d::JniHelper::getMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallFloatMethod(object, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static long callObjectLongMethod(jobject object,
                                       const std::string& className,
                                       const std::string& methodName,
                                       Ts... xs) {
        long ret = 0;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")J";
        if (cocos2d::JniHelper::getMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallLongMethod(object, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static jbyteArray callObjectByteArrayMethod(jobject object,
                                     const std::string& className, 
                                     const std::string& methodName, 
                                     Ts... xs) {
        jbyteArray ret = nullptr;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")[B";
        if (cocos2d::JniHelper::getMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = (jbyteArray)t.env->CallObjectMethod(object, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static void callStaticVoidMethod(const std::string& className, 
                                     const std::string& methodName, 
                                     Ts... xs) {
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")V";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            t.env->CallStaticVoidMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
    }

    template <typename... Ts>
    static bool callStaticBooleanMethod(const std::string& className, 
                                        const std::string& methodName, 
                                        Ts... xs) {
        jboolean jret = JNI_FALSE;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")Z";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            jret = t.env->CallStaticBooleanMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return (jret == JNI_TRUE);
    }

    template <typename... Ts>
    static int callStaticIntMethod(const std::string& className, 
                                   const std::string& methodName, 
                                   Ts... xs) {
        jint ret = 0;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")I";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallStaticIntMethod(t.classID, t.methodID, convert(t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static float callStaticFloatMethod(const std::string& className, 
                                       const std::string& methodName, 
                                       Ts... xs) {
        jfloat ret = 0.0;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")F";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallStaticFloatMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static long callStaticLongMethod(const std::string& className,
                                       const std::string& methodName,
                                       Ts... xs) {
        jlong ret = 0;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")J";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallStaticLongMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }


    template <typename... Ts>
    static float* callStaticFloatArrayMethod(const std::string& className, 
                                       const std::string& methodName, 
                                       Ts... xs) {
        static float ret[32];
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")[F";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            jfloatArray array = (jfloatArray) t.env->CallStaticObjectMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            jsize len = t.env->GetArrayLength(array);
            if (len <= 32) {
                jfloat* elems = t.env->GetFloatArrayElements(array, 0);
                if (elems) {
                    memcpy(ret, elems, sizeof(float) * len);
                    t.env->ReleaseFloatArrayElements(array, elems, 0);
                };
            }
            t.env->DeleteLocalRef(array);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
            return &ret[0];
        } else {
            reportError(className, methodName, signature);
        }
        return nullptr;
    }

    template <typename... Ts>
    static Vec3 callStaticVec3Method(const std::string& className, 
                                       const std::string& methodName, 
                                       Ts... xs) {
        Vec3 ret;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")[F";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            jfloatArray array = (jfloatArray) t.env->CallStaticObjectMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            jsize len = t.env->GetArrayLength(array);
            if (len == 3) {
                jfloat* elems = t.env->GetFloatArrayElements(array, 0);
                ret.x = elems[0];
                ret.y = elems[1];
                ret.z = elems[2];
                t.env->ReleaseFloatArrayElements(array, elems, 0);
            }
            t.env->DeleteLocalRef(array);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static double callStaticDoubleMethod(const std::string& className, 
                                         const std::string& methodName, 
                                         Ts... xs) {
        jdouble ret = 0.0;
        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")D";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            ret = t.env->CallStaticDoubleMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            t.env->DeleteLocalRef(t.classID);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }

    template <typename... Ts>
    static std::string callStaticStringMethod(const std::string& className, 
                                              const std::string& methodName, 
                                              Ts... xs) {
        std::string ret;

        cocos2d::JniMethodInfo t;
        std::string signature = "(" + std::string(getJNISignature(xs...)) + ")Ljava/lang/String;";
        if (cocos2d::JniHelper::getStaticMethodInfo(t, className.c_str(), methodName.c_str(), signature.c_str())) {
            LocalRefMapType localRefs;
            jstring jret = (jstring)t.env->CallStaticObjectMethod(t.classID, t.methodID, convert(localRefs, t, xs)...);
            ret = cocos2d::JniHelper::jstring2string(jret);
            t.env->DeleteLocalRef(t.classID);
            t.env->DeleteLocalRef(jret);
            deleteLocalRefs(t.env, localRefs);
        } else {
            reportError(className, methodName, signature);
        }
        return ret;
    }
private:
    static JNIEnv* cacheEnv(JavaVM* jvm);

    static bool getMethodInfo_DefaultClassLoader(JniMethodInfo &methodinfo,
                                                 const char *className,
                                                 const char *methodName,
                                                 const char *paramCode);

    static JavaVM* _psJavaVM;
    
    static jobject _activity;

    static jstring convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo& t, const char* x);

    static jstring convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo& t, const std::string& x);

	static jobject convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo &t, const std::vector<std::string> &x);

    template <typename T>
    static T convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo&, T x) {
        return x;
    }

    template <typename T>
    static jobject convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo &t, std::pair<T *, size_t> data) {
        jobject ret = nullptr;

#define JNI_SET_TYPED_ARRAY(lowercase, camelCase)                                                                   \
    j##lowercase##Array array = t.env->New##camelCase##Array(data.second);                                         \
    t.env->Set##camelCase##ArrayRegion(array, 0, data.second, reinterpret_cast<const j##lowercase *>(data.first)); \
    if (array) {                                                                                                    \
        (localRefs)[t.env].push_back(array);                                                                      \
    }                                                                                                               \
    ret = static_cast<jobject>(array);

        using U = typename std::remove_cv<typename std::remove_pointer<T>::type>::type;

        if (sizeof(U) == 1) {
            JNI_SET_TYPED_ARRAY(byte, Byte)
        } else if (sizeof(U) == 4 && std::is_integral<U>::value) {
            JNI_SET_TYPED_ARRAY(int, Int)
        } else if (sizeof(U) == 8 && std::is_integral<U>::value) {
            JNI_SET_TYPED_ARRAY(long, Long);
        } else if (sizeof(U) == 4 && std::is_floating_point<U>::value) {
            JNI_SET_TYPED_ARRAY(float, Float);
        } else if (sizeof(U) == 8 && std::is_floating_point<U>::value) {
            JNI_SET_TYPED_ARRAY(double, Double);
        }

#undef JNI_SET_TYPED_ARRAY
        return ret;
    }

    template <typename T, typename A>
    static typename std::enable_if<std::is_arithmetic<T>::value, jobject>::type convert(LocalRefMapType &localRefs, cocos2d::JniMethodInfo &t, const std::vector<T, A> &data) {
        jobject ret = nullptr;

#define JNI_SET_TYPED_ARRAY(lowercase, camelCase)                                                                    \
    j##lowercase##Array array = t.env->New##camelCase##Array(data.size());                                          \
    t.env->Set##camelCase##ArrayRegion(array, 0, data.size(), reinterpret_cast<const j##lowercase *>(data.data())); \
    if (array) {                                                                                                     \
        (localRefs)[t.env].push_back(array);                                                                       \
    }                                                                                                                \
    ret = static_cast<jobject>(array);

        if (sizeof(T) == 1) {
            JNI_SET_TYPED_ARRAY(byte, Byte)
        } else if (sizeof(T) == 4 && std::is_integral<T>::value) {
            JNI_SET_TYPED_ARRAY(int, Int)
        } else if (sizeof(T) == 8 && std::is_integral<T>::value) {
            JNI_SET_TYPED_ARRAY(long, Long);
        } else if (sizeof(T) == 4 && std::is_floating_point<T>::value) {
            JNI_SET_TYPED_ARRAY(float, Float);
        } else if (sizeof(T) == 8 && std::is_floating_point<T>::value) {
            JNI_SET_TYPED_ARRAY(double, Double);
        }

#undef JNI_SET_TYPED_ARRAY
        return ret;
    }

    static void deleteLocalRefs(JNIEnv *env, LocalRefMapType &localRefs);

    static std::string getJNISignature() {
        return "";
    }

    static std::string getJNISignature(bool) {
        return "Z";
    }

    // jchar is unsigned 16 bits, we do char => jchar conversion on purpose
    static std::string getJNISignature(char) {
        return "C";
    }


    static std::string getJNISignature(unsigned char /*unused*/) {
        return "B"; // same as jbyte
    }

    static std::string getJNISignature(jshort) {
        return "S";
    }

    static std::string getJNISignature(jint) {
        return "I";
    }

    static std::string getJNISignature(jlong) {
        return "J";
    }

    static std::string getJNISignature(jfloat) {
        return "F";
    }

    static std::string getJNISignature(jdouble) {
        return "D";
    }

    static std::string getJNISignature(jbyteArray) {
        return "[B";
    }

    static std::string getJNISignature(jintArray /*unused*/) {
        return "[I";
    }

    static std::string getJNISignature(const char * /*unused*/) {
        return "Ljava/lang/String;";
    }

    static std::string getJNISignature(const std::string&) {
        return "Ljava/lang/String;";
    }

    template <typename T>
    static std::string getJNISignature(T x) {
        // This template should never be instantiated
        static_assert(sizeof(x) == 0, "Unsupported argument type");
        return "";
    }

    template <typename T>
    static std::string getJNISignature(std::pair<T *, size_t> /*x*/) {
        typename std::remove_pointer<typename std::remove_cv<T>::type>::type m;
        return std::string("[") + getJNISignature(m);
    }

    template <typename T, typename A>
    static std::string getJNISignature(const std::vector<T, A> & /*x*/) {
        T m;
        return std::string("[") + getJNISignature(m);
    }

    template <typename T, typename... Ts>
    static std::string getJNISignature(T x, Ts... xs) {
        return getJNISignature(x) + getJNISignature(xs...);
    }

    static void reportError(const std::string& className, const std::string& methodName, const std::string& signature);
};

NS_CC_END

#endif // __ANDROID_JNI_HELPER_H__
