#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <QString>
#include <QStringList>
#include <QOpenGLShaderProgram>

class ShaderProgram
{
public:
    ShaderProgram(const QString &vertexShaderFilePath, const QString & fragmentShaderFilePath);
    bool Create();
    void Destroy();
    void Bind();
    void Release();

    void AddUniform(QString name);
    QOpenGLShaderProgram* GetRawProgram();

    QStringList m_uniformNames;
    QList<int> m_uniformIDs;

private:
    QOpenGLShaderProgram *m_program;
    QString m_vertexShaderFilePath;
    QString m_fragmentShaderFilePath;
};

#endif // SHADERPROGRAM_H