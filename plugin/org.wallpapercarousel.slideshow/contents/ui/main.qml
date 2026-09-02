/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.wallpapers.image as Wallpaper
import org.kde.plasma.plasmoid

WallpaperItem {
    id: root

    // Screen attached property only resolves on an Item, so bind it here rather
    // than reaching for it from inside Connections.
    readonly property string screenName: Screen.name

    // used by WallpaperInterface for drag and drop
    onOpenUrlRequested: (url) => {
        if (root.pluginName === "org.kde.image") {
            const result = imageWallpaper.addUsersWallpaper(url);
            if (result.length > 0) {
                // Can be a file or a folder (KPackage)
                root.configuration.Image = result;
            }
        } else {
            imageWallpaper.addSlidePath(url);
            // Save drag and drop result
            root.configuration.SlidePaths = imageWallpaper.slidePaths;
        }
        root.configuration.writeConfig();
    }

    contextualActions: root.pluginName === "org.wallpapercarousel.slideshow" ? [openWallpaperAction, imageWallpaper.nextSlideAction] : []

    PlasmaCore.Action {
        id: openWallpaperAction
        text: i18nd("plasma_wallpaper_org.kde.image", "Open Wallpaper Image")
        icon.name: "document-open"
        onTriggered: imageView.mediaProxy.openModelImage();
    }

    Connections {
		enabled: root.pluginName === "org.wallpapercarousel.slideshow"
        target: Qt.application
        function onAboutToQuit() {
            root.configuration.writeConfig(); // Save the last position
        }
    }

    Component.onCompleted: {
        // In case plasmashell crashes when the config dialog is opened
        root.configuration.PreviewImage = "null";
        root.loading = true; // delays ksplash until the wallpaper has been loaded
    }

    // Live endpoint: the backend advances slides without touching the "Image"
    // config key, so persist the actually-shown slide here for external tools.
    // CurrentScreen goes with it — appletsrc has no other key that identifies
    // which monitor a containment paints, so multi-screen readers need this.
    Connections {
        target: imageView
        function onModelImageChanged() {
            if (root.pluginName !== "org.wallpapercarousel.slideshow") {
                return;
            }
            let dirty = false;
            if (root.configuration.CurrentImage !== imageView.modelImage) {
                root.configuration.CurrentImage = imageView.modelImage;
                dirty = true;
            }
            if (root.configuration.CurrentScreen !== root.screenName) {
                root.configuration.CurrentScreen = root.screenName;
                dirty = true;
            }
            if (dirty) {
                root.configuration.writeConfig();
            }
        }
    }

    ImageStackView {
        id: imageView
        anchors.fill: parent

        fillMode: root.configuration.FillMode
        configColor: root.configuration.Color
        blur: root.configuration.Blur
        forceImageAnimation: root.configuration.ForceImageAnimation

        source: {
            if (root.pluginName === "org.wallpapercarousel.slideshow") {
                // Carousel: the config key is the source of truth so that
                // Wallpaper Carousel can jump to a specific image by writing it.
                // The backend keeps advancing and writes this key on every slide.
                return root.configuration.Image;
            }
            if (root.configuration.PreviewImage !== "null") {
                return root.configuration.PreviewImage;
            }
            return root.configuration.Image;
        }
        sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)
        wallpaperInterface: root

        Wallpaper.ImageBackend {
            id: imageWallpaper

            // Not using root.configuration.Image to avoid binding loop warnings
            configMap: root.configuration
            usedInConfig: false
            //the oneliner of difference between image and slideshow wallpapers
            renderingMode: (root.pluginName === "org.kde.image") ? Wallpaper.ImageBackend.SingleImage : Wallpaper.ImageBackend.SlideShow
            dynamicMode: root.configuration.DynamicMode
            targetSize: imageView.sourceSize
            slidePaths: root.configuration.SlidePaths
            slideTimer: root.configuration.SlideInterval
            slideshowMode: root.configuration.SlideshowMode
            slideshowFoldersFirst: root.configuration.SlideshowFoldersFirst
            uncheckedSlides: root.configuration.UncheckedSlides

            // Invoked from C++
            function writeImageConfig(newImage: string) {
                configMap.Image = newImage;
            }
        }
    }

    Component.onDestruction: {
        if (root.pluginName === "org.wallpapercarousel.slideshow") {
            root.configuration.writeConfig(); // Save the last position
        }
    }
}
