#ifndef RENDERING_PAG_WINDOW_H_
#define RENDERING_PAG_WINDOW_H_

#include <QOBject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include "maintance/PAGCheckUpdateModel.h"
#include "profiling/PAGBenchmarkModel.h"
#include "profiling/PAGFileInfoModel.h"
#include "profiling/PAGRunTimeModelManager.h"
#include "rendering/PAGViewWindow.h"
#include "rendering/PAGWindowHelper.h"
#include "translate/PAGLanguageModel.h"

class PAGWindow : public QObject {
  Q_OBJECT
 public:
  explicit PAGWindow(QObject* parent = nullptr);
  ~PAGWindow() override;

  auto Open() -> void;
  auto getEngine() -> QQmlApplicationEngine*;
  auto getFilePath() -> QString;

  Q_SLOT void close();
  Q_SLOT void openFile(const QString& path);
  Q_SLOT void onPAGViewerDestroyed();
  Q_SLOT void onShowVideoFramesChanged(bool show);

  Q_SIGNAL void openPAGFile(QString path);
  Q_SIGNAL void destroyWindow(PAGWindow* window);

  Q_INVOKABLE void openProject(const QString& path);

  static QList<PAGWindow*> AllWindows;

 private:
  QString filePath;
  QQuickWindow* quickWindow = nullptr;
  QQmlApplicationEngine* qmlEngine = nullptr;
  PAGViewWindow* viewWindow = nullptr;
  PAGWindowHelper* windowHelper = nullptr;
  PAGLanguageModel* languageModel = nullptr;
  PAGFileInfoModel* fileInfoModel = nullptr;
  PAGBenchmarkModel* benchmarkModel = nullptr;
  PAGCheckUpdateModel* checkUpdateModel = nullptr;
  PAGRunTimeModelManager* runTimeModelManager = nullptr;
};

#endif  // RENDERING_PAG_WINDOW_H_