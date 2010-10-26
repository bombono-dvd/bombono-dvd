
#include <mgui/tests/_pc_.h>

#include "test_author.h"
#include "test_mbrowser.h"

#include <mgui/sdk/player_utils.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/menu.h>

#include <mgui/editor/toolbar.h>
#include <mgui/execution.h>
#include <mgui/dialog.h>
#include <mgui/init.h>

#include <mbase/obj_bind.h> // MediaItem2String()

#include <mlib/format.h>
#include <mlib/sdk/stream_util.h>
#include <mlib/sdk/system.h>
#include <mlib/gettext.h>

static void WriteAsPPM(int fd, RefPtr<Gdk::Pixbuf> pix)
{
    int wdh = pix->get_width();
    int hgt = pix->get_height();
    int stride     = pix->get_rowstride();
    int channels   = pix->get_n_channels();
    guint8* pixels = pix->get_pixels();

    char tmp[20];
    snprintf(tmp, ARR_SIZE(tmp), "P6\n%d %d\n255\n", wdh, hgt);
    checked_writeall(fd, tmp, strlen(tmp));

    for( int row = 0; row < hgt; row++, pixels += stride )
    {
        guint8* p = pixels;
        for( int col = 0; col < wdh; col++, p += channels )
        {
            tmp[0] = p[0];
            tmp[1] = p[1];
            tmp[2] = p[2];

            checked_writeall(fd, tmp, 3);
        }
    }
}

static std::string FFmpegPostArgs(const std::string& out_fname, bool is_4_3, bool is_pal, 
                                  const std::string& a_fname = std::string(), double a_shift = 0.)
{
    std::string a_input = "-an"; // без аудио
    if( !a_fname.empty() )
    {
        std::string shift; // без смещения
        if( a_shift )
            shift = boost::format("-ss %.2f ") % a_shift % bf::stop;
        a_input = boost::format("%2%-i \"%1%\"") % a_fname % shift % bf::stop; 
    }

    const char* target = "pal";
    int wdh = 720; // меню всегда полноразмерное
    int hgt = 576;
    if( !is_pal )
    {
        target = "ntsc";
        hgt = 480;
    }

    return boost::format("%1% -target %5%-dvd -aspect %2% -s %3%x%4% -y \"%6%\"")
        % a_input % (is_4_3 ? "4:3" : "16:9") % wdh % hgt % target % out_fname % bf::stop;
}

BOOST_AUTO_TEST_CASE( TestRenderTranscoding )
{
    return;
    std::string fname   = "/home/ilya/opt/programming/atom-project/Autumn.mpg";
    std::string a_fname = "/home/ilya/opt/programming/atom-project/transition.mpg";

    Mpeg::FwdPlayer plyr;
    RGBOpen(plyr, fname);

    bool is_pal = plyr.MInfo().vidSeq.hgt == 576;

    int fps = 30;
    double shift = 1. / fps;
    double end  = 1.; // секунда

    // 1. Ставим частоту fps впереди -i pipe, чтобы она воспринималась для входного потока
    // (а для выходного сработает -target)
    // 2. И наоборот, для выходные параметры (-aspect, ...) ставим после всех входных файлов
    std::string ffmpeg_cmd = boost::format("ffmpeg -r %1% -f image2pipe -vcodec ppm -i pipe: %2%")
        % fps % FFmpegPostArgs("../dvd_out/trans.vob", false, is_pal, a_fname, 6) % bf::stop;

    int in_fd = NO_HNDL;
    GPid pid = Spawn(0, ffmpeg_cmd.c_str(), 0, false, &in_fd);
    ASSERT( in_fd != NO_HNDL );

    double all_time = GetClockTime();

    int i = 0;
    for( double cur = 0; cur < end; cur += shift, i++ )
    {
        RefPtr<Gdk::Pixbuf> img = GetRawFrame(cur, plyr);
        ASSERT( img ); 

        //std::string fpath = "/dev/null"; //boost::format("../dvd_out/ppms/%1%.ppm") % i % bf::stop;
        //int fd = OpenFileAsArg(fpath.c_str(), false);
        WriteAsPPM(in_fd, img);
        //close(fd);
    }
    close(in_fd);
    // варианты остановить транскодирование сразу после окончания видео, см. ffmpeg.c:av_encode():
    // - через сигнал (SIGINT, SIGTERM, SIGQUIT)
    // - по достижению размера или времени (см. соответ. опции вроде -t); но тут возможна проблема
    //   блокировки/закрытия раньше, чем мы закончим посылку всех данных; да и время высчитывать тоже придется
    // - в момент окончания первого из источников, -shortest; не подходит, если аудио существенно длинней
    //   чем мы хотим записать
    Execution::Stop(pid);
    g_spawn_close_pid(pid);

    all_time = GetClockTime() - all_time;
    io::cout << "Time to run: " << all_time << io::endl;
    io::cout << "FPS: " << i / all_time << io::endl;
}

BOOST_AUTO_TEST_CASE( TestStillTranscoding )
{
    return;
    std::string fname   = "/home/ilya/opt/programming/atom-project/dvd_out/3.Menu 4/Menu.png";

    // в случае автономной работы ffmpeg указываем длительность аргументом
    double duration = 15;
    std::string ffmpeg_cmd = boost::format("ffmpeg -t %3$.2f -loop_input -i \"%1%\" %2%") 
        % fname % FFmpegPostArgs("../dvd_out/trans.vob", false, true) % duration % bf::stop;
 
    io::cout << ffmpeg_cmd << io::endl;
    //Execution::SimpleSpawn(ffmpeg_cmd.c_str());
    ExitData ed = System(ffmpeg_cmd);
    BOOST_CHECK( ed.IsGood() );
}

namespace Project {

BOOST_AUTO_TEST_CASE( TestMenuSettings )
{
    return;
    GtkmmDBInit gd_init;
    RefPtr<MenuStore> ms = InitAndLoadPrj(TestProjectPath()).mnStore;
    ASSERT( Size(ms) );

    Menu mn = GetMenu(ms, ms->children().begin());
    void MenuSettings(Menu mn, Gtk::Window* win);
    MenuSettings(mn, 0);
}

BOOST_AUTO_TEST_CASE( TestCheckFFmpeg )
{
    std::string conts = GetTestFNameContents("ffmpeg05_valid_dvd_encoders.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/hardy_formats_.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/lucid_formats.txt");

    void TestFFmpegForDVDEncoding(const std::string& conts);
    TestFFmpegForDVDEncoding(conts);
}

} // namespace Project


