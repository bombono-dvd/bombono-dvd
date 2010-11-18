
#include <mdemux/tests/_pc_.h>

#include <mdemux/demuxconst.h> // IsTSValid()

#include <mlib/tech.h>
#include <mlib/geom2d.h>

#include <mlib/stream.h>

C_LINKAGE_BEGIN
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
C_LINKAGE_END

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

double TS2Time(int64_t ts, AVFormatContext* ic, int stream_idx)
{
    double tm = INV_TS;
    if( ts != (int64_t)AV_NOPTS_VALUE )
        tm = ts * av_q2d(ic->streams[stream_idx]->time_base);
    return tm;
}

BOOST_AUTO_TEST_CASE( TestFFmpegOpen )
{
    const char* fname = "/home/ilya/opt/programming/atom-project/Autumn.mpg";
    //const char* fname = "/opt/src/test-mpegs/MonaLisa_02_6-c3.vob";

    av_register_all();

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
    BOOST_CHECK( !res );

    int err = av_find_stream_info(ic);
    //if (err < 0) {
    //    fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
    //    ret = -1;
    //    goto fail;
    //}
    BOOST_CHECK( err >= 0 );
    
    DumpIFile(ic);
    ASSERT_RTL( ic->duration   != (int64_t)AV_NOPTS_VALUE );
    // начальное время не сущ. в редких случаях - оно устанавливается
    // явно (avi) либо через минимум по элементарным потокам (mpeg-подобные)
    ASSERT_RTL( ic->start_time != (int64_t)AV_NOPTS_VALUE );

    // в секундах
    double dur = ic->duration / (double)AV_TIME_BASE;
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

    Point sz(VideoSize(dec));
    ASSERT_RTL( sz.x );
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
    SwsContext* rgb_convert_ctx = sws_getContext(sz.x, sz.y, dec->pix_fmt, sz.x, sz.y, 
        dst_pf, sws_flags, 0, 0, 0);
    ASSERT( rgb_convert_ctx );

    Point dst_sz(sz);
    AVFrame rgb_frame;
    avcodec_get_frame_defaults(&rgb_frame); // не помешает
    uint8_t* rgb_buf = (uint8_t*)av_malloc(avpicture_get_size(dst_pf, dst_sz.x, dst_sz.y) * sizeof(uint8_t));
    avpicture_fill((AVPicture*)&rgb_frame, rgb_buf, dst_pf, dst_sz.x, dst_sz.y);

    // время текущего кадра
    double cur_pts = INV_TS;

    // перемещение
    double start = 5.9; // секунд
    int flags = AVSEEK_FLAG_BACKWARD; // чтоб раньше времени пришли
    ASSERT( ic->start_time != (int64_t)AV_NOPTS_VALUE );
    int64_t start_ts = AV_TIME_BASE * start + ic->start_time;
    // вполне подойдет поиск по умолчальному потоку (все равно видео выберут)
    res = av_seek_frame(ic, -1, start_ts, flags);
    ASSERT_RTL( !res );

    //AVFrame* frame = avcodec_alloc_frame();
    while( true )
    {
        AVFrame picture;
        AVPacket pkt;
        int got_picture;

        // предположительное время начала следующего кадра (если не будет явно 
        // установлено) - расчет до следующего av_read_frame()
        double next_pts = cur_pts;
        if( IsTSValid(next_pts) )
        {
            // судя по всему, код ffplay(.c) устарел - длительность кадра вычисляется по 
            // frame->repeat_pict, который всегда ноль;
            // а вот ffmpeg делает хитрее, через "parser" (аналог моего Mpeg::Decoder'а),-
            // тот repeat_pict заполняет правильно, и с учетом dec->time_base
            // (1 + 1|2|3|5] = 2|3|4|6 полутактов для MPEG2)
            AVStream* st = ic->streams[video_idx];
            int ticks    = st->parser ? st->parser->repeat_pict + 1 : dec->ticks_per_frame ;
            next_pts    += av_q2d(dec->time_base) * ticks;
        }

        res = av_read_frame(ic, &pkt);
        ASSERT( res >= 0 );

        // только одно видео фильтруем
        ASSERT_RTL( pkt.stream_index == video_idx );

        dec->reordered_opaque = pkt.pts;

        avcodec_get_frame_defaults(&picture);
        res = avcodec_decode_video(dec, &picture, &got_picture, pkt.data, pkt.size);
        ASSERT_RTL( res >= 0 );

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
            cur_pts = TS2Time(ff_pts, ic, video_idx);
            if( !IsTSValid(cur_pts) )
                cur_pts = next_pts;

            // * перевод в RGB
            // не допускаем смены разрешения
            ASSERT_RTL( VideoSize(dec) == sz );
            // не очень понятно как пользовать аргументы 4, 5
            sws_scale(rgb_convert_ctx, picture.data, picture.linesize,
                      0,  sz.y, rgb_frame.data, rgb_frame.linesize);
            uint8_t* buf = rgb_frame.data[0];
            uint8_t* ptr = buf;
            uint8_t tmp;
            for( int y=0; y<dst_sz.y; y++ )
                for( int x=0; x<dst_sz.x; x++, ptr += 3 )
                {
                    // b <-> r
                    tmp = ptr[0];
                    ptr[0] = ptr[2];
                    ptr[2] = tmp;
                }

            io::cout << "real pts: " << cur_pts << "; pts: " << TS2Time(pkt.pts, ic, video_idx)
                     << "; dts: " << TS2Time(pkt.dts, ic, video_idx) << io::endl;
            if( IsTSValid(cur_pts) )
                ASSERT( fabs(cur_pts - TS2Time(pkt.dts, ic, video_idx)) < 0.001 );

            void ShowRGB24(int width, int height, uint8_t* buf);
            ShowRGB24(dst_sz.x, dst_sz.y, buf);

            // :TEMP:
            //break;
            static int idx = 0;
            if( idx++ >= 5 )
                break;
        }
    }
    //av_free(frame);

    av_free(rgb_buf);
    //av_free(rgb_frame);
    sws_freeContext(rgb_convert_ctx);

    // контекст кодека закрывается отдельно
    avcodec_close(dec);

    // судя по тому как, например, поле ctx_flags нигде не обнуляется
    // (кроме как при инициализации), то повторно использовать структуру
    // не принято -> все заново создаем при переоткрытии
    av_close_input_file(ic);
}

