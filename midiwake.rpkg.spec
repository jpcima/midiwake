Name:       {{{ git_dir_name }}}

Version:    {{{ git_dir_version lead="0.0" follow="0" }}}

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
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-linguist

%description
A utility to block the screen saver during MIDI activity

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
%{_bindir}/midiwake
%{_datadir}/applications/midiwake.desktop
%{_datadir}/pixmaps/midiwake.png

%changelog
{{{ git_dir_changelog }}}
