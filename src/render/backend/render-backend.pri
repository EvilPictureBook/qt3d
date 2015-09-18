INCLUDEPATH += $$PWD

include("framegraph/framegraph.pri")
include("jobs/render-jobs.pri")

HEADERS += \
    $$PWD/renderthread_p.h \
    $$PWD/renderconfiguration_p.h \
    $$PWD/renderer_p.h \
    $$PWD/quniformvalue_p.h \
    $$PWD/renderview_p.h \
    $$PWD/rendercommand_p.h \
    $$PWD/renderqueue_p.h \
    $$PWD/parameterpack_p.h \
    $$PWD/rendertarget_p.h \
    $$PWD/renderattachment_p.h \
    $$PWD/scenemanager_p.h \
    $$PWD/attachmentpack_p.h \
    $$PWD/shadervariables_p.h \
    $$PWD/qgraphicsutils_p.h \
    $$PWD/managers_p.h \
    $$PWD/handle_types_p.h \
    $$PWD/platformsurfacefilter_p.h \
    $$PWD/cameralens_p.h \
    $$PWD/entity_p.h \
    $$PWD/layer_p.h \
    $$PWD/nodefunctor_p.h \
    $$PWD/scene_p.h \
    $$PWD/transform_p.h

SOURCES += \
    $$PWD/renderthread.cpp \
    $$PWD/renderconfiguration.cpp \
    $$PWD/renderer.cpp \
    $$PWD/quniformvalue.cpp \
    $$PWD/renderview.cpp \
    $$PWD/rendercommand.cpp \
    $$PWD/renderqueue.cpp \
    $$PWD/parameterpack.cpp \
    $$PWD/rendertarget.cpp \
    $$PWD/renderattachment.cpp \
    $$PWD/scenemanager.cpp \
    $$PWD/attachmentpack.cpp \
    $$PWD/platformsurfacefilter.cpp \
    $$PWD/cameralens.cpp \
    $$PWD/entity.cpp \
    $$PWD/layer.cpp \
    $$PWD/scene.cpp \
    $$PWD/transform.cpp
