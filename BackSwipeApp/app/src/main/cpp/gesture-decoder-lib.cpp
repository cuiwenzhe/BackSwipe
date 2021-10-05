#include <jni.h>
#include <string>
#include <vector>
#include "GestureDecoder.h"
#include "internal/base/logging.h"
#include "internal/Louds/louds-lm.h"
#include "internal/Louds/louds-lm-adapter.h"
#include "internal/touch-sequence.h"
#include "internal/keyboardSetting/keyboard.h"
#include "internal/keyboardSetting/keyboard-layout-tools.h"
#include "internal/decoder-result.h"

using std::string;
using std::vector;
using std::unique_ptr;
using std::move;
using keyboard::lm::louds::LoudsLm;
using keyboard::decoder::lm::LoudsLmAdapter;
using keyboard::decoder::TouchSequence;
using keyboard::decoder::KeyboardLayout;
using keyboard::decoder::GestureDecoder;
using keyboard::decoder::LexiconInterface;
using keyboard::decoder::DecoderResult;
static GestureDecoder* decoder = nullptr;

int kMaxResults = 5;

// Converts a Java jbyteArray (encoding a UTF8 string) to a native UTF8 string.
string JbyteArrayToString(JNIEnv* env, jbyteArray input) {
    jint len = env->GetArrayLength(input);
    string output;
    output.resize(len);
    env->GetByteArrayRegion(input, 0, len,
                            reinterpret_cast<jbyte*>(&(output)[0]));
    return output;
}

// Converts a native UTF8 string to a Java jbyteArray.
jbyteArray StringToJbyteArray(JNIEnv* env, string input) {
    jbyteArray output = env->NewByteArray(input.length());
    if (output != nullptr) {
        env->SetByteArrayRegion(output, 0, input.length(),
                                reinterpret_cast<const jbyte*>(&(input)[0]));
    }
    return output;
}

jobject getJDecoderResults(JNIEnv* env, const vector<DecoderResult>& results) {
    // Retrieve the class and method to add to a java ArrayList.
    jclass FloatClass = env->FindClass("java/lang/Float");
    jclass java_util_ArrayList = env->FindClass("java/util/ArrayList");
    jmethodID java_util_ArrayList_ = env->GetMethodID(java_util_ArrayList, "<init>", "()V");
    jmethodID java_util_ArrayList_size = env->GetMethodID(java_util_ArrayList, "size", "()I");
    jmethodID java_util_ArrayList_get = env->GetMethodID(java_util_ArrayList, "get",
                                                         "(I)Ljava/lang/Object;");
    jmethodID java_util_ArrayList_add = env->GetMethodID(java_util_ArrayList, "add",
                                                         "(Ljava/lang/Object;)Z");

    jobject stringResultArray =
            env->NewObject(java_util_ArrayList, java_util_ArrayList_);
    jobject floatResultArray =
            env->NewObject(java_util_ArrayList, java_util_ArrayList_);
    jmethodID float_to_Float_constructor_id =
            env->GetMethodID(FloatClass, "<init>", "(F)V");
    for (size_t i = 0; i < results.size() && i < kMaxResults; ++i) {
        //jbyteArray word_jbytes = StringToJbyteArray(env, results[i].word());
        jstring word_jbytes = env->NewStringUTF(results[i].word().c_str());
        if (word_jbytes == nullptr) {
            LOG(ERROR) << "word_jbytes is null";
            break;
        }

        env->CallBooleanMethod(stringResultArray, java_util_ArrayList_add,
                               word_jbytes);
        // Delete references in manually since we may iterate over a large number of
        // objects. Otherwise they'll be released when the method returns.
        env->DeleteLocalRef(word_jbytes);

        jobject output_score =
                env->NewObject(FloatClass, float_to_Float_constructor_id,
                               static_cast<float>(results[i].score()));
        if (output_score == NULL) {
            break;
        }
        env->CallBooleanMethod(floatResultArray, java_util_ArrayList_add,
                               output_score);
        env->DeleteLocalRef(output_score);
    }
    jclass decoder_result = env->FindClass("com/example/simplegestureinput/common/DecoderResults");
    jmethodID decoder_result_void_mid = env->GetMethodID(decoder_result, "<init>", "()V");
    jmethodID decoder_result_set_result_mid = env->GetMethodID(decoder_result, "setResults",
                                                               "(Ljava/util/ArrayList;Ljava/util/ArrayList;)V");
    jobject j_decoder_results = env->NewObject(decoder_result, decoder_result_void_mid);

    jvalue args[2];
    args[0].l = stringResultArray;
    args[1].l = floatResultArray;

    env->CallVoidMethod(j_decoder_results, decoder_result_set_result_mid, stringResultArray, floatResultArray);

    return j_decoder_results;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_simplegestureinput_DecoderTools_createDecoderNative(JNIEnv *env, jclass clazz,
                                                                     jbyteArray lm_name_bytes,
                                                                     jbyteArray file_path_bytes,
                                                                     jlong lm_offset,
                                                                     jlong lm_size) {

    if(decoder == nullptr){
        decoder = new GestureDecoder(true);
    }
    string lm_name = JbyteArrayToString(env, lm_name_bytes);
    string filename = JbyteArrayToString(env, file_path_bytes);

    unique_ptr<LoudsLm> louds_lm = LoudsLm::CreateFromMappedFileOrNull(filename, lm_offset, lm_size);
    if (louds_lm == nullptr) {
        return reinterpret_cast<jlong>(nullptr);
    }
    unique_ptr<LoudsLmAdapter> lm_adapter(
            new LoudsLmAdapter(move(louds_lm)));
    LexiconInterface* lexicon = lm_adapter->lexicon();
    decoder->AddLexiconAndLm(lm_name, lexicon, move(lm_adapter));
    decoder->RecreateDecoderForActiveLms();
    return reinterpret_cast<jlong>(decoder);;
}
// Sets the keyboard layout according to the given parameters.
extern "C"
JNIEXPORT void JNICALL
Java_com_example_simplegestureinput_DecoderTools_setKeyboardLayoutNative(
        JNIEnv* env, jclass clazz, jlong decoder_ptr, jint key_count,
        jint most_common_key_width, jint most_common_key_height,
        jint keyboard_width, jint keyboard_height, jintArray codes_array,
        jintArray x_coords_array, jintArray y_coords_array, jintArray widths_array,
        jintArray heights_array) {
    if (!decoder) {
        LOGE("Invalid decoder pointer");
        return;
    }

    vector<int> codes;
    vector<int> x_coords;
    vector<int> y_coords;
    vector<int> widths;
    vector<int> heights;

    codes.resize(key_count);
    x_coords.resize(key_count);
    y_coords.resize(key_count);
    widths.resize(key_count);
    heights.resize(key_count);

    env->GetIntArrayRegion(codes_array, 0, key_count, codes.data());
    env->GetIntArrayRegion(x_coords_array, 0, key_count, x_coords.data());
    env->GetIntArrayRegion(y_coords_array, 0, key_count, y_coords.data());
    env->GetIntArrayRegion(widths_array, 0, key_count, widths.data());
    env->GetIntArrayRegion(heights_array, 0, key_count, heights.data());

    KeyboardLayout keyboard_layout =
            keyboard::decoder::keyboard_layout_tools::CreateKeyboardLayoutFromParams(
                    most_common_key_width, most_common_key_height, keyboard_width,
                    keyboard_height, codes, x_coords, y_coords, widths, heights);
    decoder->SetKeyboardLayout(keyboard_layout);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_simplegestureinput_DecoderTools_decodeGesture(JNIEnv *env, jclass clazz,
                                                               jintArray xs, jintArray ys, jintArray ts,
                                                               jint points_count,
                                                               jbyteArray prev_word) {
    // TODO: implement decodeGesture()
    vector<int> x_coords;
    vector<int> y_coords;
    vector<int> times;
    int pointer_id = 0;

    x_coords.resize(points_count);
    y_coords.resize(points_count);
    times.resize(points_count);

    env->GetIntArrayRegion(xs, 0, points_count, x_coords.data());
    env->GetIntArrayRegion(ys, 0, points_count, y_coords.data());
    env->GetIntArrayRegion(ts, 0, points_count, times.data());

    // sample_distance_in_key_width = 0.25 from decoder-params
    // most_common_key_width = 108
//    const float sample_dist = params_.sample_distance_in_key_widths() *
//                              keyboard_->most_common_key_width();
    const float sample_dist =26;

    string prev = JbyteArrayToString(env, prev_word);
    vector<DecoderResult> results;
    if(decoder){
        TouchSequence* touch_sequence =
                new TouchSequence(x_coords, y_coords, times, pointer_id, sample_dist);
        results = decoder->DecodeTouch(touch_sequence, prev);
    }

    // TODO(zivkovic): Set block_offensive_words in android_decoder_params
    if(results.size()!=0){
        const float autocorrection_confidence =
                decoder->GetAutocorrectThreshold(results[0].score(), y_coords.size());

    }
    jobject decoderResult = getJDecoderResults(env, results);
    return decoderResult;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_simplegestureinput_DecoderTools_deleteDecoderNative(JNIEnv *env, jclass clazz) {
    if(!decoder){
        delete decoder;
    }
}