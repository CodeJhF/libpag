#ifndef TRANSLATE_PAG_LANGUAGE_MODEL_H
#define TRANSLATE_PAG_LANGUAGE_MODEL_H

#include <QApplication>
#include <QObject>
#include <QTranslator>

enum Language : int8_t { Unknown = -1, Chinese = 0, English = 1 };

class PAGLanguageModel : public QObject {
  Q_OBJECT
 public:
  static void Init(QCoreApplication* app);
  static void ToEnglish();
  static void ToChinese();
  static bool SystemIsEnglish();

  Q_INVOKABLE void setLanguage(bool isEnglish);
  Q_INVOKABLE bool getSystemLanguage();

 private:
  static bool IsEnglish;
  static Language SystemLanguage;
  static QTranslator Translator;
  static QCoreApplication* App;
};

#endif  // TRANSLATE_PAG_LANGUAGE_MODEL_H