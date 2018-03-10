#include "KineticsPanel72OutputNode.h"
#include <iostream>
#include <algorithm>
#include <string.h>

KineticsPanel72OutputNode::KineticsPanel72OutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(new KineticsPanel72OutputNodePrivate(context, chainSize), context, chainSize, 5) {
    connect(this, &SelfTimedReadBackOutputNode::frame, this, &KineticsPanel72OutputNode::onFrame, Qt::DirectConnection);
    /*data packet header (
    ("magic", "I", 0x0401dc4a),
    ("version", "H", 0x0100),
    ("type", "H", 0x0101),
    ("sequence", "I", 0x00000000),
    ("port", "B", 0x00),
    ("padding", "B", 0x00),
    ("flags", "H", 0x0000),
    ("timer", "I", 0xffffffff),
    ("universe", "B", 0x00),)*/

    // 21 bytes for the header; 3 bytes (RGB) for each pixel
    //magic, version, type, sequence, port, padding, flags, timer, universe
    d()->outPacket->fill(0, 681);
    char const *packetHeader = "\x04\x01\xdc\x4a\x01\x00\x08\x01";
    d()->outPacket->replace(0, 8, packetHeader,8);
    d()->outPacket->replace(17, 1, "\xd1");
    d()->outPacket->replace(21, 1, "\x02");
    d()->outPacket->replace(680, 1, "\xbf"); //TODO!!! add this back if it breaks
    start();
}

KineticsPanel72OutputNode::KineticsPanel72OutputNode(const KineticsPanel72OutputNode &other)
: SelfTimedReadBackOutputNode(other) {
}

KineticsPanel72OutputNode *KineticsPanel72OutputNode::clone() const {
    return new KineticsPanel72OutputNode(*this);
}

QSharedPointer<KineticsPanel72OutputNodePrivate> KineticsPanel72OutputNode::d() const {
    return d_ptr.staticCast<KineticsPanel72OutputNodePrivate>();
}

void KineticsPanel72OutputNode::onFrame(QSize size, QByteArray frame) {
    // frame.fill(0);
    // there are two controllers for each panel
    int numPanels = (d()->sEnd - d()->sStart + 1) *2;
    for(int pIdx = 0; pIdx < numPanels; pIdx++){
        (*d()->outPacket)[16] = pIdx+1;

        int startI = pIdx%2 ==0 ? 0 : 11;
        int endI = pIdx%2 ==0 ? 6 : 6;
        int deltaI = pIdx%2 ==0 ? 1 : -1;
        int panel = d()->sStart+(pIdx)/2;
        int counter = 71;
        for(int frameI =startI; abs(frameI-endI) > 0; frameI+=deltaI) {
            for(int frameJ =11; frameJ >=0; frameJ--) {
                //          y jump down one row       y offset by panels above           //x + panel offset
                int index = (frameJ*12*d()->fWidth) + (d()->fHeight - panel/d()->fWidth - 1)*(144*d()->fWidth) + (frameI + 12*(panel%d()->fWidth));
                (*d()->outPacket)[24 + counter*3] = frame.at(index*4);
                (*d()->outPacket)[24 + counter*3+1] = frame.at(index*4+1);
                (*d()->outPacket)[24 + counter*3+2] = frame.at(index*4+2);
                counter--;
            }
        }
        // xmit[24:240] = rollaxis(asarray(panel)[x:x+12,y:y+6][::-1,::k],1).ravel()

        //send bytes to the device
        d()->udpSocket->writeDatagram(*d()->outPacket, *d()->host, 6038);
    }
}

QString KineticsPanel72OutputNode::typeName() {
    return "KineticsPanel72OutputNode";
}

void KineticsPanel72OutputNode::initDataSocket(QJsonObject obj) {
    std::string sectionStart = obj.value("sectionStart").toString().toUtf8().constData();
    sectionStart = sectionStart.compare("") == 0 ? "1" : sectionStart;
    std::string sectionEnd = obj.value("sectionEnd").toString().toUtf8().constData();
    sectionEnd = sectionEnd.compare("") == 0  ? "1" : sectionEnd;
    std::string fullWidth = obj.value("fullWidth").toString().toUtf8().constData();
    fullWidth = fullWidth.compare("") == 0 ? "1" : fullWidth;
    std::string fullHeight = obj.value("fullHeight").toString().toUtf8().constData();
    fullHeight = fullHeight.compare("") == 0 ? "1" : fullHeight;
    d()->sStart = std::stoi(sectionStart);
    d()->sEnd = std::stoi(sectionEnd);
    d()->fWidth = std::stoi(fullWidth);
    d()->fHeight = std::stoi(fullHeight);
}

VideoNode *KineticsPanel72OutputNode::deserialize(Context *context, QJsonObject obj) {
    std::string fullWidth = obj.value("fullWidth").toString().toUtf8().constData();
    fullWidth = fullWidth.compare("") == 0 ? "1" : fullWidth;
    std::string fullHeight = obj.value("fullHeight").toString().toUtf8().constData();
    fullHeight = fullHeight.compare("") == 0 ? "1" : fullHeight;
    int fWidth = std::stoi(fullWidth)*12;
    int fHeight = std::stoi(fullHeight)*12;

    KineticsPanel72OutputNode *e = new KineticsPanel72OutputNode(context, QSize(fWidth, fHeight));
    e->initDataSocket(obj);
    return e;
}

bool KineticsPanel72OutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *KineticsPanel72OutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> KineticsPanel72OutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Color Kinetics Panel72", "KineticsPanel72OutputInstantiator.qml");
    return m;
}

KineticsPanel72OutputNodePrivate::KineticsPanel72OutputNodePrivate(Context *context, QSize chainSize)
: SelfTimedReadBackOutputNodePrivate(context, chainSize)
{
    outPacket = new QByteArray();
    host = new QHostAddress();
    udpSocket = new QUdpSocket();
}
