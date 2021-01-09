project('mkspr', 'c')

    mkspr_deps = []
    libpng = dependency('libpng', required: true)
    sdl2 = dependency('sdl2', required: true)
    sdl2_image = dependency('SDL2_image', required: true)
    mkspr_deps += [libpng, sdl2, sdl2_image]

    mkspr_bin = executable(
            'mkspr',
            'main.c',
            dependencies: mkspr_deps
            )
