/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QTest>
#include <Qt3DRender/private/skeleton_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DCore/qjoint.h>
#include <Qt3DCore/qskeletonloader.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qpropertyupdatedchange.h>
#include <Qt3DCore/private/qbackendnode_p.h>
#include <Qt3DCore/private/qpropertyupdatedchangebase_p.h>
#include <qbackendnodetester.h>
#include <testpostmanarbiter.h>
#include <testrenderer.h>

using namespace Qt3DCore;
using namespace Qt3DRender;
using namespace Qt3DRender::Render;

Q_DECLARE_METATYPE(Qt3DRender::Render::JointInfo)
Q_DECLARE_METATYPE(Qt3DRender::Render::SkeletonData)

namespace {

void linearizeTreeHelper(QJoint *joint, QVector<QJoint *> &joints)
{
    joints.push_back(joint);
    for (const auto child : joint->childJoints())
        linearizeTreeHelper(child, joints);
}

QVector<QJoint *> linearizeTree(QJoint *rootJoint)
{
    QVector<QJoint *> joints;
    linearizeTreeHelper(rootJoint, joints);
    return joints;
}

}

class tst_Skeleton : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:
    void checkPeerPropertyMirroring()
    {
        // GIVEN
        TestRenderer renderer;
        NodeManagers nodeManagers;
        renderer.setNodeManagers(&nodeManagers);
        Skeleton backendSkeleton;
        backendSkeleton.setRenderer(&renderer);
        QSkeletonLoader skeleton;

        skeleton.setSource(QUrl::fromLocalFile("funnybones.json"));

        // WHEN
        simulateInitialization(&skeleton, &backendSkeleton);

        // THEN
        QCOMPARE(backendSkeleton.peerId(), skeleton.id());
        QCOMPARE(backendSkeleton.isEnabled(), skeleton.isEnabled());
        QCOMPARE(backendSkeleton.source(), skeleton.source());
    }

    void checkInitialAndCleanedUpState()
    {
        // GIVEN
        TestRenderer renderer;
        NodeManagers nodeManagers;
        renderer.setNodeManagers(&nodeManagers);
        Skeleton backendSkeleton;
        backendSkeleton.setRenderer(&renderer);

        // THEN
        QVERIFY(backendSkeleton.peerId().isNull());
        QCOMPARE(backendSkeleton.isEnabled(), false);
        QCOMPARE(backendSkeleton.source(), QUrl());
        QCOMPARE(backendSkeleton.status(), QSkeletonLoader::NotReady);

        // GIVEN
        QSkeletonLoader skeleton;
        skeleton.setSource(QUrl::fromLocalFile("skeleton1.json"));

        // WHEN
        simulateInitialization(&skeleton, &backendSkeleton);
        backendSkeleton.cleanup();

        // THEN
        QCOMPARE(backendSkeleton.source(), QUrl());
        QCOMPARE(backendSkeleton.isEnabled(), false);
        QCOMPARE(backendSkeleton.status(), QSkeletonLoader::NotReady);
    }

    void checkPropertyChanges()
    {
        // GIVEN
        TestRenderer renderer;
        NodeManagers nodeManagers;
        renderer.setNodeManagers(&nodeManagers);
        Skeleton backendSkeleton;
        backendSkeleton.setRenderer(&renderer);
        backendSkeleton.setDataType(Skeleton::File);
        Qt3DCore::QPropertyUpdatedChangePtr updateChange;

        // Initialize to ensure skeleton manager is set
        QSkeletonLoader skeleton;
        skeleton.setSource(QUrl::fromLocalFile("skeleton1.json"));
        simulateInitialization(&skeleton, &backendSkeleton);

        // WHEN
        updateChange.reset(new Qt3DCore::QPropertyUpdatedChange(Qt3DCore::QNodeId()));
        updateChange->setPropertyName("enabled");
        updateChange->setValue(true);
        backendSkeleton.sceneChangeEvent(updateChange);

        // THEN
        QCOMPARE(backendSkeleton.isEnabled(), true);

        // WHEN
        const QUrl newSource = QUrl::fromLocalFile("terminator.json");
        updateChange.reset(new Qt3DCore::QPropertyUpdatedChange(Qt3DCore::QNodeId()));
        updateChange->setPropertyName("source");
        updateChange->setValue(newSource);
        backendSkeleton.sceneChangeEvent(updateChange);

        // THEN
        QCOMPARE(backendSkeleton.source(), newSource);
    }

    void checkStatusPropertyBackendNotification()
    {
        // GIVEN
        TestRenderer renderer;
        NodeManagers nodeManagers;
        renderer.setNodeManagers(&nodeManagers);
        TestArbiter arbiter;
        Skeleton backendSkeleton;
        backendSkeleton.setRenderer(&renderer);
        backendSkeleton.setEnabled(true);
        Qt3DCore::QBackendNodePrivate::get(&backendSkeleton)->setArbiter(&arbiter);

        // WHEN
        backendSkeleton.setStatus(QSkeletonLoader::Error);

        // THEN
        QCOMPARE(backendSkeleton.status(), QSkeletonLoader::Error);
        QCOMPARE(arbiter.events.count(), 1);
        Qt3DCore::QPropertyUpdatedChangePtr change = arbiter.events.first().staticCast<Qt3DCore::QPropertyUpdatedChange>();
        QCOMPARE(change->propertyName(), "status");
        QCOMPARE(change->value().value<QSkeletonLoader::Status>(), backendSkeleton.status());
        QCOMPARE(Qt3DCore::QPropertyUpdatedChangeBasePrivate::get(change.data())->m_isIntermediate,
                 false);

        arbiter.events.clear();

        // WHEN
        backendSkeleton.setStatus(QSkeletonLoader::Error);

        // THEN
        QCOMPARE(backendSkeleton.status(), QSkeletonLoader::Error);
        QCOMPARE(arbiter.events.count(), 0);

        arbiter.events.clear();
    }

    void checkCreateFrontendJoint_data()
    {
        QTest::addColumn<JointInfo>("jointInfo");
        QTest::addColumn<QJoint *>("expectedJoint");

        QTest::newRow("default") << JointInfo() << new QJoint();

        const QVector3D t(1.0f, 2.0f, 3.0f);
        const QQuaternion r = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 45.0f);
        const QVector3D s(1.5f, 2.5f, 3.5f);
        JointInfo jointInfo;
        jointInfo.localPose.scale = s;
        jointInfo.localPose.rotation = r;
        jointInfo.localPose.translation = t;

        QJoint *joint = new QJoint();
        joint->setTranslation(t);
        joint->setRotation(r);
        joint->setScale(s);
        QTest::newRow("localPose") << jointInfo << joint;

        QMatrix4x4 m;
        m.rotate(r);
        m.scale(QVector3D(1.0f, 1.0f, 1.0f) / s);
        m.translate(-t);
        jointInfo.inverseBindPose = m;

        joint = new QJoint();
        joint->setTranslation(t);
        joint->setRotation(r);
        joint->setScale(s);
        joint->setInverseBindMatrix(m);
        QTest::newRow("inverseBind") << jointInfo << joint;
    }

    void checkCreateFrontendJoint()
    {
        // GIVEN
        Skeleton backendSkeleton;
        QFETCH(JointInfo, jointInfo);
        QFETCH(QJoint *, expectedJoint);

        // WHEN
        const QJoint *actualJoint = backendSkeleton.createFrontendJoint(jointInfo);

        // THEN
        QCOMPARE(actualJoint->scale(), expectedJoint->scale());
        QCOMPARE(actualJoint->rotation(), expectedJoint->rotation());
        QCOMPARE(actualJoint->translation(), expectedJoint->translation());
        QCOMPARE(actualJoint->inverseBindMatrix(), expectedJoint->inverseBindMatrix());

        // Cleanup
        delete actualJoint;
        delete expectedJoint;
    }

    void checkCreateFrontendJoints_data()
    {
        QTest::addColumn<SkeletonData>("skeletonData");
        QTest::addColumn<QJoint *>("expectedRootJoint");

        QTest::newRow("empty") << SkeletonData() << (QJoint*)nullptr;

        SkeletonData skeletonData;
        JointInfo rootJointInfo;
        skeletonData.joints.push_back(rootJointInfo);
        const int childCount = 10;
        for (int i = 0; i < childCount; ++i) {
            JointInfo childJointInfo;
            const float x = static_cast<float>(i);
            childJointInfo.localPose.translation = QVector3D(x, x, x);
            childJointInfo.parentIndex = 0;
            skeletonData.joints.push_back(childJointInfo);
        }

        QJoint *rootJoint = new QJoint();
        for (int i = 0; i < childCount; ++i) {
            QJoint *childJoint = new QJoint();
            const float x = static_cast<float>(i);
            childJoint->setTranslation(QVector3D(x, x, x));
            rootJoint->addChildJoint(childJoint);
        }

        QTest::newRow("wide") << skeletonData << rootJoint;

        skeletonData.joints.clear();
        skeletonData.joints.push_back(rootJointInfo);
        for (int i = 0; i < childCount; ++i) {
            JointInfo childJointInfo;
            const float x = static_cast<float>(i);
            childJointInfo.localPose.translation = QVector3D(x, x, x);
            childJointInfo.parentIndex = i;
            skeletonData.joints.push_back(childJointInfo);
        }

        rootJoint = new QJoint();
        QJoint *previousJoint = rootJoint;
        for (int i = 0; i < childCount; ++i) {
            QJoint *childJoint = new QJoint();
            const float x = static_cast<float>(i);
            childJoint->setTranslation(QVector3D(x, x, x));
            previousJoint->addChildJoint(childJoint);
            previousJoint = childJoint;
        }

        QTest::newRow("deep") << skeletonData << rootJoint;
    }

    void checkCreateFrontendJoints()
    {
        // GIVEN
        Skeleton backendSkeleton;
        QFETCH(SkeletonData, skeletonData);
        QFETCH(QJoint *, expectedRootJoint);

        // WHEN
        QJoint *actualRootJoint = backendSkeleton.createFrontendJoints(skeletonData);

        // THEN
        if (skeletonData.joints.isEmpty()) {
            QVERIFY(actualRootJoint == expectedRootJoint); // nullptr
            return;
        }

        // Linearise the tree of joints and check them against the skeletonData
        QVector<QJoint *> joints = linearizeTree(actualRootJoint);
        QCOMPARE(joints.size(), skeletonData.joints.size());
        for (int i = 0; i < joints.size(); ++i) {
            // Check the translations match
            QCOMPARE(joints[i]->translation(), skeletonData.joints[i].localPose.translation);
        }

        // Now we know the order of Joints match. Check the parents match too
        for (int i = 0; i < joints.size(); ++i) {
            // Get parent index from joint info
            const int parentIndex = skeletonData.joints[i].parentIndex;
            if (parentIndex == -1) {
                QVERIFY(joints[i]->parent() == nullptr);
            } else {
                QCOMPARE(joints[i]->parent(), joints[parentIndex]);
            }
        }

        // Cleanup
        delete actualRootJoint;
        delete expectedRootJoint;
    }
};

QTEST_APPLESS_MAIN(tst_Skeleton)

#include "tst_skeleton.moc"
