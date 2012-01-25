# bombono-dvd.spec
# spec file for Bombono DVD
#
# Copyright (c) 2007-2009 Ilya Murav'jov <muravev@yandex.ru>
#
%global         rel_tag .20120125git3f4adbb

Name:           bombono-dvd
Version:        1.2.0%{?rel_tag}
Release:        1%{?dist}
Summary:        DVD authoring program with nice and clean GUI
License:        GPLv2 and GPLv2+ and Boost and Python and LGPLv2+
                # License breakdown in README.
Group:          Applications/Productivity
Url:            http://www.bombono.org
# git clone https://git.gitorious.org/bombono-dvd/bombono-dvd.git bombono-dvd
# tag=.20120125git3f4adbb; cd bombono-dvd;  git reset --hard ${tag##*git}; cd ..
# tar czf bombono-dvd-1.2.0.20120125git3f4adbb.tar.gz --exclude .git bombono-dvd
Source:         bombono-dvd-%{version}.tar.gz

BuildRequires:  boost-devel
BuildRequires:  desktop-file-utils
BuildRequires:  ffmpeg-devel
BuildRequires:  gettext
BuildRequires:  gtkmm24-devel
BuildRequires:  libdvdread-devel
BuildRequires:  libstdc++-devel
BuildRequires:  libxml++-devel
BuildRequires:  mjpegtools-devel
BuildRequires:  pkgconfig
BuildRequires:  scons
Requires:       dvd+rw-tools
Requires:       dvdauthor
Requires:       enca
Requires:       ffmpeg
Requires:       mjpegtools
#Suggests:      totem, gvfs, scons, twolame

Requires(posttrans):    gtk2
Requires(post):         desktop-file-utils
Requires(postun):       desktop-file-utils gtk2

%global  boost_flags \\\
    -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_FILESYSTEM_VERSION=2
%global warn_flags  \
    -Wno-reorder -Wno-unused-variable
%global  scons       \
    scons  %{?jobs:-j%{jobs}}                              \\\
    BUILD_CFG=debug                                        \\\
    BUILD_BRIEF=false                                      \\\
    BUILD_QUICK=false                                      \\\
    CC="%__cc"                                             \\\
    CXX="%__cxx"                                           \\\
    CFLAGS="%{optflags}"                                   \\\
    CXXFLAGS="%{optflags} %{warn_flags} %{boost_flags}"    \\\
    PREFIX=%{_prefix}                                      \\\
    TEST=false                                             \\\
    TEST_BUILD=false                                       \\\
    USE_EXT_BOOST=true

%description
Bombono DVD is an easy to use program for making DVD-Video. The main features
are an excellent MPEG viewer with time line, a real WYSIWYG menu editor with
live thumbnails and monitor, and comfortable Drag-N-Drop support. Authoring
to folder, making an ISO-image or burning directly to DVD as well as
re-authoring by importing video from DVD discs is also supported.

%prep
%setup -q -n bombono-dvd
sed -i '\;#![ ]*/usr/bin/env;d'  $(find . -name SCons\*)
rm -r debian libs/boost-lib src/mlib/tests libs/mpeg2dec

%build
%scons build

%install
rm config.opts
%scons DESTDIR=%{buildroot} install
rm -rf docs/man docs/TechTasks docs/Atom.planner
%find_lang %{name}
desktop-file-validate \
    %{buildroot}%{_datadir}/applications/bombono-dvd.desktop

%postun
/usr/bin/update-desktop-database &> /dev/null || :
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%post
/usr/bin/update-desktop-database  &> /dev/null || :
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files -f  bombono-dvd.lang
%doc README COPYING docs
%{_bindir}/bombono-dvd
%{_bindir}/mpeg2demux
%{_datadir}/bombono
%{_datadir}/applications/bombono-dvd.desktop
%{_datadir}/pixmaps/bombono-dvd.png
%{_datadir}/icons/hicolor/*/apps/bombono-dvd.png
%{_datadir}/mime/packages/bombono.xml
%{_mandir}/man1/bombono-dvd.*
%{_mandir}/man1/mpeg2demux.*

%changelog
* Wed Jan 25 2012 Alec Leamas <alec@nowhere.com> 1.2.0.20120125git3f4adbb-1
- Removing irrelevant files in docs/
- Updating deps to reflect bb7f789 "twolame is optional..."
- Removing bundled libmpeg2
* Sat Jan 21 2012 Alec Leamas <alec@nowhere.com>   1.2.0.20101210git2840c3a-1
- Updating to latest git source. Many patches accepted.
- Removing %%defattr.
* Sat Jan 14 2012 Alec Leamas <alec@nowhere.com>             1.2.0-4
- Refactoring scons parameters to rpm macro %%scons.
- Adding virtual provide for bundled mpeg lib.
- Use of Boost license, was BSD, handling license file.
* Sun Jan 8 2012 Alec Leamas <alec@nowhere.com>              1.2.0-3
- Using external boost lib, removing bundled one.
- Feeding standard rpm build flags to scons.
- Cleaning up post/posttrans snippets and deps.
- Restoring execute permissions on some scripts in %%prep
* Fri Jan 6 2012 Alec Leamas <alec@nowhere.com>              1.2.0-2
- Using manpages from debian dir, remove debian stuff.
* Tue Jan 4 2012 Alec Leamas <alec@nowhere.com>              1.2.0-1
- First attempt to modify Ilya Murav'jo's spec file for Fedora.
- Adding changelog
- Added find_lang handling of .mo files.
- Adjusted source URL, Fedora wants a complete public one.
- Adjusted License: tag to reflect README.
- Removed things not used and/or deprecated in Fedora.
- Added scriptlets handling icons and mime types.
- Added %%doc
- Modified arguments to scons build and install.
- Fixed various rpmlint warnings
- Adjusted dependencies after mock build test.
* Mon Jan 1 2007  Ilya Murav'jov <muravev@yandex.ru>         1.0.2-0
- Faked entry by Alec leamas to reflect initial packager.

# vim: set expandtab ts=4 sw=4:
