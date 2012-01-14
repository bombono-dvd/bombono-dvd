//
// mgui/mgui/ffviewer.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#include <mgui/_pc_.h>

#include "ffviewer.h"
#include "img_utils.h"
#include "render/common.h" // FillEmpty()
#include "prefs.h"

#include <mdemux/dvdread.h>

#include <mlib/gettext.h>
#include <mlib/read_stream.h> // ReadAllStream()
#include <mlib/string.h>

// Прямой доступ к ff_codec_bmp_tags, в частности, закрыл, некий
// Anton Khirnov, см. libavformat/libavformat.v (из него генерится скрипт
// для опции --version-script=<script_file> линковщика ld)
// Вообще, можно воспользоваться av_codec_get_tag(), а доступ к ff_codec_bmp_tags
// получить через ff_avi_muxer->codec_tag (самого же найти по имени "avi") или подобный объект,
// но это сейчас неактуально (никто не попадается на ошибку отсутствия кодека) => игра не стоит свеч
//#define CALC_FF_TAG

#ifdef CALC_FF_TAG
// :KLUDGE: потому что riff.h не копируют
C_LINKAGE_BEGIN

typedef struct AVCodecTag {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,39,00)
    enum CodecID id;
#else
    int id;
#endif
    unsigned int tag;
} AVCodecTag;

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,34,00)
static uint FFCodecID2Tag(CodecID codec_id) 
{
    unsigned int ff_codec_get_tag(const AVCodecTag *tags, int id);
    extern const AVCodecTag ff_codec_bmp_tags[];
    return ff_codec_get_tag(ff_codec_bmp_tags, codec_id);
}
#else
static uint FFCodecID2Tag(CodecID codec_id) 
{
    unsigned int codec_get_tag(const AVCodecTag *tags, int id);
    extern const AVCodecTag codec_bmp_tags[];
    return codec_get_tag(codec_bmp_tags, codec_id);
}
#endif

C_LINKAGE_END
#endif // CALC_FF_TAG

static AVStream* VideoStream(FFData& ffv)
{
    return ffv.iCtx->streams[ffv.videoIdx];
}

AVCodecContext* GetVideoCtx(FFData& ffv)
{
    return VideoStream(ffv)->codec;
}

Point DAspectRatio(FFData& ffv)
{
    if( !ffv.IsOpened() )
        return Point(4, 3);

    // по примеру ffplay
    AVRational sample_r = VideoStream(ffv)->sample_aspect_ratio;
    if( !sample_r.num )
        sample_r = GetVideoCtx(ffv)->sample_aspect_ratio;
    if( !sample_r.num || !sample_r.den )
    {
        sample_r.num = 1;
        sample_r.den = 1;
    }

    Point res(sample_r.num*ffv.vidSz.x, sample_r.den*ffv.vidSz.y);
    ReducePair(res);
    return res;
}

static double VideoFrameLength(AVCodecContext* dec, int ticks)
{
    return av_q2d(dec->time_base) * ticks;
}

double FrameFPS(FFData& ffv)
{
    double res = Mpeg::PAL_SECAM_FRAME_FPS;
    if( ffv.IsOpened() )
    {
        // не работает для mpegts (Панама, Плавание)
        // хоть и пишется, что r_frame_rate "just a guess", но
        // все применяют его (mplayer, ffmpeg.c)
        res = av_q2d(VideoStream(ffv)->r_frame_rate);

        // не всегда работает для MPEG4+AVI (Пацаны)
        //AVCodecContext* dec = GetVideoCtx(ffv);
        //res = 1.0/VideoFrameLength(dec, dec->ticks_per_frame);
    }
    return res; 
}

bool IsFTSValid(int64_t ts)
{
    return ts != (int64_t)AV_NOPTS_VALUE;
}

static double AVTime2Sec(int64_t val)
{
    ASSERT( IsFTSValid(val) );
    return val / (double)AV_TIME_BASE;
}

static double Duration(AVFormatContext* ic)
{
    return AVTime2Sec(ic->duration);
}

static double StartTime(AVFormatContext* ic)
{
    return AVTime2Sec(ic->start_time);
}

void CheckOpen(VideoViewer& vwr, const std::string& fname)
{
    bool is_open = vwr.Open(fname.c_str());
    ASSERT_OR_UNUSED( is_open );
}

void RGBOpen(VideoViewer& vwr, const std::string& fname)
{
    //SetOutputFormat(plyr, fofRGB);
    if( !fname.empty() )
        CheckOpen(vwr, fname);
}

double FrameTime(VideoViewer& ffv, int fram_pos)
{
    return fram_pos / FrameFPS(ffv);
}

bool TryGetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, FFViewer& ffv)
{
    bool res = false;
    RefPtr<Gdk::Pixbuf> img_pix = GetRawFrame(time, ffv);
    if( img_pix )
    {
        res = true;
        // заполняем кадр
        if( pix )
            //RGBA::Scale(pix, img_pix);
            RGBA::CopyOrScale(pix, img_pix);
        else
            pix = img_pix->copy();
    }
    return res;
}

RefPtr<Gdk::Pixbuf> GetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, FFViewer& ffv)
{
    if( !TryGetFrame(pix, time, ffv) )
        FillEmpty(pix);
    return pix;
}

double Duration(FFData& ffv)
{
    double res = 0.;
    if( ffv.IsOpened() )
        res = Duration(ffv.iCtx);
    return res;
}

// длина медиа в кадрах
double FramesLength(FFViewer& ffv)
{
    return Duration(ffv) * FrameFPS(ffv);
}

// FFViewer

FFData::FFData(): iCtx(0), videoIdx(-1) {}

bool FFData::IsOpened()
{
    return iCtx != 0;
}

void CloseInfo(FFData& ffi)
{
    if( ffi.IsOpened() )
    {
        // контекст кодека закрывается отдельно
        if( ffi.videoIdx != -1 )
            avcodec_close(GetVideoCtx(ffi));
        ffi.videoIdx = -1;

        // судя по тому как, например, поле ctx_flags нигде не обнуляется
        // (кроме как при инициализации), то повторно использовать структуру
        // не принято -> все заново создаем при переоткрытии
        av_close_input_file(ffi.iCtx);
        ffi.iCtx = 0;
    }
}

static void ResetCurPTS(FFViewer& ffv);

FFViewer::FFViewer(): rgbBuf(0), rgbCnvCtx(0)
{
    ResetCurPTS(*this);
}

FFViewer::~FFViewer()
{
    Close();
}

static bool IsCurPTS(FFViewer& ffv)
{
    return IsTSValid(ffv.curPTS);
}

void FFViewer::Close()
{
    if( IsOpened() )
    {
        ResetCurPTS(*this);

        av_free(rgbBuf);
        rgbBuf = 0;
        sws_freeContext(rgbCnvCtx);
        rgbCnvCtx = 0;
    }

    CloseInfo(*this);
}

static void DumpIFile(AVFormatContext* ic, int idx = 0, const std::string& fname = std::string())
{
    //
    // Инфо о всем контейнере как ее показывает ffmpeg
    //     
    // idx - идентификатор файла (для клиента)
    //const char* fname = "n/a";
    // входной/выходной файл
    int is_output = 0;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,101,00)
    av_dump_format(ic, idx, fname.c_str(), is_output);
#else
    dump_format(ic, idx, fname.c_str(), is_output);
#endif
}

Point VideoSize(AVCodecContext* dec)
{
    return Point(dec->width, dec->height);
}

static bool SeekCall(AVFormatContext* ic, int64_t ts, int flags)
{
    // вполне подойдет поиск по умолчальному потоку (все равно видео выберут)
    int av_res = av_seek_frame(ic, -1, ts, flags);
    return av_res == 0;
}

static bool IsFFError(int av_res)
{
    return av_res < 0;
}

static bool SetIndex(int& idx, int i, bool b)
{
    bool res = (idx == -1) && b;
    if( res )
        idx = i;
    return res;
}

#ifdef CALC_FF_TAG
static unsigned char GetChar(uint tag, int bit_begin)
{
    return (tag>>bit_begin) & 0xFF;
}

static std::string CodecID2Str(CodecID codec_id)
{
#ifdef _MSC_VER
    std::string tag_str = boost::format("%1%") % codec_id % bf::stop;
#else // _MSC_VER
    uint tag = FFCodecID2Tag(codec_id);
    std::string tag_str = boost::format("0x%1$04x") % tag % bf::stop;
    unsigned char c0 = GetChar(tag, 0), c8 = GetChar(tag, 8), 
        c16 = GetChar(tag, 16), c24 = GetChar(tag, 24);
    if( isprint(c0) && isprint(c8) && isprint(c16) && isprint(c24) )
        tag_str = boost::format("%1%%2%%3%%4% / %5%") 
            % c0 % c8 % c16 % c24 % tag_str % bf::stop;
#endif // !_MSC_VER
    return tag_str;
}

#else // CALC_FF_TAG

static std::string CodecID2Str(CodecID codec_id)
{
    return Int2Str(codec_id);
}

#endif // CALC_FF_TAG

bool OpenInfo(FFData& ffi, const char* fname, FFDiagnosis& diag)
{
    std::string& err_str = diag.errStr;

    av_register_all();

    ASSERT( !ffi.IsOpened() );
    bool res = false;

    // AVInputFormat* для форсирования формата контейнера
    // создается из av_find_input_format(str), где str из опции -f для ffmpeg
    // (ffmpeg -formats)
    AVInputFormat* file_iformat = 0;

    AVFormatContext* ic = 0;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,2,0)
    int av_res = avformat_open_input(&ic, fname, file_iformat, 0);
#else
    // для уточнения параметров входного потока; используется в случаях, когда
    // по самому потоку невозможно определить их (не для контейнеров, а для 
    // элементарных потоков
    AVFormatParameters* ap = 0;
    // всегда нуль (ffmpeg, ffplay)
    int buf_size = 0;
    int av_res = av_open_input_file(&ic, fname, file_iformat, buf_size, ap);
#endif
    if( IsFFError(av_res) ) // ошибка
    {
        switch( av_res )
        {
        case AVERROR(ENOENT):
            // :TODO: решить, ставить в конце точки или нет (сообщения пользователю
            // показывается не HIG-ого)
            err_str = _("No such file");
            break;
        case AVERROR(EILSEQ): // использовалось до 0.6, заменено на AVERROR_INVALIDDATA
        // основной источник ошибок, ffmpeg'у предлагается открыть явно не видео (вроде текста),
        // см. код ff_probe_input_buffer()
        // :TODO: однако данный код выбрасывается ffmpeg'ом еще в куче мест; если будет
        // прецендент, когда эта ошибка выбросится не ff_probe_input_buffer() и дезориентирует
        // пользователя, то стоит добавить дополнительную проверку явным вызовом
        // той самой ff_probe_input_buffer()
        case AVERROR_INVALIDDATA:
            err_str = _("Unknown file format");
            break;
        default:
            err_str = boost::format("FFmpeg unknown error: %1%") % av_res % bf::stop;
            break;
        }
    }
    else
    {
        //
        // * файл открыт
        //
        ffi.iCtx = ic;
    
        av_res = av_find_stream_info(ic);
        if( IsFFError(av_res) )
        {
            // например .webm для FFmpeg <= 0.5 
            err_str = BF_("Can't find stream information: %1%") % av_res % bf::stop;
            return false;
        }
        if( LogFilter->IsEnabled(::Log::Info) )
            DumpIFile(ic);
    
        int video_idx = -1, audio_idx = -1;
        for( int i=0; i < (int)ic->nb_streams; i++ )
        {
            AVStream* strm = ic->streams[i];
            AVCodecContext* avctx = strm->codec;
            if( SetIndex(video_idx, i, avctx->codec_type == AVMEDIA_TYPE_VIDEO) )
                ;
            else
                // для демиксера имеет значение только NONE и ALL
                strm->discard = AVDISCARD_ALL;

            SetIndex(audio_idx, i, avctx->codec_type == AVMEDIA_TYPE_AUDIO);
        }

        if( video_idx == -1 )
        {
            err_str = _("No video stream found");
            return false;
        }
        // включить по требованию (и поправить flower.mpg)
        //if( audio_idx == -1 )
        //{
        //    err_str = _("No audio stream found");
        //    return false;
        //}
                    
        if( !IsFTSValid(ic->duration) )
        {
            err_str = _("Can't find the file duration");
            return false;
        }

        if( !IsFTSValid(ic->start_time) )
        {
            // в 99% отсутствие нач. времени - элементарный поток = без контейнера;
            // см. особенности ffmpeg, update_initial_timestamps()
            err_str = _("Start time of the file is unknown");
            diag.isElemStream = true;
            return false;
        }

        // :TRICKY: индекс Duck_Dodgers_101a_Duck_Deception_[Moonsong].avi таков,
        // что в начало не прыгнуть (первое значение > start_time), потому
        // AVSEEK_FLAG_BACKWARD дает неудачу, хотя в целом перемещаться можно => проверяем
        // возможность без AVSEEK_FLAG_BACKWARD
        // Теоретически возможна и обратная ситуация, когда весь индекс < start_time, и
        // только SeekCall(AVSEEK_FLAG_BACKWARD) == true, но реально это будет означать
        // бесполезность индекса => ошибка
        // 
        //if( !SeekCall(ic, ic->start_time, AVSEEK_FLAG_BACKWARD) )
        if( !SeekCall(ic, ic->start_time, 0) )
        {
            // проверка индекса/возможности перемещения
            err_str = _("Can't seek through the file");
            return false;
        }
    
        // открытие кодека
        AVCodecContext* dec = ic->streams[video_idx]->codec;
        // для H.264 и плохих TS
        dec->strict_std_compliance = FF_COMPLIANCE_STRICT;
    
        // Chromium зачем-то выставляет явно, но такие значения уже по умолчанию
        //dec->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
        //dec->error_recognition = FF_ER_CAREFUL;
    
        std::string tag_str = CodecID2Str(dec->codec_id);
        // AVCodec - это одиночка, а AVCodecContext - состояние для него
        // в соответ. потоке контейнера 
        AVCodec* codec = avcodec_find_decoder(dec->codec_id);
        if( !codec )
        {
            err_str = BF_("No decoder found for the stream: %1%") % tag_str % bf::stop;
            return false;
        }

	// :TRICKY: вся полезна инфо о дорожке, включая размеры видео, реально парсится 
	// в av_find_stream_info(), а в avcodec_open() - кодек только привязывается к
	// контексту
	// Более того, в версиях libavcodec 53.4.x-53.9.x есть ошибка, портящая размеры
	// для h.264, в процессе вызова avcodec_open() (попало в Ubuntu Oneiric)
	// (см. b47904d..2214191, черт бы тебя побрал, Felipe Contreras, лезть не в свое дело!)
	Point sz(VideoSize(dec));
	if( sz.IsNull() )
	{
	    err_str = "Video has null size";
	    return false;
	}
	ffi.vidSz = sz;

        if( IsFFError(avcodec_open(dec, codec)) )
        {
            err_str = boost::format("Can't open codec: %1%") % tag_str % bf::stop;
            return false;
        }

        //
        // * декодер настроен
        //
        ffi.videoIdx = video_idx;

        res = true;
    }
    return res;
}

bool OpenInfo(FFData& ffi, const char* fname, std::string& err_str)
{
    FFDiagnosis diag;
    bool res = OpenInfo(ffi, fname, diag);

    err_str.swap(diag.errStr);
    return res;
}

FFInfo::FFInfo() {}

FFInfo::FFInfo(const std::string& fname)
{
    std::string err_str;
    bool res = OpenInfo(*this, fname.c_str(), err_str);
    ASSERT_OR_UNUSED( res );
}

FFInfo::~FFInfo()
{
    CloseInfo(*this);
}

bool FFViewer::Open(const char* fname, std::string& err_str)
{
    // * закрываем открытое ранее
    Close();

    bool res = OpenInfo(*this, fname, err_str);
    if( res )
    {
        Point sz(vidSz);
        // по умолчанию такое использует ffmpeg/ffplay
        // (для переопределения у них используется временный&глобальный
        //     sws_opts = sws_getContext(16,16,0, 16,16,0, sws_flags, NULL,NULL,NULL);
        //     opt_default(); // обновление sws_opts по -sws_flags
        //     sws_flags = av_get_int(sws_opts, "sws_flags", NULL); // = sws_opts.flags    
        int sws_flags = SWS_BICUBIC;
        // при сборке с --enable-runtime-cpudetect (появилось после 0.5), который полюбили пакетировщики,
        // лучшая оптимизация выбирается на этапе выполнения, а не сборке; однако для 0.6 времени
        // maverick оно еще не доделано, см. http://ffmpeg.arrozcru.org/forum/viewtopic.php?f=1&t=1185
        // :KLUDGE: потому добавляем явно
        sws_flags |= SWS_CPU_CAPS_MMX|SWS_CPU_CAPS_MMX2;

        // :TRICKY: почему-то ffmpeg'у "нравится" BGR24 и не нравиться RGB24 в плане использования
        // MMX (ускорения); цена по времени неизвестна,- используем только ради того, чтобы не было 
        // предупреждений
        // Другой вариант - PIX_FMT_RGB32, но там зависимый порядок байтов (в GdkPixbuf - нет) и
        // мы нацелены на RGB24
        // :TODO: с версии LIBSWSCALE_VERSION_INT >= 0.8.11 появился прямой yuv -> rgb24, поправить
        PixelFormat dst_pf = PIX_FMT_BGR24; // PIX_FMT_RGB24;
        rgbCnvCtx = sws_getContext(sz.x, sz.y, GetVideoCtx(*this)->pix_fmt, sz.x, sz.y,
            dst_pf, sws_flags, 0, 0, 0);
        ASSERT( rgbCnvCtx );
    
        Point dst_sz(sz);
        rgbBuf = (uint8_t*)av_malloc(avpicture_get_size(dst_pf, dst_sz.x, dst_sz.y) * sizeof(uint8_t));
        avcodec_get_frame_defaults(&rgbFrame); // не помешает
        avpicture_fill((AVPicture*)&rgbFrame, rgbBuf, dst_pf, dst_sz.x, dst_sz.y);
    }
    else
        // защита от неполных открытий
        Close();
    return res;
}

bool FFViewer::Open(const char* fname)
{
    std::string err;
    return Open(fname, err);
}

static double TS2Time(int64_t ts, FFViewer& ffv)
{
    double tm = INV_TS;
    if( IsFTSValid(ts) )
        tm = ts * av_q2d(VideoStream(ffv)->time_base);
    return tm;
}

// с версий больше чем 0.6 используем skip_frame вместо hurry_up,
// потому что все равно отрубят последний
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,65,0)
#define USE_SKIP_FRAME
#endif

static bool IsInHurry(AVCodecContext* dec)
{
#ifdef USE_SKIP_FRAME
    return dec->skip_frame >= AVDISCARD_NONREF;
#else
    return dec->hurry_up != 0;
#endif
}

static void SetHurryUp(AVCodecContext* dec, bool is_on)
{
#ifdef USE_SKIP_FRAME
    UNUSED_VAR(dec);
    UNUSED_VAR(is_on);
#else
    // как признак (хоть и устаревший)
    dec->hurry_up =  is_on ? 1 : 0;
#endif
}

struct HurryModeEnabler
{
    AVCodecContext* dec;

    HurryModeEnabler(AVCodecContext* dec_): dec(dec_)
    {
        SetHurryUp(dec, true);
        // Прирост скорости (h264): 
        // - AVDISCARD_NONREF: 2x
        // - AVDISCARD_BIDIR: для h264 (и других современных кодеков?) разница в скорости 
        //   с AVDISCARD_NONREF небольшая, зато корректно все необходимые кадры распакуем
        // - AVDISCARD_NONKEY: неотличим от AVDISCARD_ALL, так как I-кадры очень
        //   редки
        // - AVDISCARD_ALL: 3,6-4 (но тогда декодер вообще перестанет выдавать
        //   кадры, например для mpeg4 - без переделки нельзя использовать)
        // 
        // 
        dec->skip_frame = AVDISCARD_NONREF;

        // незначительный прирост дают
        //dec->skip_loop_filter = AVDISCARD_ALL;
        //dec->skip_idct = AVDISCARD_ALL;

    }
   ~HurryModeEnabler()
    {
        SetHurryUp(dec, false);
        dec->skip_frame = AVDISCARD_DEFAULT;
        //dec->skip_idct = AVDISCARD_DEFAULT;
        //dec->skip_loop_filter = AVDISCARD_DEFAULT;
    }
};


// для включения логов (де)кодера на время,
// чтобы другое консоль не забивало
struct CodecDebugEnabler
{
    int oldLvl;

    CodecDebugEnabler(AVCodecContext* dec, int flags)
    {
        oldLvl = av_log_get_level();
        av_log_set_level(AV_LOG_DEBUG);
        dec->debug |= flags;
    }
   ~CodecDebugEnabler()
    {
        av_log_set_level(oldLvl);
    }
};

static void DoVideoDecode(FFViewer& ffv, int& got_picture, AVPacket* pkt)
{
    // FF_DEBUG_PICT_INFO - вывод типов декодируемых картинок
    // FF_DEBUG_MMCO - (h.264) управление зависимыми кадрами + порядок кадров (poc) 
    //CodecDebugEnabler cde(GetVideoCtx(ffv), FF_DEBUG_PICT_INFO);

    AVFrame& picture = ffv.srcFrame;
    avcodec_get_frame_defaults(&picture); // ffmpeg.c очищает каждый раз
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,25,00)
    if( !pkt )
    {
        // никогда бы не использовал alloca(), но не хочется создавать 
        // на стеке лишние байты для исключительных случаев 
        pkt = (AVPacket*)alloca(sizeof(AVPacket));
        av_init_packet(pkt);
        pkt->data = 0;
        pkt->size = 0;
    }
    int av_res = avcodec_decode_video2(GetVideoCtx(ffv), &picture, &got_picture, pkt);
#else
    const uint8_t* buf = 0;
    int buf_sz = 0;
    if( pkt )
    {
        buf = pkt->data;
        buf_sz = pkt->size;
    }
    int av_res = avcodec_decode_video(GetVideoCtx(ffv), &picture, &got_picture, buf, buf_sz);
#endif
    if( av_res < 0 )
    {    
        // ничего не требуется делать в случае ошибок
        LOG_WRN << "Error while decoding frame!" << io::endl;
    }
}

static void ResetCurPTS(FFViewer& ffv)
{
    ffv.curPTS  = INV_TS;
    ffv.prevPTS = INV_TS;
}

static void UpdatePTS(FFViewer& ffv, double new_pts)
{
    ffv.prevPTS = ffv.curPTS;
    ffv.curPTS  = new_pts;
}

static bool DoDecode(FFViewer& ffv)
{
    double cur_pts = ffv.curPTS;
    AVCodecContext* dec = GetVideoCtx(ffv);

    AVPacket pkt;
    int got_picture = 0;

    // предположительное время начала следующего кадра (если не будет явно 
    // установлено) - расчет до следующего av_read_frame()
    double next_pts = cur_pts;
    if( IsTSValid(cur_pts) )
    {
        // в идеале длительность уже была рассчитана в предыдущем pkt->duration;
        // пока же сделаем копипаст как в ffmpeg.c - см. особенности ffmpeg (compute_pkt_fields()) 
        AVStream* st = VideoStream(ffv);
        int ticks    = st->parser ? st->parser->repeat_pict + 1 : dec->ticks_per_frame ;
        next_pts    += VideoFrameLength(dec, ticks);
    }

    bool res = true;
    int av_res = av_read_frame(ffv.iCtx, &pkt);
    if( av_res >= 0 )
    {
        // хотя только одно видео фильтруем, по ходу работы
        // может найтись новый поток - 
        // samples.mplayerhq.hu/MPEG2/dothack2.mpg (субтитры на 8й секунде)
        if( pkt.stream_index == ffv.videoIdx )
        {
            dec->reordered_opaque = pkt.pts;
            
            DoVideoDecode(ffv, got_picture, &pkt);
        }
        av_free_packet(&pkt);
    }
    else if( av_res == AVERROR_EOF ) // для mpegts также -EIO приходит
    {
        // остатки в декодере забираем
        DoVideoDecode(ffv, got_picture, 0);

        if( !got_picture )
            // больше ничего нет, даже того что было
            res = false;
    }
    else
        // остальные проблемы демиксера
        res = false;

    if( res )
    {
        if( got_picture )
        {
            // * PTS текущего кадра
            cur_pts = TS2Time(ffv.srcFrame.reordered_opaque, ffv);
            if( !IsTSValid(cur_pts) )
                cur_pts = next_pts;

            // pts: граничные случаи
            double cur_dts = TS2Time(pkt.dts, ffv);
            if( IsTSValid(cur_dts) )
            {
                if( !IsTSValid(cur_pts) )
                    // первые кадры для avi-контейнеров (не B-кадры) имеют только dts
                    cur_pts = cur_dts;
                else if( IsInHurry(dec) )
                    // помимо не декодирования декодер еще и пропускает B-кадры,
                    // а значит можно пропустить pts
                    cur_pts = std::max(cur_pts, cur_dts);
            }

            UpdatePTS(ffv, cur_pts);
        }
    }
    else
        // очередного кадра нет => следующий SetTime()
        // не должен все обрушить
        ResetCurPTS(ffv);

    return res;
}

static double Delta(double time, double ts, FFViewer& ffv)
{
    ASSERT( IsTSValid(ts) );
    return (time - ts) * FrameFPS(ffv);
}

static double Delta(double time, FFViewer& ffv)
{
    return Delta(time, ffv.curPTS, ffv);
}

const double NEG_WIN_DELTA = -1.; // в кадрах
const double MAX_WIN_DELTA = 300.; // 12.;
// используем вместо нуля из-за погрешности преобразований
// AV_TIME_BASE(int64_t) <-> секунды(double)
const double NULL_DELTA    = 0.0001;

typedef boost::function<bool(FFViewer&)> FFVFunctor;

static bool DecodeLoop(FFViewer& ffv, const FFVFunctor& condition_fnr)
{
    bool res = true;
    while( !condition_fnr(ffv) )
        if( !DoDecode(ffv) )
        {
            res = false;
            break;
        }
    return res;
}

static bool IsFrameFound(double delta)
{
    return delta <= NULL_DELTA;
}

static bool IsFrameFound(double time, double diff, FFViewer& ffv)
{
    return IsFrameFound(Delta(time, ffv) - diff);
}

// доводка до разницы <= diff (в кадрах)
static bool DecodeForDiff(double time, double diff, FFViewer& ffv)
{
    return DecodeLoop(ffv, bb::bind(&IsFrameFound, time, diff, _1));
}

//static bool IsFrameLate(double delta)
//{
//    return delta < NEG_WIN_DELTA - NULL_DELTA;
//}
//
static bool IsFrameLate(double time, FFViewer& ffv)
{
    bool res = false;
    if( IsTSValid(ffv.prevPTS) )
    {
        double delta = Delta(time, ffv.prevPTS, ffv);
        res = IsFrameFound(delta);
    }
    else
        res = Delta(time, ffv) < NEG_WIN_DELTA - NULL_DELTA;
    return res;
}

static bool SeekSetTime(FFViewer& ffv, double time);

static bool DecodeTill(FFViewer& ffv, double time, bool can_seek)
{
    ASSERT( IsCurPTS(ffv) );

    bool res = false;
    // * проверка диапазона
    double orig_delta = Delta(time, ffv);
    bool wish_seek = IsFrameLate(time, ffv) || (orig_delta > MAX_WIN_DELTA);
    if( wish_seek && can_seek )
        res = SeekSetTime(ffv, time);
    else
    {
        if( wish_seek )
        {
            LOG_WRN << "Seek delta overflow: " << orig_delta << io::endl;
            if( orig_delta > 0 )
                // уменьшаем до приемлемого, явно
                time = ffv.curPTS + MAX_WIN_DELTA/FrameFPS(ffv);
        }

        // * допустимая разница, доводим
        LOG_INF << "Decoding delta: " << Delta(time, ffv) << io::endl;

        {
            HurryModeEnabler hme(GetVideoCtx(ffv));
            // кадр может длится <= 3 тактов по декодеру, но явный PTS
            // может и больше
            res = DecodeForDiff(time, 10, ffv);
        }

        // оставшееся в полном режиме
        res = res && DecodeForDiff(time, 0, ffv);
    }

    return res;
}

static bool DoSeek(FFViewer& ffv, int64_t ts, bool is_byte_seek)
{
    // * перемещение
    // если перемещение не прошло (индекс частично поломан), то полагаем, что
    // состояние прежнее (обнуление не требуется)
    int flags = is_byte_seek ? AVSEEK_FLAG_BYTE
        : AVSEEK_FLAG_BACKWARD; // чтоб раньше времени пришли
    bool res = SeekCall(ffv.iCtx, ts, flags);
    if( res )
    {
        // * обнуляем
        ResetCurPTS(ffv);
    
        // сбрасываем буфера декодеров (отдельно от seek)
        avcodec_flush_buffers(GetVideoCtx(ffv));
    
        // * до первого кадра с PTS
        res = DecodeLoop(ffv, &IsCurPTS);
    }
    return res;
}

bool TimeSeek(FFViewer& ffv, double seek_time, double time)
{
    bool res = DoSeek(ffv, AV_TIME_BASE * seek_time, false);

    return res && !IsFrameLate(time, ffv);
}

static double StartTime(FFViewer& ffv)
{
    return StartTime(ffv.iCtx);
}

static bool CanByteSeek(AVFormatContext* ic)
{
    // переход по позиции не работает для avi, mkv - см. особенности ffmpeg
    // однако для без-заголовочных демиксеров (MPEG-PS, MPEG-TS) требуется

    typedef std::map<std::string, AVInputFormat*> Map;
    static Map map;
    if( map.empty() )
    {
        // для видео < 1 секунды показывает пусто
        map["mpeg"]   = av_find_input_format("mpeg");
        // перейти в начало иногда возможно только так,- PanamaCanal_1080p-h264.ts
        map["mpegts"] = av_find_input_format("mpegts");
    }

    bool res = false;
    boost_foreach( Map::reference ref, map )
        if( ic->iformat == ref.second )
        {
            res = true;
            break;
        }
    return res;
}

std::string PrefContents(const char* fname)
{
    std::string user_opts = ReadAllStream(PreferencesPath(fname));
    // только первая строка
    size_t eol = user_opts.find_first_of("\n\r");
    if( eol != std::string::npos )
        user_opts = std::string(user_opts.c_str(), eol);
    return user_opts;
}

bool ReadPref(const char* name, RPData& rp)
{
    bool& is_read = rp.isRead;
    
    bool res = !is_read;
    if( res )
    {
        is_read = true;
        rp.val = PrefContents(name);
    }
    return res;
}

static bool SeekSetTime(FFViewer& ffv, double time)
{
    bool is_begin = false;
    double start_time = StartTime(ffv);
    for( int i=0; i<4 && !is_begin; i++ )
    {
        static double seek_shift = 0.;
        static RPData ss_rp;
        if( ReadPref("av_seek_shift", ss_rp) )
        {
            std::string str = ss_rp.val;
            if( !str.empty() )
            {
                double val;
                if( Str::GetType<double>(val, str.c_str()) )
                {
                    const double max_val = 15;
                    if ( (val < -max_val) || (val > max_val) )
                        LOG_WRN << "Value from av_seek_shift is out of range: " << max_val << io::endl;
                    else
                        seek_shift = val;
                }
                else
                    LOG_WRN << "Can't read float from av_seek_shift" << io::endl;
            }
        }

        int n = (1 << i) - 1; // 0, 1, 3, 7
        double seek_time = time + seek_shift - n;

        if( seek_time <= start_time )
        {
            is_begin = true;
            break;
        }

        if( TimeSeek(ffv, seek_time, time) )
            break;
    }

    if( is_begin )
    {
        bool seek_ok = TimeSeek(ffv, start_time, time);
        if( !seek_ok && CanByteSeek(ffv.iCtx) )
            // тогда переходим в начало файла
            seek_ok = DoSeek(ffv, ffv.iCtx->data_offset, true);

        // некоторое видео глючит в начале (Hellboy), из-за чего
        // последовательный доступ выполняется с перескоками -
        // явно ставим пред. кадр
        if( seek_ok && IsCurPTS(ffv) )
            // :KLUDGE: -1 уже занят, поэтому -0.5
            // (система работает, пока start_time не бывает отрицательным)
            ffv.prevPTS = start_time - 0.5;
    }

    return IsCurPTS(ffv) && DecodeTill(ffv, time, false);
}

// время без смещения
bool SetTime(FFViewer& ffv, double time)
{
    ASSERT( ffv.IsOpened() );

    if( (time < 0) || (time > Duration(ffv.iCtx)) )
        return false;
    time += StartTime(ffv);

    // :TODO: рассчитывать только если включено логирование
    double GetClockTime();
    double cur_time = GetClockTime();

    bool res = false;
    if( !IsCurPTS(ffv) )
        res = SeekSetTime(ffv, time);
    else
        res = DecodeTill(ffv, time, true);

    if( res )
    {
        // декодер должен выдать результат
        ASSERT( ffv.srcFrame.data[0] );

        // * перевод в RGB
        Point sz(ffv.vidSz);
        // не допускаем смены разрешения в меньшую сторону, чтобы
        // не вылететь при скалировании; вообще задумано, что разрешение
        // не меняется
        // :TODO: по требованию реализовать смену обновлением контекста 
        // (размер rgb_frame оставить постоянным!) 
        // Пример смены: ElephDream_720-h264.mov, 405->406; причем 
        // vlc видит оба разрешения как Resolution/Display Resolution
        ASSERT_RTL( !(VideoSize(GetVideoCtx(ffv)) < sz) );
        AVFrame& rgb_frame = ffv.rgbFrame;
        // не очень понятно как пользовать аргументы 4, 5
        sws_scale(ffv.rgbCnvCtx, ffv.srcFrame.data, ffv.srcFrame.linesize,
                  0, sz.y, rgb_frame.data, rgb_frame.linesize);
        uint8_t* buf = rgb_frame.data[0];
        uint8_t* ptr = buf;
        uint8_t tmp;
        for( int y=0; y<sz.y; y++ )
            for( int x=0; x<sz.x; x++, ptr += 3 )
            {
                // b <-> r
                tmp = ptr[0];
                ptr[0] = ptr[2];
                ptr[2] = tmp;
            }
    }

    LOG_INF << "Time setting: " << time << "; current PTS: " << ffv.curPTS << "; previous PTS: " << ffv.prevPTS << io::endl;
    LOG_INF << "SetTime() timing: " << GetClockTime() - cur_time << io::endl;

    return res;
}

RefPtr<Gdk::Pixbuf> GetRawFrame(double time, FFViewer& ffv)
{
    RefPtr<Gdk::Pixbuf> res_pix;
    if( ffv.IsOpened() && SetTime(ffv, time) )
        res_pix = CreateFromData(ffv.rgbFrame.data[0], ffv.vidSz, false);
    return res_pix;
}

namespace DVD {

struct VobCtx
{
   int64_t  curPos;
   VobFile  vFile;
   
   VobCtx(VobPtr vob, dvd_reader_t* dvd): curPos(0), vFile(vob, dvd) {}
};
    
// только для передачи параметров для протокола bmdvob
static VobPtr BmdVob;    
static dvd_reader_t* BmdDVD = 0;    

static int VobOpen(URLContext *h, const char *filename, int flags)
{
    // параметры передаем внешним образом
    ASSERT( strcmp(filename, "bmdvob:") == 0 );
    ASSERT( flags == URL_RDONLY );
    
    VobCtx* vc = new VobCtx(BmdVob, BmdDVD);
    h->priv_data = (void*) vc;
    return 0;
}

static int64_t DoSeek(VobCtx* vc, int64_t n_pos, int whence)
{
    int64_t cnt  = Size(vc->vFile);
    int64_t& pos = vc->curPos;
    switch( whence )
    {
    case SEEK_SET:
        pos = n_pos;
        break;
    case SEEK_CUR:
        pos += n_pos;
        break;
    case SEEK_END:
        pos = cnt + n_pos;
        break;
    default:
        ASSERT(0);
    }
    pos = std::max(int64_t(0), pos);
    pos = std::min(pos, cnt);
    return pos;
}

static int VobRead(URLContext *h, unsigned char *buf, int sz)
{
    VobCtx* vc = (VobCtx*)h->priv_data;
    int ret = -1;
    if( sz > 0 )
    {
        int64_t old_pos = vc->curPos;
        ret = DoSeek(vc, sz, SEEK_CUR) - old_pos;
        
        try
        {
            ReadVob((char*)buf, ret, vc->vFile, old_pos);
        }
        catch( const std::exception& exc )
        {
            LOG_INF << "VobRead fail: " << exc.what() << io::endl;
            ret = -1;
        }
    }
    return ret;
}

static int64_t VobSeek(URLContext *h, int64_t n_pos, int whence)
{
    VobCtx* vc = (VobCtx*)h->priv_data;
    
    int64_t ret = 0;
    if( whence == AVSEEK_SIZE )
        ret = Size(vc->vFile);
    else
        ret = DoSeek(vc, n_pos, whence);
    
    return ret;
}

static int VobClose(URLContext *h)
{
    delete (VobCtx*)h->priv_data;
    return 0;
}
    
static void RegisterVobProt()
{
    static bool is_init = false;
    if( !is_init )
    {
        is_init = true;
        
        // так как постоянно добавляют атрибуты, то лучше так
        //static URLProtocol bmdvob_up = {
        //    "bmdvob",
        //    VobOpen,
        //    VobRead,
        //    0, //file_write,
        //    VobSeek,
        //    VobClose,
        //    0,0,0,0
        //};
        static URLProtocol bmdvob_up;
        int sz = sizeof(URLProtocol);
        memset(&bmdvob_up, 0, sz);

        bmdvob_up.name = "bmdvob";
        bmdvob_up.url_open  = VobOpen;
        bmdvob_up.url_read  = VobRead;
        bmdvob_up.url_seek  = VobSeek;
        bmdvob_up.url_close = VobClose;
                
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 69, 0)
        // :TRICKY: с некоторого момента сделали его deprecated,
        // а вместо него ffurl_register_protocol(), только в публичном заголовке его нет
        av_register_protocol2(&bmdvob_up, sz);
#else
        av_register_protocol(&bmdvob_up);
#endif
    }
}
   
bool OpenVob(FFViewer& ffv, VobPtr vob, dvd_reader_t* dvd, std::string& err_str)
{
    RegisterVobProt();
    
    BmdVob = vob;
    BmdDVD = dvd;
    bool res = ffv.Open("bmdvob:", err_str);
    BmdVob = 0;
    BmdDVD = 0;
    return res;
}

} // namespace DVD

