
#include <mgui/_pc_.h>

#include "ffviewer.h"
#include "img_utils.h"
#include "render/common.h" // FillEmpty()

static AVStream* VideoStream(FFViewer& ffv)
{
    return ffv.iCtx->streams[ffv.videoIdx];
}

static AVCodecContext* GetVideoCtx(FFViewer& ffv)
{
    return VideoStream(ffv)->codec;
}

Point DAspectRatio(VideoViewer& ffv)
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

double FrameFPS(FFViewer& ffv)
{
    double res = Mpeg::PAL_SECAM_FRAME_FPS;
    if( ffv.IsOpened() )
        // хоть и пишется, что r_frame_rate "just a guess", но
        // все применяют его (mplayer, ffmpeg.c)
        res = av_q2d(VideoStream(ffv)->r_frame_rate);
    return res; 
}

static double AVTime2Sec(int64_t val)
{
    ASSERT( val != (int64_t)AV_NOPTS_VALUE );
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

// :REFACTOR: убрать копипаст в 
// 1 test_ffmpeg.cpp
// +2 здесь
// 3 общее API с Mpeg::Player

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

RefPtr<Gdk::Pixbuf> GetRawFrame(double time, FFViewer& ffv);

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

double Duration(FFViewer& ffv)
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

FFViewer::FFViewer(): iCtx(0), videoIdx(-1), curPTS(INV_TS),
    rgbBuf(0), rgbCnvCtx(0)
{
}

FFViewer::~FFViewer()
{
    Close();
}

bool FFViewer::IsOpened()
{
    return iCtx != 0;
}

static void ResetCurPTS(FFViewer& ffv)
{
    ffv.curPTS = INV_TS;
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

        // контекст кодека закрывается отдельно
        if( videoIdx != -1 )
            avcodec_close(GetVideoCtx(*this));

        // судя по тому как, например, поле ctx_flags нигде не обнуляется
        // (кроме как при инициализации), то повторно использовать структуру
        // не принято -> все заново создаем при переоткрытии
        av_close_input_file(iCtx);
        iCtx = 0;
    }
}

static void DumpIFile(AVFormatContext* ic, int idx = 0)
{
    //
    // Инфо о всем контейнере как ее показывает ffmpeg
    //     
    // idx - идентификатор файла (для клиента)
    const char* fname = "n/a";
    // входной/выходной файл
    int is_output = 0;
    dump_format(ic, idx, fname, is_output);
}

Point VideoSize(AVCodecContext* dec)
{
    return Point(dec->width, dec->height);
}

static bool SeekCall(AVFormatContext* ic, int64_t ts, bool is_byte_seek)
{
    int flags = is_byte_seek ? AVSEEK_FLAG_BYTE
        : AVSEEK_FLAG_BACKWARD; // чтоб раньше времени пришли

    // вполне подойдет поиск по умолчальному потоку (все равно видео выберут)
    int av_res = av_seek_frame(ic, -1, ts, flags);
    return av_res == 0;
}

bool FFViewer::Open(const char* fname, std::string& /*err_str*/)
{
    bool res = true;

    av_register_all();
    // * закрываем открытое ранее
    Close();

    // AVInputFormat* для форсирования формата контейнера
    // создается из av_find_input_format(str), где str из опции -f для ffmpeg
    // (ffmpeg -formats)
    AVInputFormat* file_iformat = 0;
    // для уточнения параметров входного потока; используется в случаях, когда
    // по самому потоку невозможно определить их (не для контейнеров, а для 
    // элементарных потоков
    AVFormatParameters* ap = 0;
    // всегда нуль (ffmpeg, ffplay)
    int buf_size = 0;

    AVFormatContext* ic = 0;
    int av_res = av_open_input_file(&ic, fname, file_iformat, buf_size, ap);
    // :TODO: обработка ошибок открытия файла
    ASSERT_RTL( !av_res );
    //
    // * файл открыт
    //
    iCtx = ic;

    av_res = av_find_stream_info(ic);
    //if (err < 0) {
    //    fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
    //    ret = -1;
    //    goto fail;
    //}
    ASSERT_RTL( av_res >= 0 );

    DumpIFile(ic);

    ASSERT_RTL( ic->duration != (int64_t)AV_NOPTS_VALUE );
    // в 99% отсутствие нач. времени - элементарный поток = без контейнера;
    // см. особенности ffmpeg, update_initial_timestamps()
    ASSERT_RTL( ic->start_time != (int64_t)AV_NOPTS_VALUE );
    // проверка индекса/возможности перемещения
    ASSERT_RTL( SeekCall(ic, ic->start_time, false) );

    // в секундах
    double dur = Duration(ic);
    io::cout << "duration: " << dur << io::endl;

    // открытие кодека
    int video_idx = -1;
    for( int i=0; i < (int)ic->nb_streams; i++ )
    {
        AVStream* strm = ic->streams[i];
        AVCodecContext* avctx = strm->codec;
        if( (video_idx == -1) && (avctx->codec_type == CODEC_TYPE_VIDEO) )
            video_idx = i;
        else
            // для демиксера имеет значение только NONE и ALL
            strm->discard = AVDISCARD_ALL;
    }
    ASSERT( video_idx != -1 );
    AVCodecContext* dec = ic->streams[video_idx]->codec;

    // Chromium зачем-то выставляет явно, но такие значения уже по умолчанию
    //dec->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    //dec->error_recognition = FF_ER_CAREFUL;

    // AVCodec - это одиночка, а AVCodecContext - состояние для него
    // в соответ. потоке контейнера 
    AVCodec* codec = avcodec_find_decoder(dec->codec_id);
    ASSERT( codec );

    av_res = avcodec_open(dec, codec);
    ASSERT( !av_res );
    //
    // * декодер настроен
    //
    videoIdx = video_idx;

    Point sz(VideoSize(dec));
    ASSERT_RTL( sz.x );
    vidSz = sz;
    // по умолчанию такое использует ffmpeg/ffplay
    // (для переопределения у них используется временный&глобальный
    //     sws_opts = sws_getContext(16,16,0, 16,16,0, sws_flags, NULL,NULL,NULL);
    //     opt_default(); // обновление sws_opts по -sws_flags
    //     sws_flags = av_get_int(sws_opts, "sws_flags", NULL); // = sws_opts.flags    
    int sws_flags = SWS_BICUBIC;
    // :TRICKY: почему-то ffmpeg'у "нравится" BGR24 и не нравиться RGB24 в плане использования
    // MMX (ускорения); цена по времени неизвестна,- используем только ради того, чтобы не было 
    // предупреждений
    // Другой вариант - PIX_FMT_RGB32, но там зависимый порядок байтов (в GdkPixbuf - нет) и
    // мы нацелены на RGB24
    PixelFormat dst_pf = PIX_FMT_BGR24; // PIX_FMT_RGB24;
    rgbCnvCtx = sws_getContext(sz.x, sz.y, dec->pix_fmt, sz.x, sz.y,
        dst_pf, sws_flags, 0, 0, 0);
    ASSERT( rgbCnvCtx );

    Point dst_sz(sz);
    uint8_t* rgbBuf = (uint8_t*)av_malloc(avpicture_get_size(dst_pf, dst_sz.x, dst_sz.y) * sizeof(uint8_t));
    avcodec_get_frame_defaults(&rgbFrame); // не помешает
    avpicture_fill((AVPicture*)&rgbFrame, rgbBuf, dst_pf, dst_sz.x, dst_sz.y);

    if( !res )
        // защита от неполных открытий
        Close();
    return res;
}

bool FFViewer::Open(const char* fname)
{
    std::string err;
    return Open(fname, err);
}

// :REFACTOR:
bool IsFTSValid(int64_t ts)
{
    return ts != (int64_t)AV_NOPTS_VALUE;
}

static double TS2Time(int64_t ts, FFViewer& ffv)
{
    double tm = INV_TS;
    if( IsFTSValid(ts) )
        tm = ts * av_q2d(VideoStream(ffv)->time_base);
    return tm;
}

static bool IsInHurry(AVCodecContext* dec)
{
    return dec->hurry_up != 0;
}

struct HurryModeEnabler
{
    AVCodecContext* dec;

    HurryModeEnabler(AVCodecContext* dec_): dec(dec_)
    {
        // как признак (хоть и устаревший)
        dec->hurry_up = 1;
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
        dec->hurry_up = 0;
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
    const uint8_t* buf = 0;
    int buf_sz = 0;
    if( pkt )
    {
        buf = pkt->data;
        buf_sz = pkt->size;
    }

    // FF_DEBUG_PICT_INFO - вывод типов декодируемых картинок
    // FF_DEBUG_MMCO - (h.264) управление зависимыми кадрами + порядок кадров (poc) 
    //CodecDebugEnabler cde(GetVideoCtx(ffv), FF_DEBUG_PICT_INFO);

    AVFrame& picture = ffv.srcFrame;
    avcodec_get_frame_defaults(&picture); // ffmpeg.c очищает каждый раз
    int av_res = avcodec_decode_video(GetVideoCtx(ffv), &picture, &got_picture, buf, buf_sz);
    if( av_res < 0 )
        // ничего не требуется делать в случае ошибок
        LOG_WRN << "Error while decoding frame!" << io::endl;
}

static bool DoDecode(FFViewer& ffv)
{
    double& cur_pts  = ffv.curPTS;
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
        next_pts    += av_q2d(dec->time_base) * ticks;
    }

    bool res = true;
    int av_res = av_read_frame(ffv.iCtx, &pkt);
    if( av_res >= 0 )
    {
        // только одно видео фильтруем
        ASSERT_RTL( pkt.stream_index == ffv.videoIdx );
    
        dec->reordered_opaque = pkt.pts;
        
        DoVideoDecode(ffv, got_picture, &pkt);
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
                    cur_pts = std::min(cur_pts, cur_dts);
            }
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

static bool IsFrameFound(double delta)
{
    return delta <= NULL_DELTA;
}

static bool IsFrameLate(double delta)
{
    return delta < NEG_WIN_DELTA - NULL_DELTA;
}

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

static bool IsFrameFound(double time, double diff, FFViewer& ffv)
{
    return IsFrameFound(Delta(time, ffv) - diff);
}

// доводка до разницы <= diff (в кадрах)
static bool DecodeForDiff(double time, double diff, FFViewer& ffv)
{
    return DecodeLoop(ffv, bb::bind(&IsFrameFound, time, diff, _1));
}

static bool SeekSetTime(FFViewer& ffv, double time);

static bool DecodeTill(FFViewer& ffv, double time, bool can_seek)
{
    ASSERT( IsCurPTS(ffv) );

    bool res = false;
    // * проверка диапазона
    double delta   = Delta(time, ffv);
    bool wish_seek = IsFrameLate(delta) || (delta > MAX_WIN_DELTA);
    if( wish_seek && can_seek )
        res = SeekSetTime(ffv, time);
    else
    {
        if( wish_seek )
        {
            LOG_WRN << "Seek delta overflow: " << delta << io::endl;
            if( delta > 0 )
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
    return res;
}

static bool DoSeek(FFViewer& ffv, int64_t ts, bool is_byte_seek)
{
    // * перемещение
    // если перемещение не прошло (индекс частично поломан), то полагаем, что
    // состояние прежнее (обнуление не требуется)
    bool res = SeekCall(ffv.iCtx, ts, is_byte_seek);
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

    return res && !IsFrameLate(Delta(time, ffv));
}

static double StartTime(FFViewer& ffv)
{
    return StartTime(ffv.iCtx);
}

static bool CanByteSeek(AVFormatContext* ic)
{
    // переход по позиции не работает для avi, mkv - см. особенности ffmpeg
    // однако для без-заголовочных демиксеров (MPEG-PS, MPEG-TS)
    // перейти в начало иногда возможно только так,- PanamaCanal_1080p-h264.ts
    static AVInputFormat* mpegts_demuxer = 0;
    if( !mpegts_demuxer )
        mpegts_demuxer = av_find_input_format("mpegts");

    return ic->iformat == mpegts_demuxer;
}

static bool SeekSetTime(FFViewer& ffv, double time)
{
    bool is_begin = false;
    double start_time = StartTime(ffv);
    for( int i=0; i<4 && !is_begin; i++ )
    {
        int n = (1 << i) - 1; // 0, 1, 3, 7
        double seek_time = time - n;

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
        if( !TimeSeek(ffv, start_time, time) && CanByteSeek(ffv.iCtx) )
            // тогда переходим в начало файла
            DoSeek(ffv, ffv.iCtx->data_offset, true);
        //TimeSeek(ffv, start_time, time);
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

    double GetClockTime();
    double cur_time = GetClockTime();

    bool res = false;
    if( !IsCurPTS(ffv) )
        res = SeekSetTime(ffv, time);
    else
        res = DecodeTill(ffv, time, true);

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

