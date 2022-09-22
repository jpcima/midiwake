Name:       {{{ git_dir_name }}}

Version:    {{{package_version}}}{{{git_version_appendix}}}

Release:    1%{?dist}

Summary:    A utility to block the screen saver during MIDI activity

License:    GPLv2+

URL:        https://github.com/jpcima/midiwake

VCS:        {{{ git_dir_vcs }}}

Source0:    {{{ git_dir_pack }}}
Source1:    {{{ git_pack dir_name=QCtrlSignals    path=thirdparty/QCtrlSignals }}}
Source2:    {{{ git_pack dir_name=QSingleInstance path=thirdparty/QSingleInstance }}}

BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: alsa-lib-devel
%if 0%{?suse_version}
BuildRequires: libQt5Widgets-devel
BuildRequires: libQt5Network-devel
BuildRequires: libQt5DBus-devel
BuildRequires: libqt5-linguist-devel
%else
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-linguist
%endif

%description
This program shows up in the notification area, and it listens for output
produced by hardware MIDI devices, such as a synthesizer keyboard.

While the device is being played with, the program prevents the desktop from
entering idle mode, which might lock the session or activate the screen saver.
For instance, one can practice a piece of music on screen without risks of
interruption.

%prep
{{{ git_dir_setup_macro }}}
tar -C thirdparty -xf %{SOURCE1}
tar -C thirdparty -xf %{SOURCE2}

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%doc README.md
%license LICENSE
%{_bindir}/midiwake
%{_datadir}/applications/midiwake.desktop
%{_datadir}/pixmaps/midiwake.png

%changelog
{{{ git_dir_changelog }}}
