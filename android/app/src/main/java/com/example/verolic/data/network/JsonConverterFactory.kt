package com.example.verolic.data.network

import kotlinx.serialization.InternalSerializationApi
import kotlinx.serialization.json.Json
import kotlinx.serialization.serializer
import okhttp3.MediaType
import okhttp3.RequestBody
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.ResponseBody
import retrofit2.Converter
import retrofit2.Retrofit
import java.lang.reflect.Type

/**
 * Minimal Retrofit <-> kotlinx.serialization bridge, written by hand instead of pulling in a
 * separate converter artifact - this app only needs request/response bodies to round-trip
 * through [Json], nothing more.
 */
@OptIn(InternalSerializationApi::class)
class JsonConverterFactory(private val json: Json, private val mediaType: MediaType) : Converter.Factory() {

    override fun responseBodyConverter(
        type: Type,
        annotations: Array<out Annotation>,
        retrofit: Retrofit
    ): Converter<ResponseBody, *> {
        val serializer = json.serializersModule.serializer(type)
        return Converter<ResponseBody, Any?> { body -> body.use { json.decodeFromString(serializer, it.string()) } }
    }

    override fun requestBodyConverter(
        type: Type,
        parameterAnnotations: Array<out Annotation>,
        methodAnnotations: Array<out Annotation>,
        retrofit: Retrofit
    ): Converter<*, RequestBody> {
        val serializer = json.serializersModule.serializer(type)
        return Converter<Any, RequestBody> { value -> json.encodeToString(serializer, value).toRequestBody(mediaType) }
    }
}
