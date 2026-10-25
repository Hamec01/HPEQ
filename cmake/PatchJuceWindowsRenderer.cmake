function(hpeq_patch_juce_windows_renderer juce_root)
    if (NOT WIN32)
        return()
    endif()

    set(windowing "${juce_root}/modules/juce_gui_basics/native/juce_Windowing_windows.cpp")
    file(READ "${windowing}" windowing_source)
    if (NOT windowing_source MATCHES "HPEQ_JUCE_DEFAULT_RENDERING_ENGINE")
        set(windowing_anchor "ComponentPeer* Component::createNewPeer (int styleFlags, void* parentHWND)")
        string(FIND "${windowing_source}" "${windowing_anchor}" windowing_anchor_at)
        if (windowing_anchor_at EQUAL -1)
            message(FATAL_ERROR "JUCE Windows peer source changed; cannot install the HPEQ renderer switch")
        endif()
        string(REPLACE "${windowing_anchor}"
            "#ifndef HPEQ_JUCE_DEFAULT_RENDERING_ENGINE\n #define HPEQ_JUCE_DEFAULT_RENDERING_ENGINE 1\n#endif\n\n${windowing_anchor}"
            windowing_source "${windowing_source}")
        string(REPLACE
            "return new HWNDComponentPeer { *this, styleFlags, (HWND) parentHWND, false, 1 };"
            "return new HWNDComponentPeer { *this, styleFlags, (HWND) parentHWND, false, HPEQ_JUCE_DEFAULT_RENDERING_ENGINE };"
            windowing_source "${windowing_source}")
        file(WRITE "${windowing}" "${windowing_source}")
    endif()

    set(image "${juce_root}/modules/juce_graphics/images/juce_Image.cpp")
    file(READ "${image}" image_source)
    if (NOT image_source MATCHES "HPEQ_JUCE_DEFAULT_IMAGE_TYPE")
        set(image_anchor "Image::Image (PixelFormat format, int width, int height, bool clearImage)")
        string(FIND "${image_source}" "${image_anchor}" image_anchor_at)
        if (image_anchor_at EQUAL -1)
            message(FATAL_ERROR "JUCE Image source changed; cannot install the HPEQ image-type switch")
        endif()
        string(REPLACE "${image_anchor}"
            "#ifndef HPEQ_JUCE_DEFAULT_IMAGE_TYPE\n #define HPEQ_JUCE_DEFAULT_IMAGE_TYPE NativeImageType\n#endif\n\n${image_anchor}"
            image_source "${image_source}")
        string(REPLACE
            ": image (NativeImageType().create (format, width, height, clearImage))"
            ": image (HPEQ_JUCE_DEFAULT_IMAGE_TYPE().create (format, width, height, clearImage))"
            image_source "${image_source}")
        file(WRITE "${image}" "${image_source}")
    endif()
endfunction()
