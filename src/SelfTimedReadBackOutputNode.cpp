#include "SelfTimedReadBackOutputNode.h"
#include "Context.h"

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec)
    : OutputNode(new SelfTimedReadBackOutputNodePrivate(context, chainSize)) {

    d()->m_workerContext = new OpenGLWorkerContext(context->threaded());
    d()->m_worker = QSharedPointer<STRBONOpenGLWorker>(new STRBONOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);

    connect(d()->m_worker.data(), &STRBONOpenGLWorker::initialized, this, &SelfTimedReadBackOutputNode::initialize, Qt::DirectConnection);
    connect(d()->m_worker.data(), &STRBONOpenGLWorker::frame, this, &SelfTimedReadBackOutputNode::frame, Qt::DirectConnection);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize", Q_ARG(QSize, chainSize));
        Q_ASSERT(result);
    }
    if (msec != 0) {
        setInterval(msec);
    }
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(SelfTimedReadBackOutputNodePrivate *other, Context *context, QSize chainSize, long msec)
    : OutputNode(other) {

    d()->m_workerContext = new OpenGLWorkerContext(context->threaded());
    d()->m_worker = QSharedPointer<STRBONOpenGLWorker>(new STRBONOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);

    connect(d()->m_worker.data(), &STRBONOpenGLWorker::initialized, this, &SelfTimedReadBackOutputNode::initialize, Qt::DirectConnection);
    connect(d()->m_worker.data(), &STRBONOpenGLWorker::frame, this, &SelfTimedReadBackOutputNode::frame, Qt::DirectConnection);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize", Q_ARG(QSize, chainSize));
        Q_ASSERT(result);
    }
    if (msec != 0) {
        setInterval(msec);
    }
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other)
    : OutputNode(other)
{
}

SelfTimedReadBackOutputNode *SelfTimedReadBackOutputNode::clone() const {
    return new SelfTimedReadBackOutputNode(*this);
}

QSharedPointer<SelfTimedReadBackOutputNodePrivate> SelfTimedReadBackOutputNode::d() const {
    return d_ptr.staticCast<SelfTimedReadBackOutputNodePrivate>();
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(QSharedPointer<SelfTimedReadBackOutputNodePrivate> other_ptr)
    : OutputNode(other_ptr.staticCast<OutputNodePrivate>())
{
}

void SelfTimedReadBackOutputNode::start() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "start");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::stop() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "stop");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::setInterval(long msec) {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "setInterval", Q_ARG(long, msec));
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::force() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "onTimeout");
    Q_ASSERT(result);
}

// WeakSelcTimedReadBackOutputNode methods

WeakSelfTimedReadBackOutputNode::WeakSelfTimedReadBackOutputNode()
{
}

WeakSelfTimedReadBackOutputNode::WeakSelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<SelfTimedReadBackOutputNodePrivate> WeakSelfTimedReadBackOutputNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

// STRBONOpenGLWorker methods

STRBONOpenGLWorker::STRBONOpenGLWorker(SelfTimedReadBackOutputNode p)
    : OpenGLWorker(p.d()->m_workerContext)
    , m_p(p) {
}

QSharedPointer<QOpenGLShaderProgram> STRBONOpenGLWorker::loadBlitShader() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto vertexString = QString{
        "#version 150\n"
        "out vec2 uv;\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "void main() {\n"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5*(vertex+1.);\n"
        "}\n"};
    auto fragmentString = QString{
        "#version 150\n"
        "uniform sampler2D iFrame;\n"
        "in vec2 uv;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = texture(iFrame, uv);\n"
        "}\n"};

    auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit fatal("Could not compile vertex shader");
        return nullptr;
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit fatal("Could not compile fragment shader");
        return nullptr;
    }
    if (!shader->link()) {
        emit fatal("Could not link shader program");
        return nullptr;
    }

    return shader;
}

void STRBONOpenGLWorker::initialize(QSize size) {
    Q_ASSERT(QThread::currentThread() == thread());

    makeCurrent();
    m_shader = loadBlitShader();
    if (m_shader.isNull()) return;

    auto fmt = QOpenGLFramebufferObjectFormat{};
    fmt.setInternalTextureFormat(GL_RGBA);
    m_fbo = QSharedPointer<QOpenGLFramebufferObject>::create(size, fmt);

    m_size = size;
    m_pixelBuffer.resize(4 * size.width() * size.height());

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &STRBONOpenGLWorker::onTimeout);

    emit initialized();
}

void STRBONOpenGLWorker::setInterval(long msec) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_timer == nullptr) {
        qWarning() << "Node not ready, ignoring setInterval";
        return;
    }

    m_timer->setInterval(msec);
}

void STRBONOpenGLWorker::start() {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_timer == nullptr) {
        qWarning() << "Node not ready, ignoring start";
        return;
    }

    m_timer->start();
}

void STRBONOpenGLWorker::stop() {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_timer == nullptr) {
        qWarning() << "Node not ready, ignoring stop";
        return;
    }
    m_timer->stop();
}

void STRBONOpenGLWorker::onTimeout() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // SelfTimedReadBackOutputNode was deleted
    SelfTimedReadBackOutputNode p(d);

    makeCurrent();
    GLuint texture = p.render();

    if (texture == 0) {
        qWarning() << "No frame available";
        return;
    }

    auto vao = p.chain().vao();

    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_fbo->bind();
    // VAO??
    glViewport(0, 0, m_size.width(), m_size.height());

    m_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    m_shader->setUniformValue("iFrame", 0);

    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao->release();

    glReadPixels(0, 0, m_size.width(), m_size.height(), GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());

    m_fbo->release();
    m_shader->release();

    emit frame(m_size, m_pixelBuffer);
}

SelfTimedReadBackOutputNodePrivate::SelfTimedReadBackOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
{
}
