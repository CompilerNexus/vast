vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO lifting-bits/gap
  REF 107767b580fd8440a5bef5654c64174e6b5ecd95
  SHA512 8b6b9a8e5fa5d740d3de40adf34ed678334a72b2bc83c698f6a22db75a131523b15c2d05b1946a898eb755e8f2c1359d706e17da1235d1bc5a0d0465f6da7347
  HEAD_REF main
)

vcpkg_check_features(
    OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        sarif GAP_ENABLE_SARIF
)

vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS
    -DGAP_ENABLE_COROUTINES=ON
    -DGAP_ENABLE_TESTING=OFF
    -DGAP_INSTALL=ON
    -DGAP_ENABLE_WARNINGS=OFF
    ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(
  PACKAGE_NAME "gap"
  CONFIG_PATH lib/cmake/gap
)

file(
  INSTALL "${SOURCE_PATH}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright
)

file(
  REMOVE_RECURSE
  "${CURRENT_PACKAGES_DIR}/debug/share"
  "${CURRENT_PACKAGES_DIR}/debug/include"
)