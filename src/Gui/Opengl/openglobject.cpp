#include "openglobject.h"

OpenglObject::OpenglObject(QColor color, QObject *parent) :
    QObject(parent),
    m_color(color)
{

}

OpenglObject::~OpenglObject()
{

}