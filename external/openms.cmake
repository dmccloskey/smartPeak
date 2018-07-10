include (ExternalProject)

set_property (DIRECTORY PROPERTY EP_BASE Dependencies)

set (DEPENDENCIES)

set (OPENMS_VERSION develop)
message (STATUS "Adding OPENMS ${OPENMS_VERSION} as an external project.")

ExternalProject_Add(openms
    GIT_REPOSITORY "https://github.com/OpenMS/OpenMS.git"
    GIT_TAG "develop"
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX=${INCLUDE_DIR_OPENMS}
        -DPYOPENMS=OFF
        #-DCMAKE_PREFIX_PATH="/usr/local/contrib-build/;/usr/local/contrib/;/usr/;/usr/local"
        -DBOOST_USE_STATIC=OFF
        -DHAS_XSERVER=OFF
        -DWITH_GUI=OFF
    GIT_PROGRESS 1
    LOG_DOWNLOAD 1
    LOG_UPDATE 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
)
