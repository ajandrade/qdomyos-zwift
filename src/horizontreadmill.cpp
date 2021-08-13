#include "horizontreadmill.h"

#include "ios/lockscreen.h"
#include "virtualtreadmill.h"
#include <QBluetoothLocalDevice>
#include <QDateTime>
#include <QFile>
#include <QMetaEnum>
#include <QSettings>

#include <QThread>
#include <math.h>
#ifdef Q_OS_ANDROID
#include <QLowEnergyConnectionParameters>
#endif
#include "keepawakehelper.h"
#include <chrono>

using namespace std::chrono_literals;

#ifdef Q_OS_IOS
extern quint8 QZ_EnableDiscoveryCharsAndDescripttors;
#endif

horizontreadmill::horizontreadmill(bool noWriteResistance, bool noHeartService) {
#ifdef Q_OS_IOS
    QZ_EnableDiscoveryCharsAndDescripttors = true;
#endif

    m_watt.setType(metric::METRIC_WATT);
    Speed.setType(metric::METRIC_SPEED);
    refresh = new QTimer(this);
    this->noWriteResistance = noWriteResistance;
    this->noHeartService = noHeartService;
    initDone = false;
    connect(refresh, &QTimer::timeout, this, &horizontreadmill::update);
    refresh->start(200ms);
}

void horizontreadmill::writeCharacteristic(uint8_t *data, uint8_t data_len, QString info, bool disable_log,
                                           bool wait_for_response) {
    QEventLoop loop;
    QTimer timeout;
    if (wait_for_response) {
        connect(this, &horizontreadmill::packetReceived, &loop, &QEventLoop::quit);
        timeout.singleShot(3000, &loop, SLOT(quit()));
    } else {
        connect(gattCustomService, SIGNAL(characteristicWritten(QLowEnergyCharacteristic, QByteArray)), &loop,
                SLOT(quit()));
        timeout.singleShot(3000, &loop, SLOT(quit()));
    }

    gattCustomService->writeCharacteristic(gattWriteCharCustomService, QByteArray((const char *)data, data_len));

    if (!disable_log)
        qDebug() << " >> " << QByteArray((const char *)data, data_len).toHex(' ') << " // " << info;

    loop.exec();
}

void horizontreadmill::waitForAPacket() {
    QEventLoop loop;
    QTimer timeout;
    connect(this, &horizontreadmill::packetReceived, &loop, &QEventLoop::quit);
    timeout.singleShot(3000, &loop, SLOT(quit()));
    loop.exec();
}

void horizontreadmill::btinit() {
    uint8_t initData01[] = {0x55, 0xaa, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00};

    uint8_t initData7[] = {0x55, 0xaa, 0x02, 0x00, 0x01, 0x16, 0xdb, 0x02, 0xed, 0xc2,
                           0x00, 0x47, 0x75, 0x65, 0x73, 0x74, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x05, 0xc2, 0x07};
    uint8_t initData10[] = {0x01, 0x01, 0x00, 0xd3, 0x8a, 0x0c, 0x00, 0x01, 0x01, 0x02,
                            0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
    uint8_t initData11[] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
    uint8_t initData12[] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                            0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData13[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30,
                            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
    uint8_t initData14[] = {0x30};

    uint8_t initData7_1[] = {0x55, 0xaa, 0x03, 0x00, 0x01, 0x16, 0xdb, 0x02, 0xae, 0x2a,
                             0x01, 0x41, 0x69, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9_1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x06, 0xc4, 0x07};
    uint8_t initData10_1[] = {0x09, 0x1c, 0x00, 0x9f, 0xef, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData7_2[] = {0x55, 0xaa, 0x04, 0x00, 0x01, 0x16, 0xdb, 0x02, 0xae, 0x2a,
                             0x01, 0x41, 0x69, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9_2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x06, 0xc4, 0x07};
    uint8_t initData10_2[] = {0x09, 0x1c, 0x00, 0x9f, 0xef, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData7_3[] = {0x55, 0xaa, 0x05, 0x00, 0x01, 0x16, 0xdb, 0x02, 0xa9, 0xe7,
                             0x02, 0x4d, 0x65, 0x67, 0x68, 0x61, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9_3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x06, 0xc5, 0x07};
    uint8_t initData10_3[] = {0x0b, 0x0f, 0x00, 0x4b, 0x40, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData7_4[] = {0x55, 0xaa, 0x06, 0x00, 0x01, 0x16, 0xdb, 0x02, 0xbc, 0x76,
                             0x03, 0x44, 0x61, 0x72, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9_4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0xca, 0x07};
    uint8_t initData10_4[] = {0x05, 0x1c, 0x00, 0x07, 0x25, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData7_5[] = {0x55, 0xaa, 0x07, 0x00, 0x01, 0x16, 0xdb, 0x02, 0x7d, 0xeb,
                             0x04, 0x41, 0x68, 0x6f, 0x6e, 0x61, 0x00, 0x00, 0x00, 0x00};
    uint8_t initData9_5[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x04, 0xcc, 0x07};
    uint8_t initData10_5[] = {0x01, 0x08, 0x00, 0xc2, 0x0f, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData7_6[] = {0x55, 0xaa, 0x08, 0x00, 0x01, 0x16, 0xdb, 0x02, 0x03, 0x0d,
                             0x05, 0x55, 0x73, 0x65, 0x72, 0x20, 0x35, 0x00, 0x00, 0x00};
    uint8_t initData9_6[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x05, 0xc2, 0x07};
    uint8_t initData10_6[] = {0x01, 0x01, 0x00, 0x8e, 0x6a, 0x0c, 0x00, 0x01, 0x01, 0x02,
                              0x23, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};

    uint8_t initData02[] = {0x55, 0xaa, 0x09, 0x00, 0x01, 0x00, 0x02, 0x00, 0xb7, 0xf1, 0x1a, 0x00};
    uint8_t initData03[] = {0x55, 0xaa, 0x0a, 0x00, 0x01, 0x00, 0x02, 0x00, 0xb7, 0xf1, 0x1a, 0x00};
    uint8_t initData04[] = {0x55, 0xaa, 0x0b, 0x00, 0x01, 0x00, 0x02, 0x00, 0xb7, 0xf1, 0x1a, 0x00};
    uint8_t initData05[] = {0x55, 0xaa, 0x0c, 0x00, 0x01, 0x00, 0x02, 0x00, 0xb7, 0xf1, 0x1a, 0x00};
    uint8_t initData06[] = {0x55, 0xaa, 0x0d, 0x00, 0x01, 0x00, 0x02, 0x00, 0xb7, 0xf1, 0x1a, 0x00};

    uint8_t initData2[] = {0x55, 0xaa, 0x0e, 0x00, 0x03, 0x14, 0x08, 0x00, 0x3f,
                           0x01, 0xe5, 0x07, 0x02, 0x08, 0x13, 0x12, 0x21, 0x00};
    uint8_t initData3[] = {0x55, 0xaa, 0x0f, 0x00, 0x03, 0x01, 0x01, 0x00, 0xd1, 0xf1, 0x01};
    uint8_t initData4[] = {0x55, 0xaa, 0x10, 0x00, 0x03, 0x10, 0x01, 0x00, 0xf0, 0xe1, 0x00};
    uint8_t initData5[] = {0x55, 0xaa, 0x11, 0x00, 0x03, 0x02, 0x11, 0x00, 0x84, 0xbe,
                           0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};
    uint8_t initData6[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01};

    if (gattCustomService) {
        writeCharacteristic(initData01, sizeof(initData01), QStringLiteral("init"), false, true);
        waitForAPacket();

        writeCharacteristic(initData7, sizeof(initData7), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9, sizeof(initData9), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10, sizeof(initData10), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_1, sizeof(initData7_1), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_1, sizeof(initData9_1), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_1, sizeof(initData10_1), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_2, sizeof(initData7_2), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_2, sizeof(initData9_2), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_2, sizeof(initData10_2), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_3, sizeof(initData7_3), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_3, sizeof(initData9_3), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_3, sizeof(initData10_3), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_4, sizeof(initData7_4), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_4, sizeof(initData9_4), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_4, sizeof(initData10_4), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_5, sizeof(initData7_5), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_5, sizeof(initData9_5), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_5, sizeof(initData10_5), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData7_6, sizeof(initData7_6), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData9_6, sizeof(initData9_6), QStringLiteral("init"), false, false);
        writeCharacteristic(initData10_6, sizeof(initData10_6), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData11, sizeof(initData11), QStringLiteral("init"), false, false);
        writeCharacteristic(initData12, sizeof(initData12), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData8, sizeof(initData8), QStringLiteral("init"), false, false);
        writeCharacteristic(initData13, sizeof(initData13), QStringLiteral("init"), false, false);
        writeCharacteristic(initData14, sizeof(initData14), QStringLiteral("init"), false, true);

        writeCharacteristic(initData02, sizeof(initData02), QStringLiteral("init"), false, true);
        writeCharacteristic(initData03, sizeof(initData03), QStringLiteral("init"), false, true);
        writeCharacteristic(initData04, sizeof(initData04), QStringLiteral("init"), false, true);
        writeCharacteristic(initData05, sizeof(initData05), QStringLiteral("init"), false, true);
        writeCharacteristic(initData06, sizeof(initData06), QStringLiteral("init"), false, true);
        writeCharacteristic(initData2, sizeof(initData2), QStringLiteral("init"), false, true);
        writeCharacteristic(initData3, sizeof(initData3), QStringLiteral("init"), false, true);
        writeCharacteristic(initData4, sizeof(initData4), QStringLiteral("init"), false, true);
        writeCharacteristic(initData5, sizeof(initData5), QStringLiteral("init"), false, false);
        writeCharacteristic(initData6, sizeof(initData6), QStringLiteral("init"), false, true);

        messageID = 0x11;
    }

    initDone = true;
}

void horizontreadmill::update() {
    if (m_control->state() == QLowEnergyController::UnconnectedState) {

        emit disconnected();
        return;
    }

    if (initRequest && firstStateChanged) {
        btinit();
        initRequest = false;
    } else if (bluetoothDevice.isValid() //&&

               // m_control->state() == QLowEnergyController::DiscoveredState //&&
               // gattCommunicationChannelService &&
               // gattWriteCharacteristic.isValid() &&
               // gattNotify1Characteristic.isValid() &&
               /*initDone*/) {

        QSettings settings;
        update_metrics(true, watts(settings.value(QStringLiteral("weight"), 75.0).toFloat()));

        // updating the treadmill console every second
        if (sec1Update++ == (500 / refresh->interval())) {

            sec1Update = 0;
            // updateDisplay(elapsed);
        }

        if (requestSpeed != -1) {
            if (requestSpeed != currentSpeed().value() && requestSpeed >= 0 && requestSpeed <= 22) {
                emit debug(QStringLiteral("writing speed ") + QString::number(requestSpeed));

                double inc = Inclination.value();
                if (requestInclination != -1) {

                    inc = requestInclination;
                    requestInclination = -1;
                }
                forceSpeedOrIncline(requestSpeed, inc);
            }
            requestSpeed = -1;
        }
        if (requestInclination != -1) {
            if (requestInclination != currentInclination().value() && requestInclination >= 0 &&
                requestInclination <= 15) {
                emit debug(QStringLiteral("writing incline ") + QString::number(requestInclination));

                double speed = currentSpeed().value();
                if (requestSpeed != -1) {

                    speed = requestSpeed;
                    requestSpeed = -1;
                }
                forceSpeedOrIncline(speed, requestInclination);
            }
            requestInclination = -1;
        }
        if (requestStart != -1) {
            emit debug(QStringLiteral("starting..."));
            if (lastSpeed == 0.0) {

                lastSpeed = 0.5;
            }
            requestStart = -1;
            emit tapeStarted();
        }
        if (requestStop != -1) {
            emit debug(QStringLiteral("stopping..."));

            requestStop = -1;
        }
        if (requestIncreaseFan != -1) {
            emit debug(QStringLiteral("increasing fan speed..."));

            // sendChangeFanSpeed(FanSpeed + 1);
            requestIncreaseFan = -1;
        } else if (requestDecreaseFan != -1) {
            emit debug(QStringLiteral("decreasing fan speed..."));

            // sendChangeFanSpeed(FanSpeed - 1);
            requestDecreaseFan = -1;
        }
    }
}

void horizontreadmill::forceSpeedOrIncline(double requestSpeed, double requestIncline) {
    Q_UNUSED(requestSpeed)
    Q_UNUSED(requestIncline)
    // TODO
}

void horizontreadmill::serviceDiscovered(const QBluetoothUuid &gatt) {
    emit debug(QStringLiteral("serviceDiscovered ") + gatt.toString());
}

void horizontreadmill::characteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                             const QByteArray &newValue) {
    double heart = 0; // NOTE : Should be initialized with a value to shut clang-analyzer's
                      // UndefinedBinaryOperatorResult
    // qDebug() << "characteristicChanged" << characteristic.uuid() << newValue << newValue.length();
    Q_UNUSED(characteristic);
    QSettings settings;
    QString heartRateBeltName =
        settings.value(QStringLiteral("heart_rate_belt_name"), QStringLiteral("Disabled")).toString();

    emit debug(QStringLiteral(" << ") + characteristic.uuid().toString() + " " + QString::number(newValue.length()) +
               " " + newValue.toHex(' '));

    if (characteristic.uuid() == QBluetoothUuid((quint16)0xFFF4)) {
        if (newValue.at(0) == 0x55) {
            customRecv = (((uint16_t)((uint8_t)newValue.at(7)) << 8) | (uint16_t)((uint8_t)newValue.at(6))) + 10;
            qDebug() << "new custom packet received. Len expected: " << customRecv;
        }

        customRecv -= newValue.length();
        if (customRecv <= 0) {
            qDebug() << "full custom packet received";
            customRecv = 0;
            emit packetReceived();
        }
    }

    if (characteristic.uuid() == QBluetoothUuid((quint16)0xFFF4) && newValue.length() > 70 && newValue.at(0) == 0x55 &&
        newValue.at(5) == 0x12) {
        Speed =
            (((double)(((uint16_t)((uint8_t)newValue.at(62)) << 8) | (uint16_t)((uint8_t)newValue.at(61)))) / 1000.0) *
            1.60934; // miles/h
        emit debug(QStringLiteral("Current Speed: ") + QString::number(Speed.value()));

        Inclination = (double)((uint8_t)newValue.at(63)) / 10.0;
        emit debug(QStringLiteral("Current Inclination: ") + QString::number(Inclination.value()));

        KCal += ((((0.048 * ((double)watts(settings.value(QStringLiteral("weight"), 75.0).toFloat())) + 1.19) *
                   settings.value(QStringLiteral("weight"), 75.0).toFloat() * 3.5) /
                  200.0) /
                 (60000.0 / ((double)lastRefreshCharacteristicChanged.msecsTo(
                                QDateTime::currentDateTime())))); //(( (0.048* Output in watts +1.19) * body weight in
                                                                  // kg * 3.5) / 200 ) / 60

        emit debug(QStringLiteral("Current KCal: ") + QString::number(KCal.value()));

        Distance += ((Speed.value() / 3600000.0) *
                     ((double)lastRefreshCharacteristicChanged.msecsTo(QDateTime::currentDateTime())));
        emit debug(QStringLiteral("Current Distance: ") + QString::number(Distance.value()));
    } else if (characteristic.uuid() == QBluetoothUuid((quint16)0xFFF4) && newValue.length() == 29 &&
               newValue.at(0) == 0x55) {
        Speed = ((double)(((uint16_t)((uint8_t)newValue.at(15)) << 8) | (uint16_t)((uint8_t)newValue.at(14)))) / 10.0;
        emit debug(QStringLiteral("Current Speed: ") + QString::number(Speed.value()));

        // Inclination = (double)((uint8_t)newValue.at(3)) / 10.0;
        // emit debug(QStringLiteral("Current Inclination: ") + QString::number(Inclination.value()));

        KCal += ((((0.048 * ((double)watts(settings.value(QStringLiteral("weight"), 75.0).toFloat())) + 1.19) *
                   settings.value(QStringLiteral("weight"), 75.0).toFloat() * 3.5) /
                  200.0) /
                 (60000.0 / ((double)lastRefreshCharacteristicChanged.msecsTo(
                                QDateTime::currentDateTime())))); //(( (0.048* Output in watts +1.19) * body weight in
        // kg * 3.5) / 200 ) / 60

        emit debug(QStringLiteral("Current KCal: ") + QString::number(KCal.value()));

        Distance += ((Speed.value() / 3600000.0) *
                     ((double)lastRefreshCharacteristicChanged.msecsTo(QDateTime::currentDateTime())));
        emit debug(QStringLiteral("Current Distance: ") + QString::number(Distance.value()));
    } else if (characteristic.uuid() == QBluetoothUuid((quint16)0x2ACD)) {
        lastPacket = newValue;

        // default flags for this treadmill is 84 04

        union flags {
            struct {

                uint16_t moreData : 1;
                uint16_t avgSpeed : 1;
                uint16_t totalDistance : 1;
                uint16_t inclination : 1;
                uint16_t elevation : 1;
                uint16_t instantPace : 1;
                uint16_t averagePace : 1;
                uint16_t expEnergy : 1;
                uint16_t heartRate : 1;
                uint16_t metabolic : 1;
                uint16_t elapsedTime : 1;
                uint16_t remainingTime : 1;
                uint16_t forceBelt : 1;
                uint16_t spare : 3;
            };

            uint16_t word_flags;
        };

        flags Flags;
        int index = 0;
        Flags.word_flags = (newValue.at(1) << 8) | newValue.at(0);
        index += 2;

        if (!Flags.moreData) {
            Speed = ((double)(((uint16_t)((uint8_t)newValue.at(index + 1)) << 8) |
                              (uint16_t)((uint8_t)newValue.at(index)))) /
                    100.0;
            index += 2;
            emit debug(QStringLiteral("Current Speed: ") + QString::number(Speed.value()));
        }

        if (Flags.avgSpeed) {
            double avgSpeed;
            avgSpeed = ((double)(((uint16_t)((uint8_t)newValue.at(index + 1)) << 8) |
                                 (uint16_t)((uint8_t)newValue.at(index)))) /
                       100.0;
            index += 2;
            emit debug(QStringLiteral("Current Average Speed: ") + QString::number(avgSpeed));
        }

        if (Flags.totalDistance) {
            // ignoring the distance, because it's a total life odometer
            // Distance = ((double)((((uint32_t)((uint8_t)newValue.at(index + 2)) << 16) |
            // (uint32_t)((uint8_t)newValue.at(index + 1)) << 8) | (uint32_t)((uint8_t)newValue.at(index)))) / 1000.0;
            index += 3;
        }
        // else
        {
            Distance += ((Speed.value() / 3600000.0) *
                         ((double)lastRefreshCharacteristicChanged.msecsTo(QDateTime::currentDateTime())));
        }

        emit debug(QStringLiteral("Current Distance: ") + QString::number(Distance.value()));

        if (Flags.inclination) {
            Inclination = ((double)(((uint16_t)((uint8_t)newValue.at(index + 1)) << 8) |
                                    (uint16_t)((uint8_t)newValue.at(index)))) /
                          10.0;
            index += 4; // the ramo value is useless
            emit debug(QStringLiteral("Current Inclination: ") + QString::number(Inclination.value()));
        }

        if (Flags.elevation) {
            index += 4; // TODO
        }

        if (Flags.instantPace) {
            index += 1; // TODO
        }

        if (Flags.averagePace) {
            index += 1; // TODO
        }

        if (Flags.expEnergy) {
            KCal = ((double)(((uint16_t)((uint8_t)newValue.at(index + 1)) << 8) |
                             (uint16_t)((uint8_t)newValue.at(index))));
            index += 2;

            // energy per hour
            index += 2;

            // energy per minute
            index += 1;
        } else {
            KCal +=
                ((((0.048 * ((double)watts(settings.value(QStringLiteral("weight"), 75.0).toFloat())) + 1.19) *
                   settings.value(QStringLiteral("weight"), 75.0).toFloat() * 3.5) /
                  200.0) /
                 (60000.0 / ((double)lastRefreshCharacteristicChanged.msecsTo(
                                QDateTime::currentDateTime())))); //(( (0.048* Output in watts +1.19) * body weight in
                                                                  // kg * 3.5) / 200 ) / 60
        }

        emit debug(QStringLiteral("Current KCal: ") + QString::number(KCal.value()));

#ifdef Q_OS_ANDROID
        if (settings.value("ant_heart", false).toBool())
            Heart = (uint8_t)KeepAwakeHelper::heart();
        else
#endif
        {
            if (Flags.heartRate) {
                if (index < newValue.length()) {

                    heart = ((double)((newValue.at(index))));
                    emit debug(QStringLiteral("Current Heart: ") + QString::number(heart));
                } else {
                    emit debug(QStringLiteral("Error on parsing heart!"));
                }
                // index += 1; //NOTE: clang-analyzer-deadcode.DeadStores
            }
        }

        if (Flags.metabolic) {
            // todo
        }

        if (Flags.elapsedTime) {
            // todo
        }

        if (Flags.remainingTime) {
            // todo
        }

        if (Flags.forceBelt) {
            // todo
        }
    }

    if (heartRateBeltName.startsWith(QStringLiteral("Disabled"))) {
        if (heart == 0.0 || settings.value(QStringLiteral("heart_ignore_builtin"), false).toBool()) {

#ifdef Q_OS_IOS
#ifndef IO_UNDER_QT
            lockscreen h;
            long appleWatchHeartRate = h.heartRate();
            h.setKcal(KCal.value());
            h.setDistance(Distance.value());
            Heart = appleWatchHeartRate;
            debug("Current Heart from Apple Watch: " + QString::number(appleWatchHeartRate));
#endif
#endif
        } else {

            Heart = heart;
        }
    }

    lastRefreshCharacteristicChanged = QDateTime::currentDateTime();

    if (m_control->error() != QLowEnergyController::NoError) {
        qDebug() << QStringLiteral("QLowEnergyController ERROR!!") << m_control->errorString();
    }
}

void horizontreadmill::stateChanged(QLowEnergyService::ServiceState state) {
    QMetaEnum metaEnum = QMetaEnum::fromType<QLowEnergyService::ServiceState>();
    QBluetoothUuid _gattWriteCharCustomService((quint16)0xFFF3);
    QBluetoothUuid _gattWriteCharControlPointId((quint16)0x2AD9);
    emit debug(QStringLiteral("BTLE stateChanged ") + QString::fromLocal8Bit(metaEnum.valueToKey(state)));

    for (QLowEnergyService *s : qAsConst(gattCommunicationChannelService)) {
        qDebug() << QStringLiteral("stateChanged") << s->serviceUuid() << s->state();
        if (s->state() != QLowEnergyService::ServiceDiscovered && s->state() != QLowEnergyService::InvalidService) {
            qDebug() << QStringLiteral("not all services discovered");
            return;
        }
    }

    qDebug() << QStringLiteral("all services discovered!");

    notificationSubscribed = 0;

    for (QLowEnergyService *s : qAsConst(gattCommunicationChannelService)) {
        if (s->state() == QLowEnergyService::ServiceDiscovered) {
            // establish hook into notifications
            connect(s, &QLowEnergyService::characteristicChanged, this, &horizontreadmill::characteristicChanged);
            connect(s, &QLowEnergyService::characteristicWritten, this, &horizontreadmill::characteristicWritten);
            connect(s, &QLowEnergyService::characteristicRead, this, &horizontreadmill::characteristicRead);
            connect(
                s, static_cast<void (QLowEnergyService::*)(QLowEnergyService::ServiceError)>(&QLowEnergyService::error),
                this, &horizontreadmill::errorService);
            connect(s, &QLowEnergyService::descriptorWritten, this, &horizontreadmill::descriptorWritten);
            connect(s, &QLowEnergyService::descriptorRead, this, &horizontreadmill::descriptorRead);

            qDebug() << s->serviceUuid() << QStringLiteral("connected!");

            auto characteristics_list = s->characteristics();
            for (const QLowEnergyCharacteristic &c : qAsConst(characteristics_list)) {
                qDebug() << QStringLiteral("char uuid") << c.uuid() << QStringLiteral("handle") << c.handle();

                if (c.properties() & QLowEnergyCharacteristic::Write && c.uuid() == _gattWriteCharControlPointId) {
                    qDebug() << QStringLiteral("FTMS service and Control Point found");
                    gattWriteCharControlPointId = c;
                    gattFTMSService = s;
                }

                if (c.properties() & QLowEnergyCharacteristic::Write && c.uuid() == _gattWriteCharCustomService) {
                    qDebug() << QStringLiteral("Custom service and Control Point found");
                    gattWriteCharCustomService = c;
                    gattCustomService = s;
                }
            }
        }
    }

    for (QLowEnergyService *s : qAsConst(gattCommunicationChannelService)) {
        if (s->state() == QLowEnergyService::ServiceDiscovered) {
            auto characteristics_list = s->characteristics();
            for (const QLowEnergyCharacteristic &c : qAsConst(characteristics_list)) {
                auto descriptors_list = c.descriptors();
                for (const QLowEnergyDescriptor &d : qAsConst(descriptors_list)) {
                    qDebug() << QStringLiteral("descriptor uuid") << d.uuid() << QStringLiteral("handle") << d.handle();
                }

                if ((c.properties() & QLowEnergyCharacteristic::Notify) == QLowEnergyCharacteristic::Notify &&
                    ((s->serviceUuid() == gattFTMSService->serviceUuid() && !gattCustomService) ||
                     (s->serviceUuid() == gattCustomService->serviceUuid() && gattCustomService))) {
                    QByteArray descriptor;
                    descriptor.append((char)0x01);
                    descriptor.append((char)0x00);
                    if (c.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration).isValid()) {
                        s->writeDescriptor(c.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), descriptor);
                        notificationSubscribed++;
                    } else {
                        qDebug() << QStringLiteral("ClientCharacteristicConfiguration") << c.uuid()
                                 << c.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration).uuid()
                                 << c.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration).handle()
                                 << QStringLiteral(" is not valid");
                    }

                    qDebug() << s->serviceUuid() << c.uuid() << QStringLiteral("notification subscribed!");
                }
            }
        }
    }

    // ******************************************* virtual treadmill init *************************************
    if (!firstStateChanged && !virtualTreadmill
#ifdef Q_OS_IOS
#ifndef IO_UNDER_QT
        && !h
#endif
#endif
    ) {

        QSettings settings;
        bool virtual_device_enabled = settings.value(QStringLiteral("virtual_device_enabled"), true).toBool();
        if (virtual_device_enabled) {
            emit debug(QStringLiteral("creating virtual treadmill interface..."));

            virtualTreadmill = new virtualtreadmill(this, noHeartService);
            connect(virtualTreadmill, &virtualtreadmill::debug, this, &horizontreadmill::debug);
        }
    }
    firstStateChanged = 1;
    // ********************************************************************************************************
}

void horizontreadmill::descriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue) {
    emit debug(QStringLiteral("descriptorWritten ") + descriptor.name() + QStringLiteral(" ") + newValue.toHex(' '));

    if (notificationSubscribed)
        notificationSubscribed--;

    if (!notificationSubscribed) {
        initRequest = true;
        emit connectedAndDiscovered();
    }
}

void horizontreadmill::descriptorRead(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue) {
    qDebug() << QStringLiteral("descriptorRead ") << descriptor.name() << descriptor.uuid() << newValue.toHex(' ');
}

void horizontreadmill::characteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                             const QByteArray &newValue) {
    Q_UNUSED(characteristic);
    emit debug(QStringLiteral("characteristicWritten ") + newValue.toHex(' '));
}

void horizontreadmill::characteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue) {
    qDebug() << QStringLiteral("characteristicRead ") << characteristic.uuid() << newValue.toHex(' ');
}

void horizontreadmill::serviceScanDone(void) {
    emit debug(QStringLiteral("serviceScanDone"));

    initRequest = false;
    firstStateChanged = 0;
    auto services_list = m_control->services();
    for (const QBluetoothUuid &s : qAsConst(services_list)) {
        gattCommunicationChannelService.append(m_control->createServiceObject(s));
        connect(gattCommunicationChannelService.constLast(), &QLowEnergyService::stateChanged, this,
                &horizontreadmill::stateChanged);
        gattCommunicationChannelService.constLast()->discoverDetails();
    }
}

void horizontreadmill::errorService(QLowEnergyService::ServiceError err) {

    QMetaEnum metaEnum = QMetaEnum::fromType<QLowEnergyService::ServiceError>();
    emit debug(QStringLiteral("horizontreadmill::errorService") + QString::fromLocal8Bit(metaEnum.valueToKey(err)) +
               m_control->errorString());
}

void horizontreadmill::error(QLowEnergyController::Error err) {

    QMetaEnum metaEnum = QMetaEnum::fromType<QLowEnergyController::Error>();
    emit debug(QStringLiteral("horizontreadmill::error") + QString::fromLocal8Bit(metaEnum.valueToKey(err)) +
               m_control->errorString());
}

void horizontreadmill::deviceDiscovered(const QBluetoothDeviceInfo &device) {

    // ***************************************************************************************************************
    // horizon treadmill and F80 treadmill, so if we want to add inclination support we have to separate the 2 devices
    // ***************************************************************************************************************
    emit debug(QStringLiteral("Found new device: ") + device.name() + QStringLiteral(" (") +
               device.address().toString() + ')');
    {
        bluetoothDevice = device;

        m_control = QLowEnergyController::createCentral(bluetoothDevice, this);
        connect(m_control, &QLowEnergyController::serviceDiscovered, this, &horizontreadmill::serviceDiscovered);
        connect(m_control, &QLowEnergyController::discoveryFinished, this, &horizontreadmill::serviceScanDone);
        connect(m_control,
                static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
                this, &horizontreadmill::error);
        connect(m_control, &QLowEnergyController::stateChanged, this, &horizontreadmill::controllerStateChanged);

        connect(m_control,
                static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
                this, [this](QLowEnergyController::Error error) {
                    Q_UNUSED(error);
                    Q_UNUSED(this);
                    emit debug(QStringLiteral("Cannot connect to remote device."));
                    emit disconnected();
                });
        connect(m_control, &QLowEnergyController::connected, this, [this]() {
            Q_UNUSED(this);
            emit debug(QStringLiteral("Controller connected. Search services..."));
            m_control->discoverServices();
        });
        connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
            Q_UNUSED(this);
            emit debug(QStringLiteral("LowEnergy controller disconnected"));
            emit disconnected();
        });

        // Connect
        m_control->connectToDevice();
        return;
    }
}

bool horizontreadmill::connected() {
    if (!m_control) {

        return false;
    }
    return m_control->state() == QLowEnergyController::DiscoveredState;
}

void *horizontreadmill::VirtualTreadmill() { return virtualTreadmill; }

void *horizontreadmill::VirtualDevice() { return VirtualTreadmill(); }

void horizontreadmill::controllerStateChanged(QLowEnergyController::ControllerState state) {
    qDebug() << QStringLiteral("controllerStateChanged") << state;
    if (state == QLowEnergyController::UnconnectedState && m_control) {
        qDebug() << QStringLiteral("trying to connect back again...");

        initDone = false;
        m_control->connectToDevice();
    }
}

int CRC_TABLE[256] = {
       iArr[1] = 4129;
       iArr[2] = 8258;
       iArr[3] = 12387;
       iArr[4] = 16516;
       iArr[5] = 20645;
       iArr[6] = 24774;
       iArr[7] = 28903;
       iArr[8] = 33032;
       iArr[9] = 37161;
       iArr[10] = 41290;
       iArr[11] = 45419;
       iArr[12] = 49548;
       iArr[13] = 53677;
       iArr[14] = 57806;
       iArr[15] = 61935;
       iArr[16] = 4657;
       iArr[17] = 528;
       iArr[18] = 12915;
       iArr[19] = 8786;
       iArr[20] = 21173;
       iArr[21] = 17044;
       iArr[22] = 29431;
       iArr[23] = 25302;
       iArr[24] = 37689;
       iArr[25] = 33560;
       iArr[26] = 45947;
       iArr[27] = 41818;
       iArr[28] = 54205;
       iArr[29] = 50076;
       iArr[30] = 62463;
       iArr[31] = 58334;
       iArr[32] = 9314;
       iArr[33] = 13379;
       iArr[34] = 1056;
       iArr[35] = 5121;
       iArr[36] = 25830;
       iArr[37] = 29895;
       iArr[38] = 17572;
       iArr[39] = 21637;
       iArr[40] = 42346;
       iArr[41] = 46411;
       iArr[42] = 34088;
       iArr[43] = 38153;
       iArr[44] = 58862;
       iArr[45] = 62927;
       iArr[46] = 50604;
       iArr[47] = 54669;
       iArr[48] = 13907;
       iArr[49] = 9842;
       iArr[50] = 5649;
       iArr[51] = 1584;
       iArr[52] = 30423;
       iArr[53] = 26358;
       iArr[54] = 22165;
       iArr[55] = 18100;
       iArr[56] = 46939;
       iArr[57] = 42874;
       iArr[58] = 38681;
       iArr[59] = 34616;
       iArr[60] = 63455;
       iArr[61] = 59390;
       iArr[62] = 55197;
       iArr[63] = 51132;
       iArr[64] = 18628;
       iArr[65] = 22757;
       iArr[66] = 26758;
       iArr[67] = 30887;
       iArr[68] = 2112;
       iArr[69] = 6241;
       iArr[70] = 10242;
       iArr[71] = 14371;
       iArr[72] = 51660;
       iArr[73] = 55789;
       iArr[74] = 59790;
       iArr[75] = 63919;
       iArr[76] = 35144;
       iArr[77] = 39273;
       iArr[78] = 43274;
       iArr[79] = 47403;
       iArr[80] = 23285;
       iArr[81] = 19156;
       iArr[82] = 31415;
       iArr[83] = 27286;
       iArr[84] = 6769;
       iArr[85] = 2640;
       iArr[86] = 14899;
       iArr[87] = 10770;
       iArr[88] = 56317;
       iArr[89] = 52188;
       iArr[90] = 64447;
       iArr[91] = 60318;
       iArr[92] = 39801;
       iArr[93] = 35672;
       iArr[94] = 47931;
       iArr[95] = 43802;
       iArr[96] = 27814;
       iArr[97] = 31879;
       iArr[98] = 19684;
       iArr[99] = 23749;
       iArr[100] = 11298;
       iArr[101] = 15363;
       iArr[102] = 3168;
       iArr[103] = 7233;
       iArr[104] = 60846;
       iArr[105] = 64911;
       iArr[106] = 52716;
       iArr[107] = 56781;
       iArr[108] = 44330;
       iArr[109] = 48395;
       iArr[110] = 36200;
       iArr[111] = 40265;
       iArr[112] = 32407;
       iArr[113] = 28342;
       iArr[114] = 24277;
       iArr[115] = 20212;
       iArr[116] = 15891;
       iArr[117] = 11826;
       iArr[118] = 7761;
       iArr[119] = 3696;
       iArr[120] = 65439;
       iArr[121] = 61374;
       iArr[122] = 57309;
       iArr[123] = 53244;
       iArr[124] = 48923;
       iArr[125] = 44858;
       iArr[126] = 40793;
       iArr[127] = 36728;
       iArr[128] = 37256;
       iArr[129] = 33193;
       iArr[130] = 45514;
       iArr[131] = 41451;
       iArr[132] = 53516;
       iArr[133] = 49453;
       iArr[134] = 61774;
       iArr[135] = 57711;
       iArr[136] = 4224;
       iArr[137] = 161;
       iArr[138] = 12482;
       iArr[139] = 8419;
       iArr[140] = 20484;
       iArr[141] = 16421;
       iArr[142] = 28742;
       iArr[143] = 24679;
       iArr[144] = 33721;
       iArr[145] = 37784;
       iArr[146] = 41979;
       iArr[147] = 46042;
       iArr[148] = 49981;
       iArr[149] = 54044;
       iArr[150] = 58239;
       iArr[151] = 62302;
       iArr[152] = 689;
       iArr[153] = 4752;
       iArr[154] = 8947;
       iArr[155] = 13010;
       iArr[156] = 16949;
       iArr[157] = 21012;
       iArr[158] = 25207;
       iArr[159] = 29270;
       iArr[160] = 46570;
       iArr[161] = 42443;
       iArr[162] = 38312;
       iArr[163] = 34185;
       iArr[164] = 62830;
       iArr[165] = 58703;
       iArr[166] = 54572;
       iArr[167] = 50445;
       iArr[168] = 13538;
       iArr[169] = 9411;
       iArr[170] = 5280;
       iArr[171] = 1153;
       iArr[172] = 29798;
       iArr[173] = 25671;
       iArr[174] = 21540;
       iArr[175] = 17413;
       iArr[176] = 42971;
       iArr[177] = 47098;
       iArr[178] = 34713;
       iArr[179] = 38840;
       iArr[180] = 59231;
       iArr[181] = 63358;
       iArr[182] = 50973;
       iArr[183] = 55100;
       iArr[184] = 9939;
       iArr[185] = 14066;
       iArr[186] = 1681;
       iArr[187] = 5808;
       iArr[188] = 26199;
       iArr[189] = 30326;
       iArr[190] = 17941;
       iArr[191] = 22068;
       iArr[192] = 55628;
       iArr[193] = 51565;
       iArr[194] = 63758;
       iArr[195] = 59695;
       iArr[196] = 39368;
       iArr[197] = 35305;
       iArr[198] = 47498;
       iArr[199] = 43435;
       iArr[200] = 22596;
       iArr[201] = 18533;
       iArr[202] = 30726;
       iArr[203] = 26663;
       iArr[204] = 6336;
       iArr[205] = 2273;
       iArr[206] = 14466;
       iArr[207] = 10403;
       iArr[208] = 52093;
       iArr[209] = 56156;
       iArr[210] = 60223;
       iArr[211] = 64286;
       iArr[212] = 35833;
       iArr[213] = 39896;
       iArr[214] = 43963;
       iArr[215] = 48026;
       iArr[216] = 19061;
       iArr[217] = 23124;
       iArr[218] = 27191;
       iArr[219] = 31254;
       iArr[220] = 2801;
       iArr[221] = 6864;
       iArr[222] = 10931;
       iArr[223] = 14994;
       iArr[224] = 64814;
       iArr[225] = 60687;
       iArr[226] = 56684;
       iArr[227] = 52557;
       iArr[228] = 48554;
       iArr[229] = 44427;
       iArr[230] = 40424;
       iArr[231] = 36297;
       iArr[232] = 31782;
       iArr[233] = 27655;
       iArr[234] = 23652;
       iArr[235] = 19525;
       iArr[236] = 15522;
       iArr[237] = 11395;
       iArr[238] = 7392;
       iArr[239] = 3265;
       iArr[240] = 61215;
       iArr[241] = 65342;
       iArr[242] = 53085;
       iArr[243] = 57212;
       iArr[244] = 44955;
       iArr[245] = 49082;
       iArr[246] = 36825;
       iArr[247] = 40952;
       iArr[248] = 28183;
       iArr[249] = 32310;
       iArr[250] = 20053;
       iArr[251] = 24180;
       iArr[252] = 11923;
       iArr[253] = 16050;
       iArr[254] = 3793;
       iArr[255] = 7920;
   }

int horizontreadmill::GenerateCRC_CCITT(QByteArray PUPtr8, int PU16_Count) {
    if (PU16_Count == 0) {
        return 0;
    }
    int crc = 65535;
    for (int i = 0; i < PU16_Count; i++) {
        crc = ((crc << 8) & 65280) ^ CRC_TABLE[((crc & 65280) >> 8) ^ ((PUPtr8[i] & 255) & 255)];
    }
    return crc;
}
