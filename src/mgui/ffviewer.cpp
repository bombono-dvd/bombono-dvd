
#include <mgui/_pc_.h>

#include "ffviewer.h"
#include "mguiconst.h"
#include "img_utils.h"

#include <mdemux/demuxconst.h> // IsTSValid()

// :REFACTOR: убрать копипаст в 
// 1 test_ffmpeg.cpp
// 2 здесь

struct FFViewer
{
    AVFormatContext* iCtx;
                int  videoIdx;
                     // время текущего кадра
             double  curPTS;

            AVFrame  srcFrame;
            AVFrame  rgbFrame;
            uint8_t* rgbBuf;
         SwsContext* rgbCnvCtx;
              Point  vidSz; // лучше бы с rgbCnvCtx брать, но
                            // не дают


                    FFViewer();
                   ~FFViewer();

              bool  Open(const char* fname);
              bool  IsOpened();
              void  Close();
};

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

static AVStream* VideoStream(FFViewer& ffv)
{
    return ffv.iCtx->streams[ffv.videoIdx];
}

static AVCodecContext* GetVideoCtx(FFViewer& ffv)
{
    return VideoStream(ffv)->codec;
}

void FFViewer::Close()
{
    if( IsOpened() )
    {
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

bool FFViewer::Open(const char* fname)
{
    bool result = true;

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
    int res = av_open_input_file(&ic, fname, file_iformat, buf_size, ap);
    // :TODO: обработка ошибок открытия файла
    ASSERT_RTL( !res );
    //
    // * файл открыт
    //
    iCtx = ic;

    int err = av_find_stream_info(ic);
    //if (err < 0) {
    //    fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
    //    ret = -1;
    //    goto fail;
    //}
    ASSERT_RTL( err >= 0 );

    DumpIFile(ic);
    ASSERT_RTL( ic->duration   != (int64_t)AV_NOPTS_VALUE );
    // начальное время не сущ. в редких случаях - оно устанавливается
    // явно (avi) либо через минимум по элементарным потокам (mpeg-подобные)
    ASSERT_RTL( ic->start_time != (int64_t)AV_NOPTS_VALUE );

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

    // AVCodec - это одиночка, а AVCodecContext - состояние для него
    // в соответ. потоке контейнера 
    AVCodec* codec = avcodec_find_decoder(dec->codec_id);
    ASSERT( codec );

    res = avcodec_open(dec, codec);
    ASSERT( !res );
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

    if( !result )
        // защита от неполных открытий
        Close();
    return result;
}

double FrameFPS(FFViewer& ffv)
{
    // хоть и пишется, что r_frame_rate "just a guess", но
    // все применяют его (mplayer, ffmpeg.c)
    return av_q2d(VideoStream(ffv)->r_frame_rate);
}

static double TS2Time(int64_t ts, FFViewer& ffv)
{
    double tm = INV_TS;
    if( ts != (int64_t)AV_NOPTS_VALUE )
        tm = ts * av_q2d(VideoStream(ffv)->time_base);
    return tm;
}

static void DoDecode(FFViewer& ffv)
{
    double& cur_pts  = ffv.curPTS;
    AVFrame& picture = ffv.srcFrame;
    AVCodecContext* dec = GetVideoCtx(ffv);

    AVPacket pkt;
    int got_picture;

    // предположительное время начала следующего кадра (если не будет явно 
    // установлено) - расчет до следующего av_read_frame()
    double next_pts = cur_pts;
    if( IsTSValid(cur_pts) )
    {
        // судя по всему, код ffplay(.c) устарел - длительность кадра вычисляется по 
        // frame->repeat_pict, который всегда ноль;
        // а вот ffmpeg делает хитрее, через "parser" (аналог моего Mpeg::Decoder'а),-
        // тот repeat_pict заполняет правильно, и с учетом dec->time_base
        // (1 + 1|2|3|5] = 2|3|4|6 полутактов для MPEG2)
        AVStream* st = VideoStream(ffv);
        int ticks    = st->parser ? st->parser->repeat_pict + 1 : dec->ticks_per_frame ;
        next_pts    += av_q2d(dec->time_base) * ticks;
    }

    int av_res = av_read_frame(ffv.iCtx, &pkt);
    ASSERT( av_res >= 0 );

    // только одно видео фильтруем
    ASSERT_RTL( pkt.stream_index == ffv.videoIdx );

    dec->reordered_opaque = pkt.pts;

    avcodec_get_frame_defaults(&picture); // ffmpeg.c очищает каждый раз
    av_res = avcodec_decode_video(dec, &picture, &got_picture, pkt.data, pkt.size);
    ASSERT_RTL( av_res >= 0 );

    av_free_packet(&pkt);
    if( got_picture )
    {
        // * PTS текущего кадра
        // Варианты вычисления pts:
        // - mplayer: только по pkt.pts с переупорядочиваем
        // - ffplay:  pkt.dts + довычисление
        // - mlt:     только pkt.dts
        // => делаем как mplayer + доводка как у ffmpeg
        int64_t ff_pts = picture.reordered_opaque;
        //if( ff_pts == AV_NOPTS_VALUE )
        //    ff_pts = pkt->dts;
        cur_pts = TS2Time(ff_pts, ffv);
        if( !IsTSValid(cur_pts) )
            cur_pts = next_pts;

        //double cur_dts = TS2Time(pkt.dts, ffv);
        //io::cout << "real pts: " << cur_pts << "; pts: " << TS2Time(pkt.pts, ffv)
        //         << "; dts: " << cur_dts << io::endl;
        //if( IsTSValid(cur_pts) )
        //    ASSERT( fabs(cur_pts - cur_dts) < 0.001 );
    }
}

static bool SeekSetTime(FFViewer& ffv, double time);

static bool DecodeTill(FFViewer& ffv, double time, bool can_seek)
{
    double& cur_pts = ffv.curPTS;
    ASSERT( IsTSValid(cur_pts) );

    const double MIN_WIN_DELTA = -1.; // в кадрах
    const double MAX_WIN_DELTA = 6.;

    bool res = false;
    // * проверка диапазона
    double delta = (time - cur_pts) * FrameFPS(ffv);
    if( (delta < MIN_WIN_DELTA) || (delta > MAX_WIN_DELTA) )
    {
        if( can_seek )
            res = SeekSetTime(ffv, time);
    }
    else if( delta <= 0 )
        res = true; // считаем что не поздно
    else // if( delta < MAX_WIN_DELTA )
    {
        // допустимая разница, доводим
        while( cur_pts < time )
            DoDecode(ffv);
        res = true;
    }

    if( res )
    {
        // * перевод в RGB
        Point sz(ffv.vidSz);
        // не допускаем смены разрешения
        ASSERT_RTL( VideoSize(GetVideoCtx(ffv)) == sz );
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

static bool SeekSetTime(FFViewer& ffv, double time)
{
    AVFormatContext* ic = ffv.iCtx;
    double& cur_pts     = ffv.curPTS;

    // * обнуляем
    cur_pts = INV_TS;

    // * перемещение
    int flags = AVSEEK_FLAG_BACKWARD; // чтоб раньше времени пришли
    ASSERT( ic->start_time != (int64_t)AV_NOPTS_VALUE );
    int64_t start_ts = AV_TIME_BASE * time + ic->start_time;
    // вполне подойдет поиск по умолчальному потоку (все равно видео выберут)
    int av_res = av_seek_frame(ic, -1, start_ts, flags);
    ASSERT_RTL( !av_res );

    // * до первого кадра с PTS
    while( !IsTSValid(cur_pts) )
        DoDecode(ffv);

    return DecodeTill(ffv, time, false);
}

// время без смещения
bool SetTime(FFViewer& ffv, double time)
{
    ASSERT( ffv.IsOpened() );
    AVFormatContext* ic = ffv.iCtx;

    if( (time < 0) || (time > Duration(ic)) )
        return false;
    time += StartTime(ic);

    bool res = false;
    if( !IsTSValid(ffv.curPTS) )
        res = SeekSetTime(ffv, time);
    else
        res = DecodeTill(ffv, time, true);
    return res;
}

RefPtr<Gdk::Pixbuf> GetRawFrame(double time, FFViewer& ffv)
{
    RefPtr<Gdk::Pixbuf> res_pix;
    if( ffv.IsOpened() && SetTime(ffv, time) )
    {
        Point sz();
        res_pix = CreateFromData(ffv.rgbFrame.data[0], ffv.vidSz, false);
    }
    return res_pix;
}

