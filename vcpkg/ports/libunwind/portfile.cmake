vcpkg_download_distfile(
    AARCH64_ASM_PATCH
    URLS https://github.com/libunwind/libunwind/commit/6ae71b3ea71bff0f38c7a6a05beda30b7dce1ef6.patch
    SHA512 8d183ca661227211871e75c26dccc3393a12cd8b38e55efe85f34551a0bc030640b8d2796900684bd5408e7f87a3640258d761c1ac3e5cecc9c967ac7a678e7b
    FILENAME 6ae71b3ea71bff0f38c7a6a05beda30b7dce1ef6.patch
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO "libunwind/libunwind"
    REF "v${VERSION}"
    HEAD_REF master
    SHA512 dd8332b7a2cbabb4716c01feea422f83b4a7020c1bee20551de139c3285ea0e0ceadfa4171c6f5187448c8ddc53e0ec4728697d0a985ee0c3ff4835b94f6af6f
    PATCHES
        "${AARCH64_ASM_PATCH}"
)

vcpkg_configure_make(
    SOURCE_PATH "${SOURCE_PATH}"
    AUTOCONFIG
    OPTIONS
        --disable-tests
)
vcpkg_install_make()
vcpkg_fixup_pkgconfig()


file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
