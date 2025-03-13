#ifndef RENDERING_PAG_QUICKITEM_TYPES_H_
#define RENDERING_PAG_QUICKITEM_TYPES_H_

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSize>
#include <QtGui/QOpenGLFunctions>

class PAGQuickItemProtocol {
 public:
  virtual ~PAGQuickItemProtocol() = default;
  virtual auto getFboSize() const -> QSize = 0;
  virtual auto getFramebufferId() const -> GLint = 0;
};

class TextureNode : public QObject, public QSGSimpleTextureNode {
  Q_OBJECT
 public:
  explicit TextureNode(QQuickWindow* window);
  ~TextureNode() override;

 private:
  QSGTexture* texture{nullptr};
  QQuickWindow* window{nullptr};
};

#endif  // RENDERING_PAG_QUICKITEM_TYPES_H_