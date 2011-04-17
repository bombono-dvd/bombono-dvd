# bombono-dvd.spec
# spec file for Bombono DVD
#
# Copyright (c) 2007-2009 Ilya Murav'jov <muravev@yandex.ru>
#
# This spec is done to be used for SUSE. 
# So, maybe adjustments required for other RPM-based distros. 

Name:		bombono-dvd
Version:	1.0.2
Summary:	DVD authoring program with nice and clean GUI
License:	GPL
Release:	0
Group:		Productivity/Multimedia/Video/Editors and Convertors
URL:		http://www.bombono.org
Packager:	Ilya Murav'jov <muravev@yandex.ru>
Source:		%{name}-%{version}.tar.bz2
BuildRequires:	scons libdvdread-devel gtkmm2-devel libxml++-devel libffmpeg-devel
Requires:       dvdauthor mjpegtools dvd+rw-tools scons toolame ffmpeg enca
BuildRoot:	%{_tmppath}/%{name}-%{version}-build

# :TODO:
# - split flags according to conditions (RPM_OPT_FLAGS?)
# - parralel build - -j NUM
%define BUILD_FLAGS TEST=1
%define clean_build_root rm -fr $RPM_BUILD_ROOT

%description
 Bombono DVD is easy to use program for making DVD-Video.
 The main features of Bombono DVD are:
  * Excellent MPEG viewer: Timeline and Monitor
  * Real WYSIWYG Menu Editor with live thumbnails
  * Comfortable Drag-N-Drop support
  * You can author to folder, make ISO-image or burn directly to DVD
  * Reauthoring: you can import video from DVD discs.

%prep
%setup -q	

%build
scons -k %{BUILD_FLAGS} PREFIX=%{_prefix} DESTDIR=$RPM_BUILD_ROOT

%install
%clean_build_root
scons install

%clean
%clean_build_root

%files
%defattr(-,root,root)
%{_bindir}/*
%{_datadir}/bombono
%{_datadir}/applications/bombono-dvd.desktop
%{_datadir}/pixmaps/bombono-dvd.png
