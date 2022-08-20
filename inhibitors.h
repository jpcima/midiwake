#pragma once
#include <QtCore/QObject>
#include <QtCore/QVector>

class Inhibitor : public QObject {
    Q_OBJECT

protected:
    explicit Inhibitor(QObject *parent) : QObject(parent) {}

public:
    virtual ~Inhibitor() {}
    virtual void inhibit(const QString &reason) = 0;
    virtual void uninhibit() = 0;
    virtual bool isValid() = 0;
    virtual QString getIdentifier() const = 0;

    static Inhibitor *createInhibitor(QObject *parent);
};
