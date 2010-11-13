
#include <mdemux/tests/_pc_.h>

#include <mlib/tech.h>
#include <mlib/stream.h>

C_LINKAGE_BEGIN
#include <libavformat/avformat.h>
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

BOOST_AUTO_TEST_CASE( TestFFmpegOpen )
{
    const char* fname = "/home/ilya/opt/programming/atom-project/Autumn.mpg";

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
    // :TODO!!!: обработка ошибок открытия файла
    BOOST_CHECK( !res );

    int err = av_find_stream_info(ic);
    //if (err < 0) {
    //    fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
    //    ret = -1;
    //    goto fail;
    //}
    BOOST_CHECK( err >= 0 );
    
    DumpIFile(ic);
    int64_t duration = ic->duration;
    // :TODO!!!:
    ASSERT( duration != (int64_t)AV_NOPTS_VALUE );
    // в секундах
    double dur = duration / (double)AV_TIME_BASE;
    io::cout << "duration: " << dur << io::endl;

    // :TODO!!!:
    //avcodec_close(ist->st->codec);

    av_close_input_file(ic);
}

