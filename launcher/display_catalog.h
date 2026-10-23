#pragma once
#include <QObject>
#include <QVariantList>
#include <QTimer>

// Enumerates through SDL, exactly like the game: Qt screen order is not the
// runtime's monitor numbering. Qt metadata is used only to label connectors.
class DisplayCatalog final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit DisplayCatalog(QObject* parent = nullptr);
    explicit DisplayCatalog(QVariantList fixtureRows, QObject* parent = nullptr);
    ~DisplayCatalog() override;
    QVariantList rows() const { return rows_; }
    QString error() const { return error_; }
    Q_INVOKABLE int rowForMonitor(int monitor) const;
    Q_INVOKABLE void refresh();
signals:
    void changed();
private:
    QVariantList rows_;
    QString error_;
    QTimer timer_;
    bool ownsVideo_ = false;
};
