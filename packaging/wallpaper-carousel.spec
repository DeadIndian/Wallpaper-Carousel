# RPM spec for Wallpaper Carousel (Fedora / COPR).
#
# Ships two things: the compiled wallpaper-carousel picker, and the pure-QML
# Carousel Slideshow wallpaper plugin it drives. Both come from one CMake install.
#
# Local test:   rpmlint packaging/wallpaper-carousel.spec
#               spectool -g -R packaging/wallpaper-carousel.spec
#               rpmbuild -ba packaging/wallpaper-carousel.spec
# Honest test:  mock -r fedora-rawhide-x86_64 rebuild <srpm>
Name:           wallpaper-carousel
Version:        0.1.0
Release:        1%{?dist}
Summary:        Pick a wallpaper per screen from a carousel of thumbnails

# AppStream wants a reverse-DNS component id, and the metainfo file name must match it.
# The .desktop keeps the short name because kglobalshortcutsrc resolves the Meta+W
# service shortcut by that basename.
%global appstream_id io.github.deadindian.WallpaperCarousel

# The app (src/, qml/) is MIT. The bundled Carousel Slideshow wallpaper is a fork of
# KDE's own slideshow wallpaper and stays GPL-2.0-or-later; its license text ships
# inside %%{_datadir}/plasma/wallpapers/org.wallpapercarousel.slideshow/.
License:        MIT AND GPL-2.0-or-later
URL:            https://github.com/DeadIndian/Wallpaper-Carousel
Source0:        %{url}/archive/refs/tags/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Quick)
BuildRequires:  cmake(Qt6DBus)
BuildRequires:  cmake(Qt6Multimedia)
BuildRequires:  cmake(Qt6Widgets)
# Without this, CMake falls back to FetchContent and tries to clone tomlplusplus at
# configure time. Mock and COPR build without network, so that fails there and only
# there.
BuildRequires:  tomlplusplus-devel
BuildRequires:  desktop-file-utils
# appstreamcli, for the %%check below. --no-net keeps it offline like mock/COPR are.
BuildRequires:  appstream

# plasmashell is how wallpapers are actually applied, and it owns the
# org.kde.plasma.wallpapers.image QML backend the wallpaper plugin renders with.
Requires:       plasma-workspace
# KWin scripting places the picker on the right output and minimizes everything else.
Requires:       kwin
# QML imports are invisible to RPM's automatic dependency generator, so every module
# the shipped QML imports has to be named by hand or the UI half-loads at runtime.
Requires:       qt6-qtdeclarative
Requires:       qt6-qtmultimedia
Requires:       qt6-qt5compat
Requires:       kf6-kirigami
Requires:       kf6-kdeclarative
Requires:       kf6-kcmutils
Requires:       kf6-knewstuff
Requires:       kf6-kitemmodels
Requires:       kf6-kconfig
Requires:       kf6-kwindowsystem

%description
A bottom-centered carousel of the images in your wallpaper folder. Arrow keys move
through it, Enter applies the highlighted image to the current screen, and Tab hops
the picker to the next monitor for when the compositor guesses wrong after a hotplug.

Wallpapers are applied through plasmashell, so the pick becomes the real desktop
wallpaper rather than a separate layer.

Also installs the Carousel Slideshow wallpaper plugin: select it as your wallpaper
type and the picker can jump the slideshow straight to a chosen image without
stopping the rotation.

%prep
%autosetup -n Wallpaper-Carousel-%{version}

%build
%cmake
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
appstreamcli validate --no-net \
    %{buildroot}%{_metainfodir}/%{appstream_id}.metainfo.xml
# The project's own assert-based self-checks. They print "ok" and exit 0.
%{__cmake_builddir}/wc-tests
%{__cmake_builddir}/wc-tests-screenorder

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_metainfodir}/%{appstream_id}.metainfo.xml
%{_datadir}/plasma/wallpapers/org.wallpapercarousel.slideshow/

%changelog
* Wed Sep 02 2026 DeadIndian <gollabharath2007@gmail.com> - 0.1.0-1
- First packaged release: per-screen picker, Tab screen-hop, reveal animation,
  and the Carousel Slideshow wallpaper plugin.
