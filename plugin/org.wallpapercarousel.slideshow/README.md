# Carousel Slideshow

A Plasma 6 slideshow wallpaper that another program can steer.

It is a fork of KDE's own `org.kde.slideshow` from plasma-workspace, so everything you
already know still applies: same folder list, ordering modes, fill modes, blur and
solid backgrounds, dynamic day/night images, same *Next Image* desktop action.

What the fork adds:

- Writing the `Image` config key jumps the slideshow to that picture mid-rotation, and
  the rotation timer carries on from there — no restart, no losing your place.
- It records the slide currently on screen (`CurrentImage`) and which monitor it is on
  (`CurrentScreen`), so an external tool can read back what you are looking at instead
  of guessing.

Those two keys are what [Wallpaper Carousel](https://github.com/DeadIndian/Wallpaper-Carousel)
drives. Install the app for the intended experience: a keyboard-driven carousel of
thumbnails, bound to a shortcut, that sets any image on any monitor and animates the
reveal. Without it this wallpaper is simply the stock slideshow.

## Install

Pure QML — nothing to compile, no root needed.

```bash
kpackagetool6 --type Plasma/Wallpaper --install org.wallpapercarousel.slideshow-0.1.0.tar.gz
```

Then right click the desktop → *Configure Desktop and Wallpaper* → *Wallpaper type* →
**Carousel Slideshow**.

## License

GPL-2.0-or-later, inherited from the KDE code this is derived from. See [`LICENSE`](LICENSE).
The rest of the Wallpaper Carousel repository is MIT; this directory is not.
