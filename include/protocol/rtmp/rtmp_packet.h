#pragma once

#include "network/flat_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace rtmp {

static constexpr size_t kRandomHandshakeSize = 1528;
static constexpr size_t kDefaultChunkLength = 128;

#define MSG_SET_CHUNK 1    /*Set Chunk Size (1)*/
#define MSG_ABORT 2        /*Abort Message (2)*/
#define MSG_ACK 3          /*Acknowledgement (3)*/
#define MSG_USER_CONTROL 4 /*User Control Messages (4)*/
#define MSG_WIN_SIZE 5     /*Window Acknowledgement Size (5)*/
#define MSG_SET_PEER_BW 6  /*Set Peer Bandwidth (6)*/
#define MSG_AUDIO 8        /*Audio Message (8)*/
#define MSG_VIDEO 9        /*Video Message (9)*/
#define MSG_DATA 18        /*Data Message (18, 15) AMF0*/
#define MSG_DATA3 15       /*Data Message (18, 15) AMF3*/
#define MSG_CMD 20         /*Command Message AMF0 */
#define MSG_CMD3 17        /*Command Message AMF3 */
#define MSG_OBJECT3 16     /*Shared Object Message (19, 16) AMF3*/
#define MSG_OBJECT 19      /*Shared Object Message (19, 16) AMF0*/
#define MSG_AGGREGATE 22   /*Aggregate Message (22)*/

#define CHUNK_NETWORK 2               /*网络相关的消息(参见 Protocol Control Messages)*/
#define CHUNK_SYSTEM 3                /*向服务器发送控制消息(反之亦可)*/
#define CHUNK_CLIENT_REQUEST_BEFORE 3 /*客户端在createStream前,向服务器发出请求的chunkID*/
#define CHUNK_CLIENT_REQUEST_AFTER 4  /*客户端在createStream后,向服务器发出请求的chunkID*/
#define CHUNK_AUDIO 6                 /*音频chunkID*/
#define CHUNK_VIDEO 7                 /*视频chunkID*/

#pragma pack(push, 1)

/// rtmp spec P8
struct rtmp_handshake
{
    rtmp_handshake();

    uint8_t time_stamp_[4] = {0};
    uint8_t zero_[4] = {0};
    uint8_t random_[kRandomHandshakeSize];
};

/// rtmp spec P14
struct rtmp_header
{
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t fmt : 2;
    uint8_t chunk_id : 6;
#else
    uint8_t chunk_id : 6;
    uint8_t fmt : 2;
#endif
    uint8_t time_stamp[3];
    uint8_t msg_length[3];
    uint8_t msg_type_id;
    uint8_t msg_stream_id[4];
};

#pragma pack(pop)

// enhanced-rtmp.pdf P5
enum class rtmp_av_frame_type : uint8_t
{
    reserved = 0,
    key_frame = 1,              // key frame (for AVC, a seekable frame)
    inter_frame = 2,            // inter frame (for AVC, a non-seekable frame)
    disposable_inter_frame = 3, // disposable inter frame (H.263 only)
    generated_key_frame = 4,    // generated key frame (reserved for server use only)
    video_info_frame = 5,       // video info/command frame
};

enum class rtmp_h264_packet_type : uint8_t
{
    h264_config_header = 0, // AVC or HEVC sequence header(sps/pps)
    h264_nalu = 1,          // AVC or HEVC NALU
    h264_end_seq = 2,       // AVC or HEVC end of sequence (lower level NALU sequence
                            // ender is not REQUIRED or supported)
};

#define MKBETAG(a, b, c, d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

// enhanced-rtmp.pdf P7
enum class rtmp_video_codec : uint32_t
{
    h263 = 2,          // Sorenson H.263
    screen_video = 3,  // Screen video
    vp6 = 4,           // On2 VP6
    vp6_alpha = 5,     // On2 VP6 with alpha channel
    screen_video2 = 6, // Screen video version 2
    h264 = 7,          // avc
    h265 = 12,         // 国内扩展

    fourcc_vp9 = MKBETAG('v', 'p', '0', '9'),
    fourcc_av1 = MKBETAG('a', 'v', '0', '1'),
    fourcc_hevc = MKBETAG('h', 'v', 'c', '1')
};

// video_file_format_spec_v10_1.pdf P76
enum class rtmp_aac_packet_type : uint8_t
{
    aac_config_header = 0, // AAC sequence header
    aac_raw = 1,           // AAC raw
};

enum class rtmp_audio_codec : uint8_t
{
    /**
    0 = Linear PCM, platform endian
    1 = ADPCM
    2 = MP3
    3 = Linear PCM, little endian
    4 = Nellymoser 16 kHz mono
    5 = Nellymoser 8 kHz mono
    6 = Nellymoser
    7 = G.711 A-law logarithmic PCM
    8 = G.711 mu-law logarithmic PCM
    9 = reserved
    10 = AAC
    11 = Speex
    14 = MP3 8 kHz
    15 = Device-specific sound
     */
    g711a = 7,
    g711u = 8,
    aac = 10,
    opus = 13 // 国内扩展
};

// enhanced-rtmp.pdf P8
enum class rtmp_av_ext_packet_type : uint8_t
{
    PacketTypeSequenceStart = 0,
    PacketTypeCodedFrames = 1,
    PacketTypeSequenceEnd = 2,
    PacketTypeCodedFramesX = 3,
    PacketTypeMetadata = 4,
    PacketTypeMPEG2TSSequenceStart = 5,
};

enum class rtmp_flv_codec_id : int8_t
{
    non_av = -1,
    h264 = 7,
    aac = 10,
};

class rtmp_packet : public std::enable_shared_from_this<rtmp_packet>
{
public:
    using ptr = std::shared_ptr<rtmp_packet>;

    static ptr create();

    bool is_abs_stamp{false};
    uint32_t chunk_stream_id;
    uint32_t msg_stream_id;
    uint32_t msg_length;
    uint8_t msg_type_id;
    uint32_t ts_delta;
    uint32_t time_stamp;

    ~rtmp_packet() = default;
    rtmp_packet(const rtmp_packet &) = delete;
    rtmp_packet &operator=(const rtmp_packet &) = delete;
    rtmp_packet(rtmp_packet &&) = delete;
    rtmp_packet &operator=(rtmp_packet &&) = delete;

    rtmp_packet &restore_context(const rtmp_packet &);

    bool is_video_keyframe() const;
    bool is_config_frame() const;
    rtmp_flv_codec_id get_av_codec_id() const;

    void set_pkt_header_length(size_t);

    size_t size();

    const network::flat_buffer::ptr &buf();

private:
    rtmp_packet();

private:
    network::flat_buffer::ptr buf_;
    size_t pkt_header_length_{0};
};

} // namespace rtmp